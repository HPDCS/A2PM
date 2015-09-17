/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#include "KShortestPath.h"



Kpath::Kpath() {
    allPathsComputed = false;
    time=0;
    AllTest=true;
}

void Kpath::computePathsToTest(std::vector<IPPath> &pathsToTest)
{
    if (!allPathsComputed) {
        allPaths = computeAllPaths();
        allPathsComputed = true;
    }
    pathsToTest.clear();

    if(time>720){
    	AllTest=true;
    }

    time +=2;

    if(AllTest){
    	for (std::map < IPv4Address , std::vector< IPPath > >::iterator it = allPaths.begin(); it!= allPaths.end(); it++) {
    		pathsToTest.insert( pathsToTest.end(), it->second.begin(), it->second.end() );
    	}
    }else{
    	for (std::map < IPv4Address , std::vector< IPPath > >::iterator it = pathsTest.begin(); it!= pathsTest.end(); it++) {
    		pathsToTest.insert( pathsToTest.end(), it->second.begin(), it->second.end() );
    	}
    }
}

void Kpath::putProbeResults(const PathEdgesResult& result) {
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

	}
}


std::map<IPv4Address, IPPath> Kpath::getBestPathsFromResults() {

    std::map<IPv4Address, IPPath> proposedPaths;

    if(AllTest){
    	proposedPaths=get_K_BestPaths();
    	AllTest=false;
    }else{
    	proposedPaths=getBestPathsFrom_K();
    }
    resulTable.clear();
    return proposedPaths;
}

std::map<IPv4Address, IPPath> Kpath::get_K_BestPaths(){
	std::map<IPv4Address, IPPath> proposedPaths;

	for (const IPv4Address dest : index_destinations) {
		double minResult = 99999;
		int NumberMinHop = 4;
		std::map< IPPath, Result>::iterator minPathIter;
		bool found = false;

		for(int i=0; i<K; i++){
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

					}
				}
			}

			for (std::map<IPPath, Result>::iterator ii = resulTable.begin();ii != resulTable.end(); ++ii){
				const IPPath& thepath = ii->first;
				const Result & theresults = ii->second;

				if (dest == thepath.back())
				{
					int NumberHop = thepath.size();
					double result;

					if(theresults.metric==LATENCY_METRIC){
						result = theresults.averageResult;


					}else if(theresults.metric==LOSS_METRIC){
						result = 100* (1.0 - (double) theresults.packetsReceived/ (double) PROBE_PACKETS_TO_SEND);
					}

					if (result==minResult && NumberHop<=NumberMinHop) {
						NumberMinHop = NumberHop;
						minPathIter = ii;
						found = true;
					}
				}
			}
			if (found) {
				proposedPaths[dest] = minPathIter->first;
				resulTable.erase(minPathIter->first);
			}
		}
	}

	return proposedPaths;
}

std::map<IPv4Address, IPPath> Kpath::getBestPathsFrom_K(){
	std::map<IPv4Address, IPPath> proposedPaths;

	for (const IPv4Address dest : index_destinations) {
	        double minResult = 99999;
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
	        		double result;

	        		if(theresults.metric==LATENCY_METRIC){
	        			result = theresults.averageResult;


	        		}else if(theresults.metric==LOSS_METRIC){
	        			result = 100* (1.0 - (double) theresults.packetsReceived/ (double) PROBE_PACKETS_TO_SEND);
	        		}

	        		if (result==minResult && NumberHop<=NumberMinHop) {
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

	return proposedPaths;
}
