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
#include "TA.h"


void signal_callback_handler(int signum)
{
   printf("Caught signal %d\n",signum);
   //transmissionAgent.onExit();
}

int main (int argc, char *argv[]) {

	if (argc > 1) {
		TransmissionAgent transmissionAgent(argv[1]);

		signal(SIGINT, signal_callback_handler);
		signal(SIGTERM, signal_callback_handler);

		transmissionAgent.run();

	}else{
		TransmissionAgent transmissionAgent;

		signal(SIGINT, signal_callback_handler);
		signal(SIGTERM, signal_callback_handler);

		transmissionAgent.run();
	}



    return EXIT_SUCCESS;
}
