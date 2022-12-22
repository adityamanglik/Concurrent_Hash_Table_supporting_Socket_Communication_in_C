#define _GNU_SOURCE
#include <netinet/in.h>  //structure for storing address information
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>  //for socket APIs
// Headers for GNU Hash table
#include <assert.h>
#include <errno.h>
#include <search.h>

// TODO: Set 5555 as port number
#define PORT_NUMBER 5556
// TODO: Find out how to read long messages without stack overflow
#define STRING_LENGTH_LIMIT 32 * 1024
#define HASHTABLE_SIZE 100

char* executeOperation(char* message, int operation) { return "ERR"; }

int main(int argc, char const* argv[]) {
  // create and initialize hash table
  struct hsearch_data* htab;
  /*dynamically allocate memory for a single table.*/
  htab = (struct hsearch_data*)calloc(1, sizeof(struct hsearch_data));
  /*zeroize the table.*/  // Not needed because calloc
  // memset(htab, 0, sizeof(struct hsearch_data));
  /*create 30 table entries.*/
  assert(hcreate_r(HASHTABLE_SIZE, htab) != 0);

  // string store data to send to client
  char sendBuffer[STRING_LENGTH_LIMIT] = "Single-thread Server: ";
  char recvBuffer[STRING_LENGTH_LIMIT] = {0};
  int num_clients = 1;
  int max_messages_per_client = 9999;

  // create server socket
  int servSockD = socket(AF_INET, SOCK_STREAM, 0);
  if (servSockD == -1) {
    printf("socket creation failed...\n");
    exit(0);
  } else
    printf("Socket successfully created..\n");

  // define server address
  struct sockaddr_in servAddr;
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(PORT_NUMBER);
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  // Binding newly created socket to given IP and verification
  if ((bind(servSockD, (struct sockaddr*)&servAddr, sizeof(servAddr))) != 0) {
    printf("Socket binding failed.\n");
    exit(0);
  } else
    printf("Socket successfully binded.\n");

  // Now server is ready to listen
  if ((listen(servSockD, 1)) != 0) {
    printf("Listen failed...\n");
    exit(0);
  } else
    printf("Server listening..\n");

  while (num_clients) {
    // integer to hold client socket.
    int clientSocket = accept(servSockD, NULL, NULL);
    if (clientSocket < 0) {
      printf("Server failed to accept the client.\n");
      exit(0);
    } else
      printf("Server accepted the client.\n");
    --num_clients;

    // receive message from client
    ssize_t msgLength = read(clientSocket, recvBuffer, sizeof(recvBuffer));

    // print buffer which contains the client contents
    printf("From client: %s", recvBuffer);
    printf("Message length: %d\n", msgLength);
    if (msgLength == 0) {
      printf("No more messages to read, exiting.\n");
      break;
    }

    // parse the input string
    int location = 0;
    int operation = 0;
    size_t keyLength = 0, valueLength = 0;

    // checks if input is GET (1) or SET (2), otherwise close connection (0)
    for (; location < msgLength;) {
      // find operation at current location
      operation = 0;
      if (recvBuffer[location] == 'G' && recvBuffer[location + 1] == 'E' &&
          recvBuffer[location + 2] == 'T')
        operation = 1;
      else if (recvBuffer[location] == 'S' && recvBuffer[location + 1] == 'E' &&
               recvBuffer[location + 2] == 'T')
        operation = 2;

      printf("Operation: %d\n", operation);
      if (operation == 0) {
        printf("Invalid operation from client, exiting.\n", operation);
        break;
      }

      // GET found = read length of key
      if (operation == 1) {
        printf("GET operation\n");
        // variables for hash table ops
        ENTRY e, *ep;
        // increment location until length number
        location += 4;
        keyLength = atoi(recvBuffer + location);
        ++location;  // increment beyond current $ to enable next dollar search
        printf("Length of key: %d\n", keyLength);

        // create and initialize buffer of read size
        char keyBuffer[keyLength + 1];
        memset(keyBuffer, 'Z', keyLength);
        keyBuffer[keyLength] = '\0';

        // update location to start reading key value
        while (recvBuffer[location - 1] != '$' && location < msgLength)
          ++location;

        printf("Location: %d\n", location);

        // copy key into buffer until keyLength
        for (int i = location; i < (location + keyLength) && i < msgLength;
             ++i) {
          // printf("recvbuffer: %c\n", recvBuffer[i]);
          keyBuffer[i - location] = recvBuffer[i];
        }
        // increment location to match current value
        location += keyLength;
        printf("Key read from GET: %s\n", keyBuffer);
        printf("Location: %d\n", location);

        // execute READ operation on DB and print value or ERR
        e.key = keyBuffer;
        int retVal = hsearch_r(e, FIND, &ep, htab);
        printf("RetVal: %d", retVal);
        if (retVal == 0) {
          printf("Value of errno: %d\n ", errno);
          printf("The error message is : %s\n", strerror(errno));
          perror("Message from perror");
          return;
        }

        // char* sendBuffer = executeOperation(keyBuffer, operation);

        // send message to client socket based on executed operation
        send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);

        // increment location to next operation
        if (recvBuffer[location] == '\n') ++location;
        printf("Location: %d value: %c\n", location, recvBuffer[location]);
        continue;
      }  // GET operation ends

      if (operation == 2) {
        printf("SET operation\n");
        // variables for hash table ops
        ENTRY e, *ep;
        // increment location until length number
        location += 4;
        keyLength = atoi(recvBuffer + location);
        printf("Length of key: %d\n", keyLength);

        // create and initialize buffer of read size
        char keyBuffer[keyLength + 1];
        memset(keyBuffer, 'Z', keyLength);
        keyBuffer[keyLength] = '\0';

        ++location;  // increment beyond current $ to enable next dollar search
        // update location to start reading key value
        while (recvBuffer[location - 1] != '$' && location < msgLength)
          ++location;

        printf("Location: %d\n", location);

        // copy key into buffer until keyLength
        for (int i = location; i < (location + keyLength) && i < msgLength;
             ++i) {
          // printf("recvbuffer: %c\n", recvBuffer[i]);
          keyBuffer[i - location] = recvBuffer[i];
        }
        // increment location to $ after key
        location += keyLength;
        printf("Key read from SET: %s\n", keyBuffer);
        printf("Location: %d\n", location);

        // read value
        ++location;  // increment beyond current $ to read value
        valueLength = atoi(recvBuffer + location);
        ++location;  // increment beyond current $ to read string value
        printf("Length of value: %d\n", valueLength);

        // create and initialize buffer of read size
        char valueBuffer[valueLength + 1];
        memset(valueBuffer, 'Z', valueLength);
        valueBuffer[valueLength] = '\0';

        // update location to start reading key value
        while (recvBuffer[location - 1] != '$' && location < msgLength)
          ++location;

        printf("Location: %d\n", location);

        // copy key into buffer until keyLength
        for (int i = location; i < (location + valueLength) && i < msgLength;
             ++i) {
          // printf("recvbuffer: %c\n", recvBuffer[i]);
          valueBuffer[i - location] = recvBuffer[i];
        }
        // increment location to $ after key
        location += valueLength;
        printf("Value read from SET: %s\n", valueBuffer);
        printf("Location: %d\n", location);

        // get lock on table, execute WRITE operation
        e.key = keyBuffer;
        e.data = valueBuffer;
        int retVal = hsearch_r(e, ENTER, &ep, htab);
        printf("RetVal: %d", retVal);
        if (retVal == 0) {
          printf("Value of errno: %d\n ", errno);
          printf("The error message is : %s\n", strerror(errno));
          perror("Message from perror");
          return;
        }
        // char* sendBuffer = executeOperation(keyBuffer, operation);

        // send message to client socket based on executed operation
        send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);

        // increment location to next operation
        if (recvBuffer[location] == '\n') ++location;
        printf("Location: %d value: %c\n", location, recvBuffer[location]);
        continue;
      }  // SET operation ends
    }
  }
  // Close the socket
  close(servSockD);
  // Destroy the hash table free heap memory
  hdestroy_r(htab);
  free(htab);
  return 0;
}

/*
TODO:
0. Support multiple messages with same client --> DONE
0.4 Binary safe strings: Length matters, not the \0 character
0.5 Implement internal database to SET (add or replace) and GET (in parallel)
keys 0.6 Edge case: sending weird characters = sanitize input 0.7 Edge case:
Sending commands other than SET GET 0.8 Edge case: Sending extremely long
strings for key (longer than 32 Mb) 0.9 Edge case: Sending extremely long string
for key AND value (longer than 32 Mb) 0.91 Edge case: Terminate connection when
message length is 0
1. Support talking to multiple clients --> multi-threading support for 1000
threads
1.7 Edge Case: client terminating the connection before finishing a request.
1.8. Misbehaving client should be terminated --> close connection
*/