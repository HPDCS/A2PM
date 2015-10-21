/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#include "RoutingManagerEuler.h"


RoutingManagerEuler::RoutingManagerEuler() {
    initialized = false;

}

void RoutingManagerEuler::init() {
    allPaths = computeAllPaths();
    const IPVector & nodes = index_nodes;
    nodesMapIndex[myMonitorIP] = 0;
    for (size_t i = 0; i< nodes.size(); i++) {
        nodesMapIndex[nodes[i]]=i+1;
    }
    graph = EulerBellmanFordGraph(nodes.size()+1);
    initialized = true;
}


void RoutingManagerEuler::computePathsToTest(std::vector< IPPath> &pathsToTest) {
    if (!initialized) {
        init();
    }

    int taille = index_nodes.size()+1;
    int maxHop = DATA_MAX_PATH_LENGTH;

    std::set<std::vector<int> > eulerPaths = EulerPathGenerator::getCoveringPaths(taille, maxHop);

    pathsToTest.clear();
    const IPVector & nodes = index_nodes;
    for (std::set<std::vector<int> >::iterator it = eulerPaths.begin(); it != eulerPaths.end(); it++) {
        IPPath path;
        // starts at 1 : we must not copy the source in the path
        for (size_t i = 1; i < it->size(); i++) {
            path.push_back(nodes.at(it->at(i)-1));
        }
        pathsToTest.push_back(path);
    }
}

void RoutingManagerEuler::putProbeResults(const PathEdgesResult& result) {
	if (result.metric == LATENCY_METRIC){
		double totalRTT = result.getTotalRTT();
		std::cout << "received latencies results : TotalRTT=" << totalRTT << " ms" << std::endl;
		if (totalRTT < PROBE_TIMEOUT_IN_MS) {
			IPv4Address u,v;
			u = myMonitorIP;
			v = result.path[0];
			double RTT = result.edgesRTT[0];
			graph.setEdgeValue(nodesMapIndex[u], nodesMapIndex[v], RTT/2.0);
			graph.setEdgeValue(nodesMapIndex[v], nodesMapIndex[u], RTT/2.0);
			for (size_t i = 1; i < result.edgesRTT.size(); i++){
				u = result.path[i-1];
				v = result.path[i];
				double RTT = result.edgesRTT[i];
				graph.setEdgeValue(nodesMapIndex[u], nodesMapIndex[v], RTT/2.0);
				graph.setEdgeValue(nodesMapIndex[v], nodesMapIndex[u], RTT/2.0);
			}
		}
	}else if (result.metric == LOSS_METRIC){

	}else if (result.metric == BW_METRIC){

	}
}

std::map<IPv4Address, IPPath> RoutingManagerEuler::getBestPathsFromResults() {
    std::map<IPv4Address, IPPath> proposedPaths;

    std::cout << "Launch Bellman Ford algorithm" << std::endl;
    std::vector< std::deque < int> >  shortestPaths = graph.BellmanFord(0, DATA_MAX_PATH_LENGTH);
    std::cout << "Bellman ford done !" << std::endl;

    const IPVector & indexNodes = index_nodes;
	for (const IPv4Address dest : index_destinations) {
		for (size_t i = 0; i < indexNodes.size(); i++) {
			if (indexNodes.at(i) == dest) {
				const std::deque<int> & path = shortestPaths.at(i + 1);
				if (path.size() != 0) {
					IPPath newPath;
					for (size_t j = 1; j < path.size(); j++) {
						newPath.push_back(indexNodes.at(path[j] - 1));
					}
					proposedPaths[indexNodes[i]] = newPath;
				}
			}
		}
	}

    graph.resetGraphValues();
    packetReceived = 0;
    return proposedPaths;
}
