/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#include "RoutingManagerEXP3.h"
#include <math.h>

RoutingManagerEXP3::RoutingManagerEXP3(size_t maxPathToTest, double gamma) {
    allPathsComputed = false;
    firstRunEXP3 = true;
    this->gamma = gamma;
    this->maxPathToTest = maxPathToTest;
}


void RoutingManagerEXP3::computePathsToTest(std::vector< IPPath> &pathsToTest) {
    if (!allPathsComputed) {
        allPaths = computeAllPaths();

       allPathsComputed = true;
        for (std::map < IPv4Address , std::vector< IPPath > >::iterator it = allPaths.begin(); it != allPaths.end(); it++) {
            for (size_t i=0; i<it->second.size(); i++) {
                allPathsId[it->second.at(i)] = i;
            }
        }
    }
    pathsToTest.clear();

    if (firstRunEXP3) {
        for (const IPv4Address dest : index_destinations) {
        	if(dest != origin){
        		const std::vector< IPPath > & pathsToDest = allPaths.at(dest);
        		if(allPathsWeigths.find(dest)==allPathsWeigths.end()){
        			allPathsWeigths[dest] = std::vector<double> (pathsToDest.size(), 1.0); // initilialize all weigths to 1
        			allPathsProba[dest] = std::vector<double> (pathsToDest.size(), 1.0/(double)pathsToDest.size()); // init paths probabilities
        		}
        	}
        }
        firstRunEXP3 = false;
    }

    for (const IPv4Address dest : index_destinations) {
    	if(dest != origin){
    		const std::vector< IPPath > & pathsToDest = allPaths.at(dest);
    		        std::vector< double > & pathsProbas = allPathsProba[dest];
    		        std::vector< double > & pathsWeights = allPathsWeigths[dest];
    		        size_t K= pathsToDest.size();

    		        double sumWeigth= 0.0;
    		        for (size_t i= 0; i < K; i++) {
    		            sumWeigth += pathsWeights[i];
    		        }

    		        std::map<int, double> pathsProbasTmp; //pathsProbasTmp[pathId]=probaTemporaire
    		        for (size_t i= 0; i < K; i++) {
    		            pathsProbas[i] = (1.0-gamma) * pathsWeights[i] / sumWeigth + gamma/(double)K;
    		            pathsProbasTmp[i] = pathsProbas[i];
    		            //std::cout << "proba path" << K << "=" << pathsProbas[i] << std::endl;
    		        }

    		        // draw the set of paths to probe

    		        IPPath currentPath = currentRoutes[dest];
    		        IPPath internetPath;
    		        internetPath.push_back(dest);

    		        for (size_t n= 0; n <= maxPathToTest; n++) {

    		            double lastElementProba = 0.0;
    		            if (n==0) { // direct path
    		                size_t id = allPathsId[internetPath];
    		                lastElementProba = pathsProbasTmp[id];
    		                pathsProbasTmp.erase(id);
    		                pathsToTest.push_back(internetPath);

    		            } else  if (currentPath != internetPath && n==1) { // current path
    		                size_t id = allPathsId[currentPath];
    		                lastElementProba = pathsProbasTmp[id];
    		                pathsProbasTmp.erase(id);
    		                pathsToTest.push_back(currentPath);
    		            } else {
    		                double p = ((double) rand() / (RAND_MAX));
    		                //std::cout << "draw number=" << p << std::endl;
    		                double cumulativeProbability = 0.0;

    		                for (std::map<int, double>::iterator it = pathsProbasTmp.begin(); it != pathsProbasTmp.end(); it++) {
    		                    cumulativeProbability += it->second;
    		                    if (p <= cumulativeProbability) {
    		                        pathsToTest.push_back(pathsToDest[it->first]);
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
    		    }
    	}
}


void RoutingManagerEXP3::putProbeResults(const PathEdgesResult& result) {
	if (result.metric == LATENCY_METRIC){
		double totalRTT = result.getTotalRTT();
		//std::cout << "received latencies results : TotalRTT=" << totalRTT << " ms" << std::endl;
		if (totalRTT < PROBE_TIMEOUT_IN_MS) {
			Result& res = resulTable[result.path];
			// This was added to be sure the metric type is passed
			res.metric = result.metric;
			// update average value
			res.averageResult = (double)(res.averageResult * res.packetsReceived + totalRTT) / (double)(res.packetsReceived +1);
			res.packetsReceived++;
			if(result.path.size()>1){
				res.averageResult = res.averageResult + 3;
			}
		}
	}else if (result.metric == LOSS_METRIC){

	}else if (result.metric == BW_METRIC){
		double min = 9999;
		Result& res = resulTable[result.path];
		// This was added to be sure the metric type is passed
		res.metric = result.metric;
		// update average value
		for(size_t i=0; i<result.edgesRTT.size();i++){
			if(result.edgesRTT.at(i)<min){
				min=result.edgesRTT.at(i);
			}
		}
		res.averageResult = min;
		res.packetsReceived++;
	}
}



std::map<IPv4Address, IPPath> RoutingManagerEXP3::getBestPathsFromResults() {
    std::map<IPv4Address, IPPath> proposedPaths;
    for (const IPv4Address dest : index_destinations) {

        double minResult = 99999;
        double maxResult = 0;
        int NumberMinHop = 4;
        std::map< IPPath, Result>::iterator minPathIter;
        bool found = false;

        for (std::map< IPPath, Result>::iterator ii = resulTable.begin(); ii != resulTable.end(); ++ii)
        {
            const IPPath& thepath = ii->first;
            const Result & theresults = ii->second;

            if (dest == thepath.back()) { // if this path is headed to the right destination

            	if(theresults.metric==LATENCY_METRIC){
            		double latency = theresults.averageResult;

            		if (latency < minResult) {
            			minResult = latency;
            		}
            	}else if(theresults.metric==LOSS_METRIC){
            		double losses = 100 * (1.0 - (double)theresults.packetsReceived / (double)PROBE_PACKETS_TO_SEND);

            	}else if (theresults.metric == BW_METRIC){
            		double BW = theresults.averageResult;

            		if (BW > maxResult) {
            			maxResult = BW;
            		}
            	}
            }
        }

        for (std::map<IPPath, Result>::iterator ii = resulTable.begin();ii != resulTable.end(); ++ii)
        {
        	const IPPath& thepath = ii->first;
        	const Result & theresults = ii->second;

        	if (dest == thepath.back())
        	{
        		int NumberHop = thepath.size();
        		double result = 0;
        		double BW = 1;

        		if(theresults.metric==LATENCY_METRIC){
        			result = theresults.averageResult;
        		}else if(theresults.metric==LOSS_METRIC){
        			result = 100* (1.0 - (double) theresults.packetsReceived/ (double) PROBE_PACKETS_TO_SEND);
        		}else if (theresults.metric == BW_METRIC){
        			BW = theresults.averageResult;
            	}

        		if (result == minResult && NumberHop<=NumberMinHop) {
        			NumberMinHop = NumberHop;
        			minPathIter = ii;
        			found = true;
        		}

        		if (BW == maxResult && NumberHop<=NumberMinHop) {
        			NumberMinHop = NumberHop;
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

            double reward;
            if(minPathIter->second.metric==LATENCY_METRIC){
            	reward = 1.0/ minResult;
            }else if(minPathIter->second.metric==LOSS_METRIC){
            	reward = (double)(PROBE_TIMEOUT_IN_MS - minResult) / (double) PROBE_TIMEOUT_IN_MS;
            }else if (minPathIter->second.metric == BW_METRIC){
            	reward = 1.0-(1.0/ minResult);
        	}
            pathsWeights[pathId] = pathsWeights[pathId] * exp( gamma * reward / (pathsProbas[pathId]*K));
        }
    }
    resulTable.clear();
    return proposedPaths;
}

