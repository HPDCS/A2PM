#ifndef ROUTINGMANAGER_H
#define ROUTINGMANAGER_H

/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#include "common/common.h"


#include "RoutingManagerInterface.h"
#include "PathManager.h"
#include "SimulationManager.h"


#define PROBE_TIMEOUT_IN_MS 10000 	//! time defined to decide if a connection works or not
#define PROBE_PERIOD_IN_S 30 		//! time between each probe packet



/*!
 * @brief This is pure virtual class used to derive all SMART algorithms
 * @function computeAllPaths this function returns all the paths for each destination
 * @function computePathsToTest this function returns only paths that should be measured for each destination according to the algorithm
 * @function getBestPathsFromResults returns the best Path for each destination
 * @function updateRoutingTable send to the forwarder updates about best found routes for destinations
 * @param pathsToTest it contains the paths we want to measure
 * @param currentRoutes the effective used routes for each destination
 *
 */
class RoutingManagerBase : public RoutingManagerInterface {
public:
    std::map<IPv4Address, std::vector<IPPath> > computeAllPaths();

    /**
     * @brief Manage the whole measurement process
     */
    virtual void run();

    virtual void runSimulation();

    /**
     * @brief Inherited classes must implement this function to compute the paths to test
     * @param paths
     */
    virtual void computePathsToTest(std::vector <IPPath> &paths) = 0;
    virtual std::map<IPv4Address, IPPath> getBestPathsFromResults() = 0;

    /**
     * @brief setLatencyProbeResults
     */
    virtual void putProbeResults(const PathEdgesResult & result) = 0;

    //TODO delete this function (test function)
    std::vector<IPPath> readPathsFromUserCommandLine();

    void writeAllPath(IPv4Address dest, std::vector< IPPath > paths);



protected:
    std::vector< IPPath > pathsToTest; // pathsToTest[dest]
    std::map <IPv4Address , IPRoute> currentRoutes; // currentPaths[dest]
    //std::map<IPv4Address, double> internetDelays;
    //std::map<IPv4Address, double> internetLosses;
    //std::map<IPv4Address, double> panaceaDelays; // measuredDelays[dest]= <delayInternet, delayBestPanacea>
    //std::map<IPv4Address, double> panaceaLosses;


    void updateRoutingTable();
};



#endif // ROUTINGMANAGER_H
