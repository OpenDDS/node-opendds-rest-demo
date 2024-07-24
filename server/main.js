var GameControlClient = require('./game_control_client')
var Uint64BE = require("int64-buffer").Uint64BE;
const { v4 : uuidv4 } = require('uuid');

var my_args = process.argv.slice(2);
var port_index = my_args.indexOf("--port");
var PORT = (port_index == -1 || my_args.size <= port_index + 1) ? 3210 : my_args[port_index + 1];
var app = require('express')();
var http = require('http').Server(app);
//var io = require('socket.io', { rememberTransport: false, transports: ['websocket'] })(http);

var gc_client = new GameControlClient();

// Split out DDS args
var ddsArgs = process.argv.slice(process.argv.indexOf(__filename) + 1);

gc_client.initializeDds(ddsArgs);

function get_time() {
  var date = new Date();
  var ms = date.getTime();
  return { sec: Math.floor(ms / 1000), nanosec: (ms % 1000) * 1000000 };
}

var server_id = uuidv4();

var apc_cache = new Map();

app.put('/playerConnection/:player_name/:player_id', function(req, res) {
  const {player_name, player_id} = req.params;
  var pc = {
    guid: player_id,
    player_id: player_name,
    connected_since: get_time(),
    server_id : server_id
  };
  let success = gc_client.create_player_connection(pc);
  res.status(success ? 200 : 404).end();
});

app.delete('/playerConnection/:player_name/:player_id', function(req, res) {
  const {player_name, player_id} = req.params;
  var pc = {
    guid: player_id,
    player_id: player_name,
    connected_since: get_time(),
    server_id : server_id
  };
  let success = gc_client.remove_player_connection(pc);
  res.status(success ? 200 : 404).end();
});

app.get('/augmentedPlayerConnection/', function(req, res){
  res.status(200).send(JSON.stringify(apc_cache.size ? Object.fromEntries(apc_cache) : {}));
});

app.get('/', function(req, res){
  res.sendFile(__dirname + '/index.html');
});

app.get('/favicon.ico', function(req, res){
  res.sendFile(__dirname + '/favicon.ico');
});

let do_listen = true;
while (do_listen) {
  try {
    http.listen(PORT, function(err) {
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
}

function remove_apc(guid) {
  console.log('Removing APC for guid:' + JSON.stringify(guid));
  apc_cache.delete(guid);
}

gc_client.subscribe_augmented_player_connections(add_apc, remove_apc);

