#include "PacketSubscription.h"
#include "PacketSubscriber.h"

PacketSubscription::PacketSubscription( std::shared_ptr<PacketSubscriber>& subscriber )
:
    m_subscriber( subscriber )
{
}

PacketSubscription::PacketSubscription( PacketSubscription&& toMove )
{
    std::swap( m_subscriber, toMove.m_subscriber );
}

PacketSubscription& PacketSubscription::operator=( PacketSubscription&& toMove )
{
    std::swap( m_subscriber, toMove.m_subscriber );
    return *this;
}

/**
    When a subscription goes out of scope it automatically removes
    its subscription from the PacketMuxer that created it.
*/
PacketSubscription::~PacketSubscription()
{
    if (m_subscriber != nullptr && m_subscriber.use_count() > 1)
    {
        m_subscriber->Unsubscribe();
    }
}

/**
    @return true if the underlying subscriber is still subscribed, false otherwise.

    The underlying subscriber can be unsubscribed before the PacketSubscription
    goes out of scope if the PacketDemuxer's receive thread terminates.
*/
bool PacketSubscription::IsSubscribed() const
{
    return m_subscriber->IsSubscribed();
}

const PacketDemuxer& PacketSubscription::GetDemuxer() const
{
    return m_subscriber->GetDemuxer();
}
