/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/
#include "LossProbe.h"


LossProbePacket::LossProbePacket() : Packet() {
}

/*!
 * The load of the probe packet is entirely defined by the structure
 * probhdr, so the attribute probeheader will point to the beginning of the message.
 */
struct LossProbeHeader * LossProbePacket::getLossProbeHeader() const {
    return (LossProbeHeader *) (msg);
}





ProbeLoss::ProbeLoss(MonitorManager * monitorManager) : ProbeInterface(LOSS_METRIC)
{
    this->monitorManager = monitorManager;
    forwarder.setProbeAgent(this);

    socketDescription = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketDescription == -1) {
        std::cout << "Error creating UDP socket\n" << std::endl;
        exit(1);
    }
}

void ProbeLoss::startForwarder()
{
    // start the forwarder process() function in a separated thread
    forwarderThread = std::thread(&ProbeLossForwarder::process, forwarder);
}

void ProbeLoss::doMonitorPaths(const std::vector<IPPath> &routes)
{
    LossProbePacket packet;

    struct sockaddr_in add;   /**< the address */
    bzero(&add, sizeof(add));
    add.sin_family = AF_INET;
    add.sin_port = htons(LATENCY_PROBE_PORT);

    for (const IPPath& route : routes) {
        memset(packet.msg, 0, BUFLENGTH);
        int pathLength = route.size() + 1;
        LossProbeHeader* header = packet.getLossProbeHeader();

        // copy the route to the network packet
        header->pathLength = pathLength;
        header->path[0] = monitorManager->getMyAddr();
        for (size_t ii = 0; ii < route.size(); ii++) {
            header->path[ii+1] = route[ii];
        }

        // send PROBE_PACKETS_TO_SEND probe packets
        header->direction = 0;
        header->nextHop = 1;
        header->nextStamp = 2;
        add.sin_addr.s_addr = header->path[header->nextHop];
        for (int j = 0; j < PROBE_PACKETS_TO_SEND; j++) {
            //struct timeval start;
            //gettimeofday(&start, NULL);
            //header->lossResult[0] = start;

            if (sendto(socketDescription, (char*) packet.msg, sizeof(LossProbeHeader), 0, (struct sockaddr*) &add,  sizeof(add)) < 0) {
                perror("failed to send the loss probe packet");
            }
        }
    }
}

void ProbeLoss::sendResultsToMonitorManager(const IPPath &path, std::vector<double> loss)
{
	monitorManager->putProbeResults(LOSS_METRIC, path, loss);
}


// Creates a UDP socket on port 'PROBE_PORT'
ProbeLossForwarder::ProbeLossForwarder() : ForwarderInterface(LATENCY_PROBE_PORT)
{
}


void ProbeLossForwarder::process()
{
    struct sockaddr_in nextForwarderAddr;
    bzero(&nextForwarderAddr, sizeof(sockaddr));
    nextForwarderAddr.sin_family = AF_INET;
    nextForwarderAddr.sin_port = htons(LATENCY_PROBE_PORT);

    std::cout << "Latency probe forwarder running..." << std::endl;

    while(1){
    	// wait for incoming probe packet
    	packet.lenght = recvfrom(socketDescription, packet.msg, sizeof(LossProbePacket), 0, NULL, NULL);

    	//! defines if the packet has came back to its source
    	bool reached = false;
    	LossProbeHeader * header = packet.getLossProbeHeader();
    	struct timeval stamp;

    	if (header->nextHop < header->pathLength-1) { // -1 to get rid of the source
			if (header->direction == 0) {
				// forward
				std::cout << "forward" << std::endl;
				header->nextHop++;
				nextForwarderAddr.sin_addr.s_addr =
						header->path[header->nextHop];

				//TODO calcul loss
				//header->lossResult[header->nextStamp] = loss;
				header->nextStamp += 2;
			} else {
				//backward
				if (header->nextHop == 0) {
					// if the packet has came back to its source, get the final result

					//TODO calcul loss
					//header->lossResult[header->nextStamp] = loss;

					// extract each edge loss
					std::vector<double> loss;
					IPPath path;
					int i = header->pathLength - 2;
					long sum = 0;
					while (i >= 0) {
						int u = header->path[i];
						int v = header->path[i + 1];
						int start = header->lossResult[2 * i];
						int finish = header->lossResult[2 * i + 1];
						//long mtime = time_diff(start, finish) - sum;

						//loss.insert(loss.begin(), mtime);
						//sum += mtime;
						i--;
					}

					for (size_t j = 1; j < header->pathLength; j++) {
						path.push_back(header->path[j]);
					}

					// send back to monitor
					agent->sendResultsToMonitorManager(path, loss);

					reached = true;
				}else {
                    std::cout << "keep going backward" << std::endl;
                    // keep going backward
                    header->nextHop--;
                    nextForwarderAddr.sin_addr.s_addr = header->path[header->nextHop];

                    //TODO calcul loss
                    //header->lossResult[header->nextStamp] = loss;
                    header->nextStamp -= 2;
                }
			}
    	}else {
            // last node of the path, the packet must go backward
            std::cout << "last node, must go backward" << std::endl;
            header->direction = 1;
            header->nextHop--;
            nextForwarderAddr.sin_addr.s_addr = header->path[header->nextHop];

            //TODO calcul loss
            //header->lossResult[header->nextStamp] = loss;
            header->nextStamp--;

        }

        // if the packet is in transit
        if (!reached) {
            if (sendto(socketDescription, (char*) packet.msg, sizeof(LossProbeHeader), 0, (struct sockaddr*) &nextForwarderAddr, sizeof(sockaddr_in)) < 0) {
                perror("failed to forward loss probe packet");
            }
        }
    }
}
