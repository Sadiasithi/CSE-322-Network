/*
 * Copyright (c) 2016 ResiliNets, ITTC, University of Kansas
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Truc Anh N. Nguyen <annguyen@ittc.ku.edu>
 *          Keerthi Ganta <keerthig@ittc.ku.edu>
 *          Md. Moshfequr Rahman <moshfequr@ittc.ku.edu>
 *          Amir Modarresi <amodarresi@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 */

#include "tcp-scalable-tweak.h"

#include "tcp-socket-state.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpScalableTweak");
NS_OBJECT_ENSURE_REGISTERED(TcpScalableTweak);

TypeId
TcpScalableTweak::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpScalableTweak")
                            .SetParent<TcpNewReno>()
                            .AddConstructor<TcpScalableTweak>()
                            .SetGroupName("Internet")
                            .AddAttribute("AIFactor",
                                          "Additive Increase Factor",
                                          UintegerValue(50),
                                          MakeUintegerAccessor(&TcpScalableTweak::m_aiFactor),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("MDFactor",
                                          "Multiplicative Decrease Factor",
                                          DoubleValue(0.125),
                                          MakeDoubleAccessor(&TcpScalableTweak::m_mdFactor),
                                          MakeDoubleChecker<double>());
    return tid;
}

TcpScalableTweak::TcpScalableTweak()
    : TcpNewReno(),
      m_ackCnt(0),
      m_aiFactor(50),
      m_mdFactor(0.125)
{
    NS_LOG_FUNCTION(this);
}

TcpScalableTweak::TcpScalableTweak(const TcpScalableTweak& sock)
    : TcpNewReno(sock),
      m_ackCnt(sock.m_ackCnt),
      m_aiFactor(sock.m_aiFactor),
      m_mdFactor(sock.m_mdFactor)
{
    NS_LOG_FUNCTION(this);
}

TcpScalableTweak::~TcpScalableTweak()
{
    NS_LOG_FUNCTION(this);
}

Ptr<TcpCongestionOps>
TcpScalableTweak::Fork()
{
    return CopyObject<TcpScalableTweak>(this);
}

void
TcpScalableTweak::CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);

    uint32_t segCwnd = tcb->GetCwndInSegments();
    NS_ASSERT(segCwnd >= 1);

    uint32_t oldCwnd = segCwnd;

    // Dynamically adjust AI factor based on current cwnd size
    uint32_t dynamicAiFactor = std::max(1u, m_aiFactor * (segCwnd / 10)); //Ensures that the AI factor does not fall below 1, even when segCwnd is very small
    uint32_t w = std::min(segCwnd, dynamicAiFactor);

    if (m_ackCnt >= w)
    {
        m_ackCnt = 0;
        segCwnd++;
    }

    m_ackCnt += segmentsAcked;
    if (m_ackCnt >= w)
    {
        uint32_t delta = m_ackCnt / w;
        m_ackCnt = 0;
        segCwnd += delta;
    }

    if (segCwnd != oldCwnd)
    {
        tcb->m_cWnd = segCwnd * tcb->m_segmentSize;
        NS_LOG_INFO("In CongAvoid, updated to cwnd " << tcb->m_cWnd << " ssthresh "
                                                     << tcb->m_ssThresh);
    }
}

std::string
TcpScalableTweak::GetName() const
{
    return "TcpScalableTweak";
}

uint32_t
TcpScalableTweak::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
    NS_LOG_FUNCTION(this << tcb << bytesInFlight);

    uint32_t segCwnd = bytesInFlight / tcb->m_segmentSize;

    // Adjusting multiplicative decrease factor to be less aggressive
    double b = 1.0 - std::min(0.0625, m_mdFactor); // predefined minimum threshold or lower bound used in the congestion control mechanism is 0.0625
    uint32_t ssThresh = static_cast<uint32_t>(std::max(2.0, segCwnd * b));

    NS_LOG_DEBUG("Calculated b(w) = " << b << " resulting (in segment) ssThresh=" << ssThresh);

    return ssThresh * tcb->m_segmentSize;
}

} // namespace ns3
