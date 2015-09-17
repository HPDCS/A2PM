/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

/**
 * !!!!!!!!!!!!!This Probe is not finished!!!!!!!!!!!!!!!!!!
 */

#ifndef LOSSPROBE_H
#define LOSSPROBE_H

#include "monitor.h"

#define LATENCY_PROBE_PORT 9547				//! PORT used for the transfer of latency probe packets

//! header of a probe packet
struct LossProbeHeader {
    unsigned int pathLength :4;
    unsigned int nextHop :4;
    unsigned int nextStamp :4;
    unsigned int direction :4; // go or return
    IPv4Address path[DATA_MAX_PATH_LENGTH + 1]; // +1 for the source address
    int lossResult[];
};


/**
 * @brief The probe packet structure
 */
class LossProbePacket : public Packet {
public :
        LossProbePacket();

        /**
         * @brief Get a pointer to the ProbeHeader
         * @return
         */
        struct LossProbeHeader * getLossProbeHeader() const;
};


class ProbeLoss;

/**
 * @brief Forwarder dedicated to loss measurement.
 * Implements the ForwarderInterface
 */
class ProbeLossForwarder : public ForwarderInterface {
public :
    ProbeLossForwarder();

    /**
     * @brief Loop waiting for packets to forward.
     * Forward the packets according to their state.
     */
    void process();

    /**
     * @brief Set the loss probe agent associated with this forwarder
     * @param agent pointer to the loss probe agent
     */
    void setProbeAgent(ProbeLoss* agent) {this->agent = agent;}

private:
    //! Stores the current packet
    LossProbePacket packet;

    /**
     * @brief pointer to the loss probe agent
     */
    ProbeLoss* agent;
};


/**
 * @brief Probe agent dedicated to loss measurement
 * Implements the ProbeInterface
 */
class ProbeLoss : public ProbeInterface {
public:
    ProbeLoss(MonitorManager *  monitorManager);

    /**
     * @brief Starts the forwarder thread
     */
    virtual void startForwarder();
    virtual void doMonitorPaths(const std::vector<IPPath> & routes);

    /**
     * @brief Send the measurements results to the monitorManager
     * @param route
     * @param loss
     */
    void sendResultsToMonitorManager(const IPPath & route, std::vector<double> loss);

private:
    /**
     * @brief The loss forwarder agent
     */
    ProbeLossForwarder forwarder;

    /**
     * @brief the thread that runs the forwarder
     */
    std::thread forwarderThread;

    /**
     * @brief Pointer to the MonitorManager
     */
    MonitorManager * monitorManager;

    /**
     * @brief Socket used to send the probe packets
     */
    int socketDescription;
};


#endif
