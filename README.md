# A Demonstration of using node-opendds to build a custom REST-to-DDS bridge

This is a very simple example of using node-opendds (the Node.js bindings for OpenDDS) in order to build a custom
bridge between RESTful web services and DDS. The premise of this demo is based on a multiplayer game in which a
single C++ 'control' application communicates using DDS with several Node.js 'server' bridge applications. Players
may simply point a javascript capable browser at a running server (default port of 3210) in order to pull down a
lightweight client web application in order to join the game.

The emphasis of this demo is to show how simple HTTP requests ('PUT', 'GET', 'DELETE') may be used to interact with
DDS sample instances via a bridgine application, and as a result polling is used in order to get notifications of
changes to game states. For real-world applications, this is obviously not ideal and some kind of asynchronous push
notification (e.g. websockets) would most likely be used for the sake of performance and user experience.

## Building the Demo

 1.
   - Set up the OpenDDS environment variables (DDS_ROOT, TAO_ROOT, ACE_ROOT etc)
   - Set up PATH / LD_LIBRARY_PATH to include OpenDDS, TAO, ACE, and MPC binaries
```
  cd <PathToOpenDDS>
  source sentenv.sh
  cd <pathToThisDemoRoot>
```
 2. Set up the environment variable DEMO_ROOT to point to the root of this repository
```
  export DEMO_ROOT=$(pwd)
```
 3. Generate project files using mwc. Assuming `gnuace`:
```
  mwc.pl -type gnuace node-opendds-rest-demo.mwc
```
 4. Build C++ IDL library and `control` application. Again, assuming 'gnuace':
```
  make depend && make
```
 5. Build Node.js `server` application
```
  cd server
  npm install
  cd -
```

## Running the Demo

For this step, you'll likely want to spawn multiple consoles / terminals in order to run both the client and the server
at the same time. Generally speaking, you hopefully shouldn't ever need to restart the control application.

### Single Server

 1. Run the control application. Assuming some flavor of Linux/UNIX:
```
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/idl
  cd control
  ./control -DCPSPendingTimeout 3 -DCPSConfigFile ../rtps.ini
```
 2. Run the server application
```
  cd server
  node main.js -DCPSPendingTimeout 3 -DCPSConfigFile ../rtps.ini
```
 3. Navigate a javascript-enabled web browser to [http://localhost:3210](http://localhost:3210)

### Multiple Servers

Same as the steps above, though you will obviously need to launch multiple 'server' applications running on different
ports (use the `--port <PORT>` option) as well as have multiple browser tabs open to connect to each of the servers.

