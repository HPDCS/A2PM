
/****************************************************************************
 * Self MAnaged Routing overlay v1 - GPL v3                                      *
 *                                                                          *
 * This file is part of SMART (Self MAnaged Routing overlay)                       *
 *                                                                          *
 * SMART is a free software distributed under the GPL v3 licence              *
 *                                                                          *
 ****************************************************************************/
#include <iostream>
#include <cstdlib>
#include "monitor.h"
#include "LatencyProbe.h"
#include "LossProbe.h"


int main (int argc, char *argv[]) {

	if (argc > 1) {
		// creates the MonitorManager
		MonitorManager manager(argv[1]);

		// add the Probe agent measuring latencies
		ProbeLatency latencyProbe(&manager);
		latencyProbe.startForwarder(); // start the latencyProbe forwarder
		manager.addProbeAgent(&latencyProbe);

		// Start the manager main loop
		// Wait for monitoring instructions send by the PathManager
		manager.main();

	} else {
		// creates the MonitorManager
		MonitorManager manager;

		// add the Probe agent measuring latencies
		ProbeLatency latencyProbe(&manager);
		latencyProbe.startForwarder(); // start the latencyProbe forwarder
		manager.addProbeAgent(&latencyProbe);

		// Start the manager main loop
		// Wait for monitoring instructions send by the PathManager
		manager.main();

	}



    return EXIT_SUCCESS;
}
