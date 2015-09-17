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
#include "router.h"

int main (int argc, char *argv[]) {
    std::cout << "SMART Router running" << std::endl;
    std::string routingManager = "euler";
    size_t maxPathsToTest = 5;
    int role = 1;

    if (argc > 1) // the user want to use a specific kind of routing manager
        routingManager = std::string(argv[1]);

    if (argc > 2)
    	role = atoi(argv[2]);

    if (argc > 3) // user want to limit the number of path explored
        maxPathsToTest = atoi(argv[3]);


    if (argc > 4){ // user want to read the result from a file
        Router router(routingManager, role, maxPathsToTest, std::string(argv[4]));
    	router.simulation();
    }else {
        Router router(routingManager, role, maxPathsToTest);
        router.run();
    }



    return EXIT_SUCCESS;
}


