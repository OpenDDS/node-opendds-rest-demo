#include "GameTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>

#include <ace/Get_Opt.h>

#include <iostream>

const CORBA::Long CONTROL_DOMAIN = 9;

std::map<std::string, Game::AugmentedPlayerConnection> apc_map;

struct PlayerConnectionDataReaderListener : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
  PlayerConnectionDataReaderListener() : apc_dw_(0) {}
  virtual ~PlayerConnectionDataReaderListener() {}

  virtual void on_requested_deadline_missed(
    DDS::DataReader*,
    const DDS::RequestedDeadlineMissedStatus&) {}
  virtual void on_requested_incompatible_qos(
    DDS::DataReader*,
    const DDS::RequestedIncompatibleQosStatus&) {}
  virtual void on_liveliness_changed(
    DDS::DataReader*,
    const DDS::LivelinessChangedStatus&) {}
  virtual void on_subscription_matched(
    DDS::DataReader*,
    const DDS::SubscriptionMatchedStatus&) {}
  virtual void on_sample_rejected(
    DDS::DataReader*,
    const DDS::SampleRejectedStatus&) {}
  virtual void on_sample_lost(
    DDS::DataReader*,
    const DDS::SampleLostStatus&) {}

  virtual void on_data_available(DDS::DataReader* reader)
  {
    std::cout << "maybe got new player connection data" << std::endl;
    Game::PlayerConnectionDataReader_var pc_dr =
      Game::PlayerConnectionDataReader::_narrow(reader);

    if (!pc_dr) {
      return;
    }

    Game::PlayerConnection pc;
    DDS::SampleInfo si;

    if (pc_dr->take_next_sample(pc, si) == DDS::RETCODE_OK) {
        std::string guid(pc.guid.in());
      if (si.valid_data) {
        auto pos = apc_map.find(guid);
        if (pos == apc_map.end()) {
          std::cout << "Got new PlayerConnection for player_id '" << pc.player_id.in() << "' with guid '" << pc.guid.in() << "'" << std::endl;
          auto& apc = apc_map[guid];
          apc.guid = pc.guid;
          apc.player_id = pc.player_id;
          apc.connected_since = pc.connected_since;
          apc.server_id = pc.server_id;
          apc.force_disconnect = false;
          apc.average_connection = 0.0f;
          apc_dw_->write(apc, 0);
        } else {
          std::cout << "Got duplicate PlayerConnection for player_id '" << pc.player_id.in() << "' with existing guid " << pc.guid.in() << "'" << std::endl;
        }
      } else if (si.instance_state & DDS::NOT_ALIVE_INSTANCE_STATE) {
        std::cout << "Removing PlayerConnection with guid '" << pc.guid.in() << "'" << std::endl;
        auto pos = apc_map.find(guid);
        if (pos != apc_map.end() && pos->second.force_disconnect != true) {
          apc_dw_->dispose(pos->second, 0);
          apc_map.erase(pos);
        }
      }
    }
  }

  void set_apc_writer(Game::AugmentedPlayerConnectionDataWriter_ptr apc_dw) {
    apc_dw_ = apc_dw;
  }

  Game::AugmentedPlayerConnectionDataWriter_ptr apc_dw_;
};

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try
  {
    // Create Domain Participant

    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    DDS::DomainParticipant_var participant =
      dpf->create_participant(CONTROL_DOMAIN,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(participant.in())) {
      std::cerr << "create_participant failed." << std::endl;
      return 1;
    }

    // Register Types

    Game::PlayerConnectionTypeSupportImpl::_var_type pc_ts_i = new Game::PlayerConnectionTypeSupportImpl();

    if (DDS::RETCODE_OK != pc_ts_i->register_type(participant.in(), "")) {
      std::cerr << "register_type failed." << std::endl;
      exit(1);
    }

    CORBA::String_var pc_type_name = pc_ts_i->get_type_name();

    Game::AugmentedPlayerConnectionTypeSupportImpl::_var_type apc_ts_i = new Game::AugmentedPlayerConnectionTypeSupportImpl();

    if (DDS::RETCODE_OK != apc_ts_i->register_type(participant.in(), "")) {
      std::cerr << "register_type failed." << std::endl;
      exit(1);
    }

    CORBA::String_var apc_type_name = apc_ts_i->get_type_name();

    // Create Topics

    DDS::TopicQos topic_qos;
    participant->get_default_topic_qos(topic_qos);

    DDS::Topic_var pc_topic =
      participant->create_topic("Player Connections",
                                pc_type_name.in(),
                                topic_qos,
                                DDS::TopicListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(pc_topic.in())) {
      std::cerr << "create_topic failed." << std::endl;
      exit(1);
    }

    DDS::Topic_var apc_topic =
      participant->create_topic("Augmented Player Connections",
                                apc_type_name.in(),
                                topic_qos,
                                DDS::TopicListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(apc_topic.in())) {
      std::cerr << "create_topic failed." << std::endl;
      exit(1);
    }

    // Create Publisher

    DDS::PublisherQos pub_qos;
    participant->get_default_publisher_qos(pub_qos);

    DDS::Publisher_var pub =
      participant->create_publisher(pub_qos, DDS::PublisherListener::_nil(),
                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(pub.in())) {
      std::cerr << "create_publisher failed." << std::endl;
      exit(1);
    }

    // Create DataWriter

    DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos(dw_qos);
    dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::DataWriter_var dw =
      pub->create_datawriter(apc_topic.in(),
                             dw_qos,
                             DDS::DataWriterListener::_nil(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(dw.in())) {
      std::cerr << "create_datawriter failed." << std::endl;
      exit(1);
    }

    Game::AugmentedPlayerConnectionDataWriter_var apc_dw =
      Game::AugmentedPlayerConnectionDataWriter::_narrow(dw.in());

    if (CORBA::is_nil(apc_dw.in())) {
      std::cerr << "Game::AugmentedPlayerConnectionDataWriter::_narrow() failed." << std::endl;
      exit(1);
    }

    // Create Subscriber

    DDS::SubscriberQos sub_qos;
    participant->get_default_subscriber_qos(sub_qos);

    DDS::Subscriber_var sub =
      participant->create_subscriber(sub_qos, DDS::SubscriberListener::_nil(),
                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(sub.in())) {
      std::cerr << "create_subscriber failed." << std::endl;
      exit(1);
    }

    // Create DataReaderListener

    PlayerConnectionDataReaderListener* pc_drli = new PlayerConnectionDataReaderListener();
    DDS::DataReaderListener_var pc_drl = pc_drli;
    pc_drli->set_apc_writer(apc_dw.in());

    // Create DataReader

    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::DataReader_var dr =
      sub->create_datareader(pc_topic.in(),
                             dr_qos,
                             pc_drl.in(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(dr.in())) {
      std::cerr << "create_datareader failed." << std::endl;
      exit(1);
    }

    Game::PlayerConnectionDataReader_var pc_dr =
      Game::PlayerConnectionDataReader::_narrow(dr.in());

    if (CORBA::is_nil(pc_dr.in())) {
      std::cerr << "Game::PlayerConnectionDataReader::_narrow() failed." << std::endl;
      exit(1);
    }

    // TODO Stuff

    std::cout << "Control Ready" << std::endl;

    while (true) {
      ACE_OS::sleep(ACE_Time_Value(1, 0));
    }

    std::cout << "Control Shutting Down" << std::endl;

    // Clean Up

    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());
  }
  catch (const CORBA::Exception& e)
  {
    std::cerr << "PUB: Exception caught in main.cpp:" << std::endl << e << std::endl;
    exit(1);
  }
  TheServiceParticipant->shutdown();

  return 0;
}
