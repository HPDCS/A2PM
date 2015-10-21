/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/
#ifndef ROUTINGMANAGEREULER_H
#define ROUTINGMANAGEREULER_H

#include "common/common.h"
#include "RoutingManager.h"
#include "EulerCompleteGraph.h"


class RoutingManagerEuler: public RoutingManagerBase {
public:
    RoutingManagerEuler();
    void init();

    void computePathsToTest(std::vector< IPPath> &pathsToTest) override;

    /**
     * @brief Method called by the path manager when he receive a new latency response
     */
    void putProbeResults(const PathEdgesResult& result) override;

    virtual std::map<IPv4Address, IPPath> getBestPathsFromResults() override;

protected:
    bool initialized;
    std::map < IPv4Address , std::vector< IPPath > > allPaths;
    EulerBellmanFordGraph graph;
    std::map<IPv4Address, int> nodesMapIndex;   /**< contains the hops */
    int packetReceived;
};

#endif // ROUTINGMANAGEREULER_H
