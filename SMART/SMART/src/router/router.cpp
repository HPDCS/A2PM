/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#include "router.h"

#include "RoutingManagerExhaustive.h"
#include "RoutingManagerEXP3.h"
#include "RoutingManagerEuler.h"
#include "RoutingManagerSPB.h"
#include "RoutingManagerCPN.h"
#include "RNN.h"



Router::Router(std::string askedAlgo, int role, size_t maxPathsToTest, std::string fileName) : simulationManager(fileName), daemon(role){
    if (askedAlgo=="exhaustive") {
        routingManager = new RoutingManagerExhaustive();
        std::cout << "Using Exhaustive probe algorithm" << std::endl;
    } else if (askedAlgo=="euler") {
        routingManager = new RoutingManagerEuler();
        std::cout << "Using Euler Tour based algorithm" << std::endl;
    } else if (askedAlgo=="exp3") {
        routingManager = new RoutingManagerEXP3(maxPathsToTest);
        std::cout << "Using EXP3 probe algorithm" << std::endl;
    } else if (askedAlgo=="spb") {
        routingManager = new RoutingManagerSPB(maxPathsToTest);
        std::cout << "Using SPB (Shortest Path Bandit) probe algorithm" << std::endl;
    } else if (askedAlgo=="rnn") {
            routingManager = new RNN(maxPathsToTest);
            std::cout << "Using RNN probe algorithm" << std::endl;
    } else if (askedAlgo=="cpn") {
        routingManager = new RoutingManagerCPN(maxPathsToTest);
        std::cout << "Using CPN Random Neural Network and Reinforcement Learning algorithm" << std::endl;
    } else {
        std::cerr << askedAlgo << " is not a valid algorithm" << std::endl;
        std::cerr << "Valid algorithms are : " << "'exhaustive', 'euler', 'exp3', 'spb', 'rnn' " << std::endl;
        exit(-1);
    }

}

void Router::run() {
    routingManager->setPathManager(&pathManager);
    daemon.setRoutingManager(routingManager);
    daemon.setPathManager(&pathManager);
    pathManager.setRoutingManager(routingManager);

    // starts the daemon thread (return when the configuration has been received)
    daemonThread = std::thread(&DaemonManager::CUCommunication, &daemon);

    // starts the pathManager threads
    pathManager.start();

    while (daemon.connectCU == false) {
    	std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // wait 3 second while the path manager init all its connections and threads
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // launch the measurement loop
    std::cout << "Starts measurement loop" << std::endl;
    routingManager->run();
}

void Router::simulation() {
	routingManager->setSimulationManager(&simulationManager);
	simulationManager.setRoutingManager(routingManager);

	std::cout << "Simulation running..." << std::endl;
	simulationManager.readListIP();
	simulationManager.readConfigurationFromFile();
	routingManager->runSimulation();
}

Router::~Router() {
    delete routingManager;
}
