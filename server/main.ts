import { v4 as uuidv4 } from 'uuid';
import express, { Application, Request, Response } from 'express';

import expressWs from 'express-ws';
import { GameControlClient } from './GameControlClient';
const { app, getWss } = expressWs(express());

var my_args = process.argv.slice(2);
var port_index = my_args.indexOf("--port");
var PORT: number = (port_index == -1 || my_args.length <= port_index + 1) ? 3210 : Number(my_args[port_index + 1]);

const gc_client = new GameControlClient();

// Split out DDS args
var ddsArgs = process.argv.slice(process.argv.indexOf(__filename) + 1);

gc_client.initializeDds(ddsArgs);

const get_time = () => {
  var date = new Date();
  var ms = date.getTime();
  return { sec: Math.floor(ms / 1000), nsec: (ms % 1000) * 1000000 };
}

var server_id = uuidv4();

var apc_cache = new Map();


// middleware for debugging
app.use(function (req, res, next) {
  //console.log(app.ws)
  return next();
});

app.ws('/ws', (ws: any, req: Request) => {
  ws.on('message', (msg) => {
    console.log(`received: ${msg}`);
    ws.send(`echo ${msg}`);
  });
  ws.on('close', (code) => {
    console.error(`Client gone with code: ${code}`);
  });
});

app.put('/playerConnection/:player_name/:player_id', (req: Request, res: Response) => {
  const {player_name, player_id} = req.params;
  var pc = {
    // this is derived from <Game.idl>::PlayerConnection
    guid: player_id,
    player_id: player_name,
    connected_since: get_time(),
    server_id : server_id,
    vehicleSpeed: 4000
  };
  let success = gc_client.create_player_connection(pc);
  res.status(success ? 200 : 404).end();
});

app.delete('/playerConnection/:player_name/:player_id', (req: Request, res: Response) => {
  const {player_name, player_id} = req.params;
  var pc = {
    // this is derived from <Game.idl>::PlayerConnection
    guid: player_id,
    player_id: player_name,
    connected_since: get_time(),
    server_id : server_id
  };
  let success = gc_client.remove_player_connection(pc);
  res.status(success ? 200 : 404).end();
});

app.get('/augmentedPlayerConnection/', (req: Request, res: Response) => {
  res.status(200).send(JSON.stringify(apc_cache.size ? Object.fromEntries(apc_cache) : {}));
});

app.get('/', (req: Request, res: Response) => {
  res.sendFile(__dirname + '/index.html');
});

app.get('/favicon.ico', (req: Request, res: Response) => {
  res.sendFile(__dirname + '/favicon.ico');
});

let do_listen = true;
while (do_listen) {
  try {
    app.listen(PORT, function(err) {
      if (err) {
        console.log("Error in server setup");
      }
      console.log('listening on *:' + PORT);
    });
    do_listen = false;
  } catch (e) {
    PORT++;
  }
}

function add_apc(sample) {
  console.log('Adding APC:' + JSON.stringify(sample));
  apc_cache.set(sample.guid, sample);
  notifyWs(sample);
}

function remove_apc(guid:string) {
  console.log('Removing APC for guid:' + JSON.stringify(guid));
  apc_cache.delete(guid);
  notifyWs(`${guid} left the game`);
}

function notifyWs(msg) {
  // get all clients connetced via webSocket
  const clients = getWss().clients;
  clients.forEach((client) => {
    //publish the same message via WebSocket
    client.send(JSON.stringify(msg));
  });
}

gc_client.subscribe_augmented_player_connections(add_apc, remove_apc);