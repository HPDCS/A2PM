/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#include "RoutingManagerExhaustive.h"


RoutingManagerExhaustive::RoutingManagerExhaustive() {
    allPathsComputed = false;
}

void RoutingManagerExhaustive::computePathsToTest(std::vector<IPPath> &pathsToTest)
{
    //if (!allPathsComputed) {
        allPaths = computeAllPaths();
    //    allPathsComputed = true;
    //}

    pathsToTest.clear();
    for (std::map < IPv4Address , std::vector< IPPath > >::iterator it = allPaths.begin(); it!= allPaths.end(); it++) {
        pathsToTest.insert( pathsToTest.end(), it->second.begin(), it->second.end() );
    }
}

void RoutingManagerExhaustive::putProbeResults(const PathEdgesResult& result) {
	if (result.metric == LATENCY_METRIC){
		double totalRTT = result.getTotalRTT();
		//std::cout << "received latencies results : TotalRTT=" << totalRTT << " ms" << std::endl;
		if (totalRTT < PROBE_TIMEOUT_IN_MS) {
			Result& res = resulTable[result.path];

			// This was added to be sure the metric type is passed
			res.metric = result.metric;

			// update average value for this path
			res.averageResult = (double)(res.averageResult * res.packetsReceived + totalRTT) / (double)(res.packetsReceived +1);
			res.packetsReceived++;
		}
	}else if (result.metric == LOSS_METRIC){

	}else if (result.metric == BW_METRIC){
		double min = 9999;
		Result& res = resulTable[result.path];
		// This was added to be sure the metric type is passed
		res.metric = result.metric;
		// update average value
		for(size_t i=0; i<result.edgesRTT.size();i++){
			if(result.edgesRTT.at(i)< min){
				min=result.edgesRTT.at(i);
			}
		}
		res.averageResult = min;
		res.packetsReceived++;
	}
}


std::map<IPv4Address, IPPath> RoutingManagerExhaustive::getBestPathsFromResults() {

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

            if (dest == thepath.back()) {

            	if(theresults.metric==LATENCY_METRIC){
                    double latency = theresults.averageResult;

                    if (latency < minResult) {
                        minResult = latency;
                    }
            	}else if(theresults.metric==LOSS_METRIC){
            		double losses = 100*(1.0- (double) theresults.packetsReceived/ (double) PROBE_PACKETS_TO_SEND);

            		if (losses < minResult) {
            			minResult = losses;
            		}
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

        		if (result==minResult && NumberHop<=NumberMinHop) {
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
            proposedPaths[dest] = minPathIter->first;
        }
    }
    resulTable.clear();
    return proposedPaths;
}


