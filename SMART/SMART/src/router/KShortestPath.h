/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#ifndef KSHORTESTPATH_H
#define KSHORTESTPATH_H

#include "common/common.h"
#include "RoutingManager.h"


class Kpath: public RoutingManagerBase{
public:
    Kpath();

    void computePathsToTest(std::vector< IPPath> &pathsToTest) override;

    /**
     * @brief Method called by the path manager when he receive a new latency response
     */
    void putProbeResults(const PathEdgesResult& result) override;


    virtual std::map<IPv4Address, IPPath> getBestPathsFromResults() override;

    std::map<IPv4Address, IPPath> get_K_BestPaths();

    std::map<IPv4Address, IPPath> getBestPathsFrom_K();

protected:
    int K = 5;
    //IPPath P;
    //std::map<IPPath,> S;
   // std::vector<IPPath> X;

    bool allPathsComputed;
    bool AllTest;

    int time;

    std::map < IPv4Address , std::vector< IPPath > > allPaths;
    std::map < IPv4Address , std::vector< IPPath > > pathsTest;
    std::map< IPPath, Result > resulTable;  /**< measurement results */
};


#endif
