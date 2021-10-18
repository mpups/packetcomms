#include "PacketSubscriber.h"
#include "PacketDemuxer.h"

/**
    This is the only constructor and is protected: the intent being
    that only a PacketDemuxer object can construct a PacketSubscription.
*/
PacketSubscriber::PacketSubscriber( const IdManager::PacketType type, PacketDemuxer& comms, CallBack& callback )
:
    m_type      ( type ),
    m_comms     ( comms ),
    m_callback  ( callback)
{
}

PacketSubscriber::~PacketSubscriber()
{
}

/**
    This is protected and only should be accessed by the friend class PacketSubscription.

    Currently if a client was allowed to call unsubscribe on a subscriber then an assert
    would trigger in PacketDemuxer::Unsubscribe() when the PacketSubscription attempted
    to unsubscribe automatically on destruction.
*/
void PacketSubscriber::Unsubscribe()
{
    m_comms.Unsubscribe( this );
}

bool PacketSubscriber::IsSubscribed() const
{
    return m_comms.IsSubscribed( this );
}
