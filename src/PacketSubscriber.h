#ifndef PACKETSUBSCRIBER_H
#define PACKETSUBSCRIBER_H

#include "IdManager.h"
#include "ComPacket.h"

#include <functional>

class PacketDemuxer;

/**
    A packet subscription is a component which can subscribe to
    a particular packet type to be received from a PacketDemuxer.
*/
class PacketSubscriber
{
friend class PacketDemuxer; /// friended so it can access m_callback directly @todo Expose callback through getter instead?
friend class PacketSubscription; // friended so it can call Unsubscribe()

public:
    typedef std::function< void( const ComPacket::ConstSharedPacket& ) > CallBack;

    virtual ~PacketSubscriber();
    PacketSubscriber( PacketSubscriber& ) = delete;
    PacketSubscriber( PacketSubscriber&& ) = delete;

    IdManager::PacketType GetType() const { return m_type; };

    const PacketDemuxer& GetDemuxer() const { return m_comms; };

protected:
    PacketSubscriber( const IdManager::PacketType, PacketDemuxer&, CallBack& );
    void Unsubscribe();
    bool IsSubscribed() const;

private:
    const IdManager::PacketType m_type;
    PacketDemuxer&  m_comms;
    CallBack        m_callback;
};


#endif // PACKETSUBSCRIBER_H
