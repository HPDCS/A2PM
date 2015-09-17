/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#include "RoutingManagerCPN.h"
#include <cmath>
#include <deque>
#include <algorithm>

RoutingManagerCPN::RoutingManagerCPN(size_t maxPathToTest, double alpha)
{
    initialized = false;
    this->alpha = alpha;
    this->maxPathToTest = maxPathToTest;
}


void RoutingManagerCPN::init() {
    allPaths = computeAllPaths();

    // creates the random neural networks associated with each destination
    for (const IPv4Address dest : index_destinations) {
        latencyRNNs[dest] = RandomNeuralNetwork(allPaths[dest].size());
    }
    std::cout << "Random neural network created for each available path" << std::endl;

    for (std::map < IPv4Address , std::vector< IPPath > >::iterator it = allPaths.begin(); it != allPaths.end(); it++) {
        for (size_t i=0; i<it->second.size(); i++) {
            allPathsId[it->second.at(i)] = i;
        }
    }

    for (const IPv4Address dest : index_destinations) {
        IPRoute directRoute;
        directRoute.push_back(dest);
        currentRoutes[dest] = directRoute;
    }
    initialized = true;
}



void RoutingManagerCPN::putProbeResults(const PathEdgesResult &result)
{
    double totalRTT = result.getTotalRTT();
    //std::cout << "received latencies results : TotalRTT=" << totalRTT << " ms" << std::endl;
    if (totalRTT < PROBE_TIMEOUT_IN_MS) {
        Result& res = resulTable[result.path];
        // update average result
        res.averageResult = (double)(res.averageResult * res.packetsReceived + totalRTT) / (double)(res.packetsReceived +1);
        res.packetsReceived++;
    }
}



std::map<IPv4Address, IPPath> RoutingManagerCPN::getBestPathsFromResults() {

    // get the best paths from the last measurements
    std::cout << "Received " << resulTable.size() << " paths RTT" << std::endl;
    std::cout << "Looking for the best path..." << std::endl;
    std::map<IPv4Address, IPPath> proposedPaths;
    for (const IPv4Address dest : index_destinations) {

        long minLatency = 99999;
        std::map< IPPath, Result>::iterator minPathIter;
        bool found = false;

        for (std::map< IPPath, Result>::iterator ii = resulTable.begin(); ii != resulTable.end(); ++ii)
        {
            const IPPath& thepath = ii->first;
            const Result & theresults = ii->second;

            if (dest == thepath.back()) { // si c'est la bonne destination
                double latency = theresults.averageResult;

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
        }
    }



    // proceed to reinforcement learning, and update RNN accordingly (for next measurements)
    std::cout << "Start reinforcement learning process..." << std::endl;
    for (const IPv4Address dest : index_destinations) {
        for (std::map< IPPath, Result>::iterator ii = resulTable.begin(); ii != resulTable.end(); ++ii)
        {
            const IPPath& thepath = ii->first;
            const Result & theresults = ii->second;

            if (dest == thepath.back()) { // if this is the good destination
                double avgRTT = theresults.averageResult;
                double reward = 1.0 / (avgRTT);

                size_t neuronId = allPathsId[thepath];

                RandomNeuralNetwork & RNN = this->latencyRNNs.at(dest);
                double &threshold = RNN.threshold[neuronId];

                threshold = alpha * threshold + (1.0 - alpha) * reward;
                //std::cout << "expected threshold = " << threshold << std::endl;

                double delta = std::abs((double)(reward - threshold));

                // update positive and negative signals
                if (reward > threshold) {
                    // the reward is greater than the minimum expected
                    for (size_t i = 0; i < RNN.getNumberOfNeurons(); i++) {
                        if (i != neuronId) {
                            RNN.Wplus[i][neuronId] += delta;
                        }
                    }
                    for (size_t i = 0; i < RNN.getNumberOfNeurons(); i++) {
                        for (size_t k = 0; k < RNN.getNumberOfNeurons(); k++) {
                            if (k != neuronId && i !=k) {
                                RNN.Wminus[i][k] += delta / 8.0;
                            }
                        }
                    }

                } else { // the reward is less than the minimum expected
                    for (size_t i = 0; i < RNN.getNumberOfNeurons(); i++) {
                        if (i !=  neuronId) {
                            RNN.Wminus[i][neuronId] += delta;
                        }
                    }
                    for (size_t i = 0; i < RNN.getNumberOfNeurons(); i++) {
                        for (size_t k = 0; k < RNN.Wplus.size(); k++) {
                            if (k !=  neuronId && i != k) {
                                RNN.Wplus[i][k] += delta / 8.0 ;
                            }
                        }
                    }
                }

                //std::cout << "update neurons firing rates for destination " <<IPToString(dest) << " and path(neuron) n°" << neuronId << "..." << std::endl;
                RNN.updateNeuronsFiringRates();
                //std::cout << "update neurons probabilities" << std::endl;
                RNN.updateNeuronsProbabilities();

            }
        }
    }
    std::cout << "Reinforcement learning process finished" << std::endl;

    resulTable.clear();
    return proposedPaths;
}



// utility function and type used to find the k greatest values of a vector (for computePathToTest)
typedef std::pair<int,double> mypair;
bool comparator ( const mypair& l, const mypair& r)
   { return l.second <= r.second; }

void RoutingManagerCPN::computePathsToTest(std::vector<IPPath> &paths)
{
    if (!initialized) {
        init();
    }

    paths.clear();

    bool pickRandom = true;

    std::cout << "Looking for the most excited neurons (neuron := path)" << std::endl;

    for (const IPv4Address dest : index_destinations) {
        std::cout << "For destination " << IPToString(dest) <<  std::endl;
        for (size_t i=0; i < this->latencyRNNs.at(dest).getNumberOfNeurons(); i++ ) {
            std::cout << "q[" << i << "]=" << this->latencyRNNs.at(dest).q[i] << std::endl;
        }

        const std::vector<double> & q = this->latencyRNNs.at(dest).q;

        if (pickRandom) {
            // First version : pick maxPathToTest at random according to their probabilities of being excited

            std::map<int, double> pathsProbasTmp; //pathsProbasTmp[pathId]=probaTemporaire
            double sumQ = 0;
            for (size_t i= 0; i < q.size(); i++) {
                sumQ += q[i];
                //std::cout << "proba path" << K << "=" << pathsProbas[i] << std::endl;
            }

            for (size_t i= 0; i < q.size(); i++) {
                pathsProbasTmp[i] = q[i] / sumQ;
                //std::cout << "proba path" << K << "=" << pathsProbas[i] << std::endl;
            }

            // draw the set of paths to test
            IPPath currentPath = currentRoutes[dest];
            IPPath internetPath;
            internetPath.push_back(dest);
            const std::vector <IPPath> & pathsToDest = allPaths[dest];

            for (size_t n= 0; n < maxPathToTest; n++) {
                double lastElementProba = 0.0;
                if (n==0) { // check internet direct path
                    size_t id = allPathsId[internetPath];
                    lastElementProba = pathsProbasTmp[id];
                    pathsProbasTmp.erase(id);
                    paths.push_back(internetPath);
                    std::cout << "probe path " << id << std::endl;
                    //printIPPath(internetPath);
                } else if (currentPath != internetPath && n==1) { // check current path
                    size_t id = allPathsId[currentPath];
                    lastElementProba = pathsProbasTmp[id];
                    pathsProbasTmp.erase(id);
                    paths.push_back(currentPath);
                    std::cout << "probe path " << id << std::endl;
                    //printIPPath(currentPath);
                } else {
                    double p = ((double) rand() / (RAND_MAX));
                    //std::cout << "draw number=" << p << std::endl;
                    double cumulativeProbability = 0.0;
                    for (std::map<int, double>::iterator it = pathsProbasTmp.begin(); it != pathsProbasTmp.end(); it++) {
                        cumulativeProbability += it->second;
                        if (p <= cumulativeProbability) {
                            paths.push_back(pathsToDest[it->first]);
                            std::cout << "probe path " << it->first << std::endl;
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
            }


        } else {
            // Second version : pick the maxPathToTest most excited neurons (according to their probabilities of being excited)
            std::vector<std::pair<size_t, double> > tmpQ;
            for (size_t i = 0; i< q.size(); i++) {
                tmpQ.push_back(std::make_pair(i, q[i]));
            }
            std::sort(tmpQ.begin(), tmpQ.end(), comparator);
            std::vector<size_t> max_q_ind; // index of the maximum q values (paths that are the most excited)
            for (size_t n = 0; n < maxPathToTest; n++) {
                max_q_ind.push_back(tmpQ[tmpQ.size()-(n+1)].first);
            }
            for (size_t n=0; n < maxPathToTest; n++) {
                std::cout << "max_q_ind[" << n<< "]=" << max_q_ind[n] << std::endl;
                //std::cout << "adding new paths to the list" << std::endl;
                //std::cout << "allPaths[" << IPToString(dest) << "].size() = " << allPaths[dest].size() << std::endl;
                paths.push_back(allPaths[dest][max_q_ind[n]]);
            }
        }

    }
    std::cout << "Found the neurons !" << std::endl;
}

