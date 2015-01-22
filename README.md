dejavu
======

<b>dejavu</b> implements a Chord DHT using MyThread.h (library created as part of the project to use multithreading)

The operations supported are:

* help 			: Provides a list of command and their usage details.
* port <x>		: Listens on this port for other instances of the program over different nodes.
* create 		: creates the ring.
* join <x>		: Joins the ring with x address.
* quit			: Shuts down the ring.
* put <key> <value>	: inserts the given <key,value> pair in the ring.
* get <key>		: returns the value corresponding to the key, if one was previously inserted in the node.
* finger		: a list of addresses of nodes on the ring.
* successor		: an address of the next node on the ring.
* predecessor		: an address of the previous node on the circle
* dump			: displays all information pertaining to calling node.
* dumpaddr <address>	: displays all information pertaining to node at address.
* dumpall		: All information of all the nodes.

-----
<b>MyThread.h</b> --- You will find all the required information about MyThread.h by opening "./html/MyThread_8h.html"


-----
<b>Build & run the code</b>

To build the code run 'make' from inside this directory.
You will need to have sudo permissions as our code requires libssl-dev to be installed on your system.

To run, after compiling the code, run './chord.o' from within the terminal.
