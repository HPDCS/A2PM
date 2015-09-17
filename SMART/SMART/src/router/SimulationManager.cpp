/****************************************************************************
 * Panacea Overlay Network v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of PON (Panacea Overlay Network)                       *
 *                                                                          *
 * PON is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/

#include "SimulationManager.h"

SimulationManager::SimulationManager(std::string fileName) : file(fileName){
    rows=0;
    cols=0;
	if (!file.is_open()){
		std::cout << "file open failed" << std::endl;
	}
	endMatrixResult = false;
	inst=1;
	reward=0.0;
}

SimulationManager::~SimulationManager(){
	file.close();
}


int SimulationManager::readNumbers( const std::string & s ) {
	std::istringstream is( s );
    double n;
    while( is >> n ) {
        matrix.push_back( n );
    }
    return matrix.size();
}



int SimulationManager::readResultsFromFile(){
	std::string line;

    int i=0;
    getline(file, line );

    cols =readNumbers( line );
    //std::cout << "cols:" << cols << std::endl;
    //std::cout << "line:" << line << std::endl;


    for ( i=1;i<dim;i++){
    	if ( getline(file, line) == 0 ) {
    		return 0;
    	}
    	readNumbers( line );
    }
    rows=i;
    return 1;
    //std::cout << "rows :" << rows << std::endl;
}

void SimulationManager::writeMatrice(){
	std::cout << "Matrix:" << std::endl;
    for (int i=0;i<rows;i++){
        for (int j=0;j<cols;j++){
        	std::cout << matrix[i*cols+j] << "\t" ;
        }
        std::cout << std::endl;
    }
}


void SimulationManager::readListIP(){
	std::string line;
	std::ifstream in("ip_address");
	if (in.is_open()){
		int dimension = 0;
		while (getline(in, line)) {
			IPv4Address Addr = inet_addr(line.c_str());
			tableIP.push_back(Addr);
			routingManager->index_nodes.push_back(Addr);
			dimension ++;
		}
		dim = dimension;
		in.close();
	}
}

void SimulationManager::readConfigurationFromFile(){
	std::cout << "Reading configuration from files..." << std::endl;
	std::string line;

	std::ifstream in("destinations");
	if (in.is_open()) {
		while (getline(in, line)) {
			// gives the destination address to the routingManager
			routingManager->index_destinations.push_back(inet_addr(line.c_str()));
		}
		in.close();
	}

	in.open("myip");
	if (in.is_open()) {
		while (getline(in, line)) {
			// gives the destination address to the routingManager
			routingManager->index_origin.push_back(inet_addr(line.c_str()));
		}
		in.close();
		//getline(in, line);
	   // origin = inet_addr(line.c_str());
	   // in.close();
	}

}


void SimulationManager::lookUpForResult(){

	PathEdgesResult result;
	int x, y;

	for (size_t i = 0; i< monitoringPacket.pathLength; i++) {
		result.path.push_back(monitoringPacket.path[i]);
	}
	result.metric = monitoringPacket.metric;

	for(int j=0;j<tableIP.size();j++){
		if(tableIP.at(j)==routingManager->origin){
			x=j;
		}
		if(tableIP.at(j)==monitoringPacket.path[0]){
			y=j;
		}
	}
	result.edgesRTT.push_back(matrix[x*cols+y]);

	for(int i=0; i<monitoringPacket.pathLength-1;i++){
		for(int j=0;j<tableIP.size();j++){
			if(tableIP.at(j)==monitoringPacket.path[i]){
				x=j;
			}
			if(tableIP.at(j)==monitoringPacket.path[i+1]){
				y=j;
			}
		}
		result.edgesRTT.push_back(matrix[x*cols+y]);
	}
	this->routingManager->putProbeResults(result);
}


int SimulationManager::run(MetricType metric, std::vector<IPPath> paths){

	//if (readResultsFromFile()!=0){
		//endMatrixResult=false;
		//std::cout << "cols :" << cols << std::endl;
		//std::cout << "rows :" << rows << std::endl;
		//std::cout << "dim :" << dim << std::endl;

		for (const IPPath & path : paths) {

    		monitoringPacket.metric = metric;
    		for (size_t i=0; i< path.size(); i++) {
    			monitoringPacket.path[i] = path.at(i);
    		}
    		monitoringPacket.pathLength = path.size();

    		lookUpForResult();
    	}
		matrix.clear();
   // }else{
    //	endMatrixResult = true;
   // 	return 0;
  //  }
	return 1;

}

double SimulationManager::calculateLatency(MetricType metric, IPPath path){
	double latency, min;
	int x, y;

	if(metric==LATENCY_METRIC){
		for(int j=0;j<tableIP.size();j++){
			if(tableIP.at(j)==routingManager->origin){
				x=j;
			}
			if(tableIP.at(j)==path.at(0)){
				y=j;
			}
		}
		latency = matrix[x*cols+y];

		for(int i=0; i<path.size()-1;i++){
			for(int j=0;j<tableIP.size();j++){
				if(tableIP.at(j)==path.at(i)){
					x=j;
				}
				if(tableIP.at(j)==path.at(i+1)){
					y=j;
				}
			}
			latency += matrix[x*cols+y];
		}
	}else if(metric == BW_METRIC){
		for(int j=0;j<tableIP.size();j++){
			if(tableIP.at(j)==routingManager->origin){
				x=j;
			}
			if(tableIP.at(j)==path.at(0)){
				y=j;
			}
		}
		latency = matrix[x*cols+y];

		for(int i=0; i<path.size()-1;i++){
			for(int j=0;j<tableIP.size();j++){
				if(tableIP.at(j)==path.at(i)){
					x=j;
				}
				if(tableIP.at(j)==path.at(i+1)){
					y=j;
				}
			}

			if(matrix[x*cols+y]<latency){
				latency = matrix[x*cols+y];
			}
		}
	}
	return latency;
}


void SimulationManager::writeResultInFile(std::map<IPv4Address, IPPath> bestPath){

	std::ofstream resultText("simulationResult.txt", std::ios::app);
	std::cout <<  " writeResult " << std::endl;

	if(resultText){

		resultText.precision(10);

		for (std::map< IPv4Address, IPPath>::iterator it = bestPath.begin(); it != bestPath.end(); ++it){

			const IPv4Address theDestination = it->first;
			const IPPath thePath = it->second;


			std::string ori_dest;
			ori_dest=IPToString(routingManager->origin)+"_"+IPToString(theDestination);

			//Length[ori_dest]+=calculateLatency(BW_METRIC, thePath);
			//reward += calculateLatency(BW_METRIC, thePath);


			resultText << ori_dest << "	" ;
			resultText << calculateLatency(BW_METRIC, thePath)  << "		" ;

			for (size_t i=0; i< thePath.size(); i++){
				resultText << IPToString(thePath.at(i)) << " " ;
			}
/*
			resultText << " 	";
			if(thePath.size()==1){
				resultText << "1" ;
			}
			resultText << " 	";
			if(thePath.size()==2){
				resultText << "1" ;
			}
			resultText << " 	";
			if(thePath.size()==3){
				resultText << "1" ;
			}
			resultText << " 	";
			if(thePath.size()==4){
				resultText << "1" ;
			}
			resultText << " 	";
			if(thePath.size()==5){
				resultText << "1" ;
			}
*/
			resultText << std::endl;

		}
		//resultText << "  " << std::endl << std::endl;
		//resultText << reward << std::endl;

	}
}

void SimulationManager::writeLenght(){
	std::ofstream resultText("len.txt", std::ios::app);
	if(resultText){
		resultText.precision(12);

		resultText << reward << std::endl;

	}

}
