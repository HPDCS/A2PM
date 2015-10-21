/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#include "RoutingManagerSPB.h"
#include "EulerCompleteGraph.h"


RoutingManagerSPB::RoutingManagerSPB(size_t maxPathToTest, double gamma, double beta, double eta) {
    isInitialized = false;
    this->firstRun = true;
    this->gamma=gamma;
    this->beta = beta;
    this->eta = eta;
    this->maxPathToTest = maxPathToTest;
}

void RoutingManagerSPB::computePathsToTest(std::vector<IPPath> &pathsToTest) {

    if (!isInitialized) {
        init();
        isInitialized = true;
        std::cout << "SPB initialized successfully !" << std::endl;
    }

    pathsToTest.clear();

    if (firstRun) {
        for (const IPv4Address dest : index_destinations) {
            const std::vector< IPPath > & pathsToDest = allPaths.at(dest);
            allPathsWeigths[dest] = std::vector<double> (pathsToDest.size(), 1.0); // initialize all weigths to 1
            allPathsProba[dest] = std::vector<double> (pathsToDest.size(), 1.0/(double)pathsToDest.size()); // initialize probabilities
            allLinksProba[dest] = std::vector<double> (allLinks.size(), 1.0);
            allLinksWeigths[dest] = std::vector<double> (allLinks.size(), 1.0);
            allTotalPathsWeigths[dest] = pathsToDest.size();
        }
        firstRun = false;
    }

    for (const IPv4Address dest : index_destinations) {
        //std::cout << "dest=" << ip_to_string(dest) << std::endl;
        const std::vector< IPPath > & pathsToDest = allPaths.at(dest);
        std::vector< double > & pathsProbas = allPathsProba[dest];
        std::vector< double > & pathsWeights = allPathsWeigths[dest];
        double & totalPathsWeights = allTotalPathsWeigths[dest];

        size_t K= pathsToDest.size();

        // compute path probabilities
        std::cout << "compute paths probabilities, C.size()=" << C.size() << std::endl;
        for (std::vector<size_t>::iterator it = inC[dest].begin(); it != inC[dest].end(); it++) {
            pathsProbas[*it] = (1.0-gamma) * pathsWeights[*it] / totalPathsWeights + gamma/(double) C.size();
            std::cout << "pathProbaInC[" <<*it<<"]=" << pathsProbas[*it] << std::endl;
        }
        for (std::vector<size_t>::iterator it = notInC[dest].begin(); it != notInC[dest].end(); it++) {
            pathsProbas[*it] = (1.0-gamma) * pathsWeights[*it] / totalPathsWeights;
            std::cout << "pathProbanotInC[" <<*it<<"]=" << pathsProbas[*it] << std::endl;
        }


        // compute edge probabilities
        // this part requires optimization
        std::cout << "compute edges probabilities" << std::endl;
        for (size_t i = 0; i < allLinks.size(); i++) {
            const Link & l = allLinks.at(i);
            double sum = 0.0;
            for (size_t p=0; p< K; p++) {
                const IPPath & path = pathsToDest[p];
                if ( (path.size()!= 0 && l.src == myMonitorIP && l.dst == path[0]) || linkBelongToPath(pathsToDest[p], l.src, l.dst)) {
                    sum += pathsProbas[p];
                }
            }
            allLinksProba[dest][i] = sum;
        }

        // draw the paths to test
        std::map<int, double> pathsProbasTmp; //pathsProbasTmp[pathId]=temporaryProbability for next draw
        for (size_t i= 0; i < K; i++) {
            pathsProbasTmp[i] = pathsProbas[i];
            std::cout << "pathProba[" <<i<<"]=" << pathsProbasTmp[i] << std::endl;
        }

        IPPath currentPath = currentRoutes[dest];
        IPPath internetPath;
        internetPath.push_back(dest);

        for (size_t n= 0; n < maxPathToTest; n++) {

            double lastElementProba = 0.0;
            if (n==0) { // direct path {
                size_t id = allPathsId[internetPath];
                lastElementProba = pathsProbasTmp[id];
                pathsProbasTmp.erase(id);
                pathsToTest.push_back(internetPath);
                monitoredPaths[dest].push_back(id);
                //printIPPath(internetPath);

            } else if (currentPath != internetPath && n==1) { // current path
                size_t id = allPathsId[currentPath];
                lastElementProba = pathsProbasTmp[id];
                pathsProbasTmp.erase(id);
                pathsToTest.push_back(currentPath);
                monitoredPaths[dest].push_back(id);
                //printIPPath(currentPath);
            } else {
                double p = ((double) rand() / (RAND_MAX));
                //std::cout << "draw number=" << p << std::endl;
                double cumulativeProbability = 0.0;

                for (std::map<int, double>::iterator it = pathsProbasTmp.begin(); it != pathsProbasTmp.end(); it++) {
                    cumulativeProbability += it->second;
                    //std::cout << " p=" << p << " , cumulativeProba = " << cumulativeProbability<<  std::endl;
                    if (p <= cumulativeProbability) {
                        IPPath thepath =  pathsToDest[it->first];
                        pathsToTest.push_back(thepath);
                        monitoredPaths[dest].push_back(allPathsId[thepath]);
                        //printIPPath(pathsToDest[it->first]);
                        lastElementProba = it->second;
                        pathsProbasTmp.erase(it);
                        break;
                    }
                }
            }
            // update probabilities for next draw
            for (std::map<int, double>::iterator it = pathsProbasTmp.begin(); it != pathsProbasTmp.end(); it++) {
                it->second = it->second / (1.0-lastElementProba);
            }

            std::cout << "n=" << n << " size=" << pathsToTest.size() << std::endl;
        }
    }
}

void RoutingManagerSPB::putProbeResults(const PathEdgesResult &result){

    double totalRTT = result.getTotalRTT();
    IPv4Address dest = result.path.back();

    if (totalRTT < PROBE_TIMEOUT_IN_MS) {

        //std::cout << "received latencies results : TotalRTT=" << totalRTT << " ms" << std::endl;
        /////////////////////////////////////////////////////////
        /// udpdate complete path result (looking in resultTable)
        /////////////////////////////////////////////////////////
        Result* res = &resulTable[result.path];
        // update average value for this path
        res->averageResult = (double)(res->averageResult * res->packetsReceived + totalRTT) / (double)(res->packetsReceived +1);
        res->packetsReceived++;

        //////////////////////////
        /// Update link results //
        //////////////////////////
        IPv4Address u,v;

        // visit the first edge (starting for the current monitor)
        u = myMonitorIP;
        v = result.path[0];
        double RTT = result.edgesRTT[0];

        // update average value for link (u,v)
        res = &latenciesGraph[dest][Link(u,v)];
        res->averageResult = (double)(res->averageResult * res->packetsReceived + RTT /2.0) / (double)(res->packetsReceived +1);
        res->packetsReceived++;

        // update average value for link (v,u)
        res = &latenciesGraph[dest][Link(v,u)];
        res->averageResult = (double)(res->averageResult * res->packetsReceived + RTT /2.0) / (double)(res->packetsReceived +1);
        res->packetsReceived++;

        // visit the other edges
        for (size_t i = 1; i < result.edgesRTT.size(); i++){
            u = result.path[i-1];
            v = result.path[i];
            double RTT = result.edgesRTT[i];

            // update average value for link (u,v)
            Result* res = &latenciesGraph[dest][Link(u,v)];
            res->averageResult = (double)(res->averageResult * res->packetsReceived + RTT /2.0) / (double)(res->packetsReceived +1);
            res->packetsReceived++;

            //update average value for link (v,u)
            res = &latenciesGraph[dest][Link(v,u)];
            res->averageResult = (double)(res->averageResult * res->packetsReceived + RTT /2.0) / (double)(res->packetsReceived +1);
            res->packetsReceived++;
        }
    }

}

std::map<IPv4Address, IPPath> RoutingManagerSPB::getBestPathsFromResults() {
    std::map<IPv4Address, IPPath> proposedPaths;

    // looking for the best path among the tested ones, for each destination
    for (const IPv4Address dest : index_destinations) {
        long minLatency = 99999;
        std::map< IPPath, Result>::iterator minPathIter;
        bool found = false;

        for (std::map< IPPath, Result>::iterator ii = resulTable.begin(); ii != resulTable.end(); ++ii)
        {
            const IPPath& thepath = ii->first;
            const Result & theresults = ii->second;
            if (dest == thepath.back()) { // if this path is headed to the right destination
                double latency = theresults.averageResult;
                //double losses = 100 * (1.0 - (double)theresults.packetsReceived / (double)PROBE_PACKETS_TO_SEND);
                if (latency < minLatency) {
                    minLatency = latency;
                    minPathIter = ii;
                    found = true;
                }
            }
        }
        if (found) {
            IPPath path = minPathIter->first;
            proposedPaths[dest] = path;
            size_t pathId = allPathsId[path];

            const std::vector< IPPath > & pathsToDest = allPaths.at(dest);
            size_t K = pathsToDest.size();

            std::vector< double > & pathsProbas = allPathsProba[dest];
            std::vector< double > & pathsWeights = allPathsWeigths[dest];
            double reward = 1.0/ minLatency;
            //double reward = (double)(PROBE_TIMEOUT_IN_MS - minLatency) / (double) PROBE_TIMEOUT_IN_MS;
            pathsWeights[pathId] = pathsWeights[pathId] * exp( gamma * reward / (pathsProbas[pathId]*K));
        }


        // estimated gain computation
        std::vector<double> estimatedLinksGain(allLinks.size());
        for (size_t i = 0; i < allLinks.size(); i++) {
            estimatedLinksGain[i] = beta/ allLinksProba[dest][i];
        }
        for (size_t p = 0; p < monitoredPaths[dest].size(); p++){
            IPPath path = allPaths[dest].at(p);
            //for (IPPath path : monitoredPaths) {
            if (path.size() != 0) {
                Link l(myMonitorIP, path.at(0));
                size_t linkId = allLinksId[l];
                estimatedLinksGain[linkId] += (1.0/latenciesGraph[dest][l].averageResult) /allLinksProba[dest][linkId];
                for (size_t i = 0; i < path.size()-1; i++) {
                    l.src = path[i];
                    l.dst = path[i+1];
                    linkId = allLinksId[l];
                    estimatedLinksGain[linkId] += (1.0/latenciesGraph[dest][l].averageResult) /allLinksProba[dest][linkId];
                }
            }
        }


        // update link weigths
        std::vector<double> & linkWeigths = allLinksWeigths[dest];
        for (size_t i = 0; i < allLinks.size(); i++) {
            linkWeigths[i] *= exp( eta *  estimatedLinksGain[i]);
        }

        // update paths weigths and total paths weigths
        std::vector<double> & pathsWeights = allPathsWeigths[dest];
        const std::vector< IPPath > & pathsToDest = allPaths.at(dest);
        double & totalPathWeigth = allTotalPathsWeigths[dest];
        totalPathWeigth = 0.0;
        size_t K = pathsToDest.size();
        for (size_t k = 0; k < K; k++) {
            double pathGain = 0.0;
            const IPPath & path = pathsToDest[k];
            Link l(myMonitorIP, path.at(0));
            size_t linkId = allLinksId[l];
            pathGain += estimatedLinksGain[linkId];
            for (size_t i = 0; i < path.size()-1; i++) {
                l.src = path[i];
                l.dst = path[i+1];
                linkId = allLinksId[l];
                pathGain += estimatedLinksGain[linkId];
            }
            pathsWeights[k] *= exp(eta * pathGain);
            totalPathWeigth += pathsWeights[k];
        }
    }

    resulTable.clear();
    monitoredPaths.clear();
    latenciesGraph.clear();
    return proposedPaths;
}

bool RoutingManagerSPB::linkBelongToPath(const IPPath &path, IPv4Address src, IPv4Address dst) {
    for (size_t i =0; i< path.size()-1; i++){
        if (path.at(i) == src && path.at(i+1) == dst) {
            return true;
        }
    }
    return false;
}

void RoutingManagerSPB::init() {
    // construct the set C of covering paths
    int taille = index_nodes.size()+1;
    int maxHop = DATA_MAX_PATH_LENGTH;

    allPaths = computeAllPaths();

    std::set<std::vector<int> > eulerPaths = EulerPathGenerator::getCoveringPaths(taille, maxHop);

    const IPVector & nodes = index_nodes;
    for (std::set<std::vector<int> >::iterator it = eulerPaths.begin(); it != eulerPaths.end(); it++) {
        IPPath path;
        // start at 1, we must not copy the source in the path
        for (size_t i = 1; i < it->size(); i++) {
            path.push_back(nodes.at(it->at(i)-1));
        }
        C.push_back(path);
    }
    // done construction of C


    nodesMapIndex[this->myMonitorIP] = 0;
    for (size_t i = 0; i< nodes.size(); i++) {
        nodesMapIndex[nodes[i]]=i+1;
    }

    // construct the set of edges
    allLinks.clear();
    for (size_t i = 0; i< nodes.size(); i++) {
        Link l(this->myMonitorIP, nodes[i]);
        allLinksId[l] = allLinks.size();
        allLinks.push_back(l);
    }

    for (size_t i = 0; i< nodes.size(); i++) {
        for (size_t j = 0; j< nodes.size(); j++) {
            if (i!=j) {
                Link l(nodes[i], nodes[j]);
                allLinksId[l] = allLinks.size();
                allLinks.push_back(l);
            }
        }
    }

    for (std::map < IPv4Address , std::vector< IPPath > >::iterator it = allPaths.begin(); it != allPaths.end(); it++) {
        //std::cout << "destination=" << ip_to_string(it->first) << std::endl;
        IPv4Address dest= it->first;

        for (size_t i=0; i<it->second.size(); i++) {
            //printIPPath(it->second.at(i));
            const IPPath& path = it->second.at(i);
            allPathsId[path] = i;

            if (find(C.begin(), C.end(), path) != C.end()) {
                inC[dest].push_back(i);
            } else {
                notInC[dest].push_back(i);
            }
        }
    }
}



