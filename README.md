Objective: Implemented an in-memory key-value store server which stores and retrieves data items referred to by the keys. The server listens on a TCP network socket, and responds to client requests to ’get’ and ’set’ values coming from programs (clients) across the network.

Implementation: Implemented as a single source file in C and runs on Linux. Does not require any external libraries apart from libpthread (for thread support).

Compilation command: gcc -Wall -pthread server.c -o server

Server launch command: ./server

Testing command (invokes the client(s)): ./runtest.sh in the respective directory

Operation Mechanism: The server keeps an internal database, correlating keys with their data values. Clients can set a key, by issuing the SET command. If the key does not exist it is created, if it already exists the data value is replaced. All clients see the same database, a key set by one should be visible to all other connected clients immediately. The server listens on TCP port 5555 and keeps accepting new connections. Requests are fully atomic, if a GET and a SET requests are launched at the same time (from different clients) the GET has to return either the data value before or after the SET. No client should be able to block another client. For example also a client that reads very slowly should not block a faster client. A misbehaving client should not be able to crash the server or cause a memory leak. This includes sending invalid requests or terminating the connection before finishing a request. If the server detects a misbehaving client, it will terminate the connection to that client, but continue functioning for other clients. 

The server implements two commands, GET and SET, and can process an 'infinite' sequence of these commands. When we say strings, we mean arbitrary length binary safe strings. They may contain null characters and might be up to 32Mb in length. To enable binary safe encoding, we prefix the length of the string in bytes. For example $6$hallo\n represents a 6-byte string. All commands and responses are terminated with a newline character. GET is followed by a key (encoded as string) the server should reply with a VALUE response followed by the keys value. If the key does not exist, the server should send the ERR response. SET is followed by a key, followed by a value (both encoded as string). The server should reply with a OK response. If the key can’t be stored (out of memory or similar error conditions) the server should send the ERR response.

