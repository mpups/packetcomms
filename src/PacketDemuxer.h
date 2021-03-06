/*
    Copyright (C) Mark Pupilli 2013, All rights reserved
*/
#ifndef __PACKET_DEMUXER_H__
#define __PACKET_DEMUXER_H__

#include <initializer_list>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "ComPacket.h"
#include "ControlMessage.h"
#include "IdManager.h"
#include "PacketSubscriber.h"
#include "PacketSubscription.h"
#include "network/AbstractSocket.h"

/**
    Class which manages communications to and from the robot.

    This is the demuxer half of the comms-system - it does not
    know anything about the messages except their size and type ID
    (ComPacket::Type).

    Throughout the low-level comms system the type ID is only used
    for muxing/demuxing packets into different send/receive queues.
    All serialisation of the actual packet data must be done externally.

    The demuxing reads packets from the transport layer and then sends
    these packets onto any subscribers registered for the packet type.

    The data itself is currently sent as byte stream over TCP.
*/
class PacketDemuxer {
 public:
  typedef std::shared_ptr<PacketSubscriber> SubscriberPtr;

  PacketDemuxer(AbstractReader& socket, const std::vector<std::string>& packetIds);
  virtual ~PacketDemuxer();

  bool ok() const;

  PacketSubscription subscribe(const std::string& type, PacketSubscriber::CallBack callback);
  void unsubscribe(const PacketSubscriber* subscriber);
  bool isSubscribed(const PacketSubscriber* subscriber) const;

  void receiveLoop();
  bool receivePacket(ComPacket& packet, const int timeoutInMilliseconds);

  const IdManager& getIdManager() const { return m_packetIds; }

 protected:
  typedef std::pair<IdManager::PacketType, std::vector<SubscriberPtr> > SubscriptionEntry;

  bool readBytes(uint8_t* buffer, size_t& size, bool transportErrorOnZeroBytes = false);
  void signalTransportError();

 private:
  IdManager m_packetIds;
  std::mutex m_subscriberLock;
  std::unordered_map<SubscriptionEntry::first_type, SubscriptionEntry::second_type> m_subscribers;
  AbstractReader& m_transport;
  bool m_transportError;

  // This must be initialised last to ensure all other members are intialised before the thread starts:
  std::thread m_receiverThread;

  void receiveHelloMessage(ComPacket& packet, int timeoutInMillisecs);
  void handleControlMessage(const ComPacket::ConstSharedPacket& sptr);
  ControlMessage getControlMessage(const ComPacket::ConstSharedPacket& sptr);
  void warnAboutSubscribers();
};

#endif /* __PACKET_DEMUXER_H__ */
