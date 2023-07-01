import * as opendds from 'opendds';
import * as path from 'path';

export class GameControlClient {
  public playerConnectionWriter = null;
  public augmentedPlayerConnectionReader = null;

  private factory;
  private participant;
  private library;

  public constructor() {
    // Handle exit gracefully
    process.on('SIGINT', () => {
      console.log("OnSIGINT");
      this.finalizeDds();
      process.exit(0);
    });
    process.on('SIGTERM', () => {
      console.log("OnSIGTERM");
      this.finalizeDds();
      process.exit(0);
    });
    process.on('exit', () => {
      console.log("OnExit");
      this.finalizeDds();
    });
  }

  public create_player_connection(player_connection_request) {
    if (this.playerConnectionWriter) {
      console.log("Attempting to write player connection: " + JSON.stringify(player_connection_request));
      try {
        this.playerConnectionWriter.write(player_connection_request);
        return true;
      } catch (err) {
        console.error(err.message);
      }
    }
    return false;
  };

  public remove_player_connection(player_connection_request) {
    if (this.playerConnectionWriter) {
      console.log("Attempting to unregister / dispose player connection: " + JSON.stringify(player_connection_request));
      try {
        this.playerConnectionWriter.dispose(player_connection_request);
        return true;
      } catch (err) {
        console.error(err.message);
      }
    }
    return false;
  };

  public subscribe_augmented_player_connections(apc_received, apc_removed) {
    try {
      var qos = { durability: 'TRANSIENT_LOCAL_DURABILITY_QOS' };
      this.augmentedPlayerConnectionReader =
        this.participant.subscribe(
          'Augmented Player Connections',
          'Game::AugmentedPlayerConnection',
          qos,
          function (dr, sInfo, sample) {
            if (sInfo.valid_data) {
              apc_received(sample);
            } else if (sInfo.instance_state == 2 || sInfo.instance_state == 4) {
              console.log("About to call apc_removed for guid '" + JSON.stringify(sample.guid) + '"');
              apc_removed(sample.guid);
            }
          }
        );
      console.log("successfully created APC reader");
    } catch (e) {
      console.log(e);
    }
  };

  private finalizeDds() {
    if (this.factory) {
      console.log("finalizing DDS connection");
      if (this.participant) {
        this.factory.delete_participant(this.participant);
        delete this.participant;
      }
      opendds.finalize(this.factory);
      delete this.factory;
    }
  };

  public initializeDds(argsArray) {
    var CONTROL_DOMAIN_ID = 9;
    this.factory = opendds.initialize.apply(null, argsArray);
    if (!process.env.DEMO_ROOT) {
      throw new Error("DEMO_ROOT environment variable not set");
    }
    this.library = opendds.load(path.join(process.env.DEMO_ROOT, 'lib', 'Game_Idl'));
    if (!this.library) {
      throw new Error("Could not open type support library");
    }
    this.participant = this.factory.create_participant(CONTROL_DOMAIN_ID);

    try {
      var qos = {};
      this.playerConnectionWriter =
        this.participant.create_datawriter(
          'Player Connections',
          'Game::PlayerConnection',
          qos
        );
      console.log("successfully created PC writer");
    } catch (e) {
      console.log(e);
    }
  };
}
