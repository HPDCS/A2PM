
############################
	DEPENDENCIES
############################

To install the required dependencies on an Ubuntu Linux system :

	sudo ./utils/install_dependencies_ubuntu.sh


############################
	COMPILATION
############################

Generate the Makefile:
	cd build
	cmake ..

To build the whole SMART system, enter the build directory and type :
	make

You can choose to build each agent separately  : 
	make router
	make monitor
	make RA
	make TA
	make forwarder

Generated executables can then be found in build/exe/ directory.

############################
   DOXYGEN DOCUMENTATION
############################
To generate the documentation, enter the build directory and type :
	make doc

The generated documentation can be found in build/doc/html/index.html

############################
   CONFIGURATION FILES
############################


Monitor configuration :
----------------------

1.File 'routerip'
routerip

Or you can passe routerip as parameter to the monitor executable (see monitor README file for more explanations)


Forwarder configuration :
-------------------------

1.File 'routerip'
routerip

Or you can passe routerip as parameter to the forwarder executable (see forwarder README file for more explanations)


TA configuration  :
---------------------

1. 'routerip' :
routerip

Or you can passe routerip as parameter to the TA executable (see TA README file for more explanations)

/////////////////////////////////////////////////////////////////////////////////////
//This configuration file is no more useful as only one TA could be used in a VM.
//We keep it only for 
2. 'nfqueueid'
nfqueueid
/////////////////////////////////////////////////////////////////////////////////////
  
RA configuration :
---------------------

1. 'mylocalip' :
mylocalip

It is the private ip of the RA VM. if public IP is used (no NAT) mylocalip=mypublicip
