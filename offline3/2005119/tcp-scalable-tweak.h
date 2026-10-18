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

#ifndef TCPSCALABLETWEAK_H
#define TCPSCALABLETWEAK_H

#include "tcp-congestion-ops.h"

namespace ns3
{

class TcpSocketState;

/**
 * \ingroup congestionOps
 *
 * \brief An implementation of TCP Scalable Tweak
 *
 * TcpScalableTweak enhances TCP Scalable by dynamically adjusting the
 * Additive Increase (AI) factor based on the current congestion window size.
 * This provides better utilization in high-speed networks and
 * reduces congestion in smaller windows.
 *
 * The Additive Increase is calculated dynamically as:
 *
 *     dynamicAiFactor = max(1, AI * (cwnd / 10))
 *
 * Multiplicative Decrease (MD) reduces congestion window more gracefully:
 *
 *     cwnd = cwnd * (1 - min(0.0625, MD))
 */
class TcpScalableTweak : public TcpNewReno
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Default constructor.
     */
    TcpScalableTweak();

    /**
     * \brief Copy constructor
     * \param sock the object to copy
     */
    TcpScalableTweak(const TcpScalableTweak& sock);

    ~TcpScalableTweak() override;

    /**
     * \brief Get the name of the congestion control algorithm
     * \return the algorithm name
     */
    std::string GetName() const override;

    /**
     * \brief Get slow start threshold following TcpScalableTweak principle
     *
     * \param tcb internal congestion state
     * \param bytesInFlight bytes in flight
     *
     * \return the slow start threshold value
     */
    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;

    /**
     * \brief Implement congestion avoidance for TcpScalableTweak
     *
     * \param tcb internal congestion state
     * \param segmentsAcked count of segments acknowledged
     */
    void CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;

    /**
     * \brief Fork the congestion control object
     *
     * \return a copy of the object
     */
    Ptr<TcpCongestionOps> Fork() override;

  private:
    uint32_t m_ackCnt;   //!< Number of received ACKs
    uint32_t m_aiFactor; //!< Additive increase factor
    double m_mdFactor;   //!< Multiplicative decrease factor
};

} // namespace ns3

#endif // TCPSCALABLETWEAK_H
