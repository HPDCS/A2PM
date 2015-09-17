/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/
#ifndef ROUTER_H
#define ROUTER_H


#include "common/common.h"

#include "daemon.h"
#include "PathManager.h"
#include "RoutingManager.h"
#include "SimulationManager.h"

/*!
 * @brief Main router class. It has two modes, normal on a network (run) and simulation
 * @param daemon it communicates with CU in order to update configuration
 * @param pathManager this entity communicates with Forwarder and Monitor
 * @param routingManager this entity communicates with algorithms (SMART intelligence) to make all kind of calculus.
 * @param simulationManager only in simulation mode it takes care of simulation entities.
 */
class Router {
public:
	// 1 for master (the same as graphCom)
    Router(std::string askedAlgo = "euler", int role = 1, size_t maxPathsToTest = 5, std::string fileName = " ");
    void run() ;
    void simulation();

    ~Router();

private:
    DaemonManager daemon;
    std::thread daemonThread;
    PathManager pathManager;
    RoutingManagerInterface * routingManager;
    SimulationManager simulationManager;
};


#endif
