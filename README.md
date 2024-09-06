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

1. Set up the OpenDDS environment variables (DDS_ROOT, TAO_ROOT, ACE_ROOT etc)
2. Update PATH to include OpenDDS/bin
3. Update LD_LIBRARY_PATH (or similar) to include lib subdirectories of ACE_ROOT and DDS_ROOT
    * Note: the preceeding steps can be done in one command by sourcing OpenDDS's setenv.sh/setenv.cmd script
4. Set the environment variable DEMO_ROOT to point to the root of this repository
5. Generate project files using mwc. Assuming `gnuace`:
```
  opendds_mwc.pl node-opendds-rest-demo.mwc
```
6. Build C++ IDL library and `control` application. Again, assuming 'gnuace':
```
  make depend && make
```
 Note: `make depend` is only needed if IDL / source code will be modified after its built
    * For Visual C++: build the generated solution
7. Build Node.js `server` application
```
  cd server
  npm install
  cd ..
```
* For Visual C++: add `--debug --lib_suffix=d` after `npm install` if using a Debug configuration

## Running the Demo

For this step, you'll likely want to spawn multiple consoles / terminals in order to run both the client and the server
at the same time. Generally speaking, you hopefully shouldn't ever need to restart the control application.

### Single Server

1. Run the control application. Assuming some flavor of Linux/UNIX:
```
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$DEMO_ROOT/lib
  control/control -DCPSConfigFile rtps.ini
```
And for Windows:
```
  set PATH=%PATH%;%DEMO_ROOT%\lib
  control\control -DCPSConfigFile rtps.ini
```
2. Run the server application
```
  cd server
  node main.js -DCPSConfigFile ../rtps.ini
```
3. Navigate a javascript-enabled web browser to [http://localhost:3210](http://localhost:3210)

### Multiple Servers

Same as the steps above, though you will obviously need to launch multiple 'server' applications running on different
ports (use the `--port <PORT>` option) as well as have multiple browser tabs open to connect to each of the servers.

### Misc

Additionally, this demo also exists in a form that supports websockets interactions between client and server. Check out the `websockets` branch if you're interested in seeing those improvements.

