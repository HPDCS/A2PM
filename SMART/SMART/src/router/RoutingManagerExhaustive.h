/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#ifndef ROUTINGMANAGEREXHAUSTIVE_H
#define ROUTINGMANAGEREXHAUSTIVE_H


#include "RoutingManager.h"

class RoutingManagerExhaustive : public RoutingManagerBase {
public:
    RoutingManagerExhaustive();

    virtual void computePathsToTest(std::vector< IPPath> &pathsToTest);

    /**
     * @brief Method called by the path manager when he receive a new latency response
     */
    virtual void putProbeResults(const PathEdgesResult& result);


    virtual std::map<IPv4Address, IPPath> getBestPathsFromResults() override;

    void printLossResult();

protected:
    bool allPathsComputed;
    std::map < IPv4Address , std::vector< IPPath > > allPaths;
    std::map< IPPath, Result > resulTable;  /**< measurement results */
};

#endif // ROUTINGMANAGEREXHAUSTIVE_H
