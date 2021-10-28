/*
    Copyright (C) Mark Pupilli 2012, All rights reserved
*/
#ifndef _COMSUBSCRIBER_H_
#define _COMSUBSCRIBER_H_

#include <memory>

class PacketSubscriber;
class PacketDemuxer;

/**
    This class is a wrapper around a shared pointer to a PacketSubscriber.
    The primary purpose of this class is the automatic unsubscription of the
    wrapped subscriber when a PacketSubscription object goes out of scope.
    hence the destructor calls PacketSubscriber::Unsubscribe() on the wrapped
    subscriber.
*/
class PacketSubscription {
 public:
  PacketSubscription() {}
  PacketSubscription(const PacketSubscription&) = delete;
  PacketSubscription& operator=(PacketSubscription&) = delete;

  PacketSubscription(std::shared_ptr<PacketSubscriber>& subscriber);
  PacketSubscription(PacketSubscription&&);
  PacketSubscription& operator=(PacketSubscription&&);

  virtual ~PacketSubscription();

  bool isSubscribed() const;
  const PacketDemuxer& getDemuxer() const;

 protected:
 private:
  std::shared_ptr<PacketSubscriber> m_subscriber;
};

#endif  // _COMSUBSCRIBER_H_
