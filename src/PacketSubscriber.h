#ifndef PACKETSUBSCRIBER_H
#define PACKETSUBSCRIBER_H

#include "ComPacket.h"
#include "IdManager.h"

#include <functional>

class PacketDemuxer;

/**
    A packet subscription is a component which can subscribe to
    a particular packet type to be received from a PacketDemuxer.
*/
class PacketSubscriber {
  friend class PacketDemuxer;       /// friended so it can access m_callback directly @todo Expose callback through getter instead?
  friend class PacketSubscription;  // friended so it can call Unsubscribe()

 public:
  typedef std::function<void(const ComPacket::ConstSharedPacket&)> CallBack;

  virtual ~PacketSubscriber();
  PacketSubscriber(PacketSubscriber&)  = delete;
  PacketSubscriber(PacketSubscriber&&) = delete;

  IdManager::PacketType getType() const { return m_type; };

  const PacketDemuxer& getDemuxer() const { return m_comms; };

 protected:
  PacketSubscriber(const IdManager::PacketType, PacketDemuxer&, CallBack&);
  void unsubscribe();
  bool isSubscribed() const;

 private:
  const IdManager::PacketType m_type;
  PacketDemuxer& m_comms;
  CallBack m_callback;
};

#endif  // PACKETSUBSCRIBER_H
