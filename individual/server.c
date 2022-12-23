#define _GNU_SOURCE
#include <netinet/in.h>  //structure for storing address information
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>  //for socket APIs
// Headers for GNU Hash table
#include <assert.h>
#include <errno.h>
#include <search.h>

// TODO: Set 5555 as port number
#define PORT_NUMBER 5555
// TODO: Find out how to read long messages without stack overflow
#define STRING_LENGTH_LIMIT 32 * 1024 * 32
#define HASHTABLE_SIZE 100

struct BSstring {
  int strLen;
  char* string;
};

// ///////////////////////////////////////////////////////
// //////////////////////////////////////////BEGIN MAIN
// ///////////////////////////////////////////////////////

int main(int argc, char const* argv[]) {
  // create and initialize hash table
  struct hsearch_data* htab;
  /*dynamically allocate memory for a single table.*/
  htab = (struct hsearch_data*)calloc(1, sizeof(struct hsearch_data));
  // zeroize the table.
  // memset(htab, 0, sizeof(struct hsearch_data));
  /*create 30 table entries.*/
  assert(hcreate_r(HASHTABLE_SIZE, htab) != 0);

  // Data structures to manage the hash table
  // Space to store data strings.
  struct BSstring dataValues[HASHTABLE_SIZE];
  memset(dataValues, 0, sizeof(struct BSstring) * HASHTABLE_SIZE);
  // Next location to insert data value.
  struct BSstring* currentInsertionLocation = dataValues;

  // string store data to send to client
  char sendBuffer[STRING_LENGTH_LIMIT] = {0};
  int sendBufferLength = 0;
  char recvBuffer[STRING_LENGTH_LIMIT] = {0};
  int num_clients = 1001; // accept 1000 clients

  // ///////////////////////////////////////////////////////
  // //////////////////////////////////////////SOCKET OPERATIONS
  // ///////////////////////////////////////////////////////

  // create server socket
  int servSockD = socket(AF_INET, SOCK_STREAM, 0);
  if (servSockD == -1) {
    printf("socket creation failed...\n");
    exit(0);
  } else
    printf("Socket successfully created..\n");

  // define server address
  struct sockaddr_in servAddr, client;
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

  // ///////////////////////////////////////////////////////
  // //////////////////////////////////////////START LISTENING
  // ///////////////////////////////////////////////////////

  while (num_clients) {
    // integer to hold client socket.
    int clientSocket = accept(servSockD, NULL, NULL);
    if (clientSocket < 0) {
      printf("Server failed to accept the client.\n");
      exit(0);
    } else
      printf("Server accepted client number %d.\n", num_clients);
    --num_clients;

    // receive message from client
    long msgLength = read(clientSocket, recvBuffer, sizeof(recvBuffer));

    // print buffer which contains the client contents
    printf("From client: %s\n", recvBuffer);
    printf("Message length: %ld\n", msgLength);
    if (msgLength == 0) {
      printf("No more messages to read, exiting.\n");
      break;
    }

    // parse the input string
    long location = 0;
    int operation = 0;
    long keyLength = 0, valueLength = 0;

    // checks if input is GET (1) or SET (2), otherwise close connection (0)
    while (location < STRING_LENGTH_LIMIT) {
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
        printf("Invalid operation %d from client, exiting.\n", operation);
        break;
      }

      // ///////////////////////////////////////////////////////
      // //////////////////////////////////////////GET OPERATION
      // ///////////////////////////////////////////////////////

      // GET found = read length of key
      if (operation == 1) {
        printf("GET operation\n");
        // increment location until length number
        location += 4;
        keyLength = atoi(recvBuffer + location);
        ++location;  // increment beyond current $ to enable next dollar search
        printf("Length of key: %ld\n", keyLength);

        // create and initialize buffer of read size
        char keyBuffer[keyLength + 1];
        memset(keyBuffer, 'Z', keyLength);
        keyBuffer[keyLength] = '\0';

        // update location to start reading key value
        while (recvBuffer[location - 1] != '$') ++location;

        printf("Location: %ld\n", location);

        // copy key into buffer until keyLength
        for (int j = 0; j < keyLength; ++j) {
          // printf("recvbuffer: %c\n", recvBuffer[i]);
          keyBuffer[j] = recvBuffer[j + location];
        }
        // increment location to match current value
        location += keyLength;
        printf("Key read from GET: %s\n", keyBuffer);
        printf("Location: %ld\n", location);

        // execute READ operation on DB and print value or ERR
        // variables for hash table ops
        ENTRY e, *ep;
        e.key = keyBuffer;
        int retVal = hsearch_r(e, FIND, &ep, htab);
        printf("GET RetVal: %d\n", retVal);
        if (retVal == 0) {  // key does not exist
          printf("Value of errno: %d\n ", errno);
          printf("The error message is : %s\n", strerror(errno));
          perror("Message from perror");
          memset(sendBuffer, 0, STRING_LENGTH_LIMIT);
          sendBuffer[0] = 'E';
          sendBuffer[1] = 'R';
          sendBuffer[2] = 'R';
          sendBuffer[3] = '\n';
          sendBufferLength = 4;
        } else {  // key exists
          struct BSstring* readValue = (struct BSstring*)(ep->data);
          printf("Read value from htab: %s\n", readValue->string);
          // Copy data to sendBuffer
          memset(sendBuffer, 0, STRING_LENGTH_LIMIT);
          int i = 0;
          sendBuffer[i++] = 'V';
          sendBuffer[i++] = 'A';
          sendBuffer[i++] = 'L';
          sendBuffer[i++] = 'U';
          sendBuffer[i++] = 'E';
          sendBuffer[i++] = '$';
          // convert string length from int to str
          char numStr[STRING_LENGTH_LIMIT];
          int numStrLength = snprintf(NULL, 0, "%d", readValue->strLen);
          printf("Length of num string: %d\n", numStrLength);
          snprintf(numStr, numStrLength + 1, "%d", readValue->strLen);
          printf("Num string: %s\n", numStr);
          for (int j = 0; j < numStrLength; ++j) sendBuffer[i++] = numStr[j];
          // append $ after length
          sendBuffer[i++] = '$';
          // append value read from table
          for (int j = 0; j < readValue->strLen; ++j)
            sendBuffer[i++] = readValue->string[j];
          // append termination characters
          sendBuffer[i++] = '\n';
          sendBufferLength = i;
        }
        printf("Sending message to client: %s\n", sendBuffer);
        // send message to client socket based on executed operation
        send(clientSocket, sendBuffer, sendBufferLength, 0);
      }  // GET operation ends

      // ///////////////////////////////////////////////////////
      // //////////////////////////////////////////SET OPERATION
      // ///////////////////////////////////////////////////////
      if (operation == 2) {
        printf("SET operation\n");
        // increment location until length number
        location += 4;
        keyLength = atoi(recvBuffer + location);
        printf("Length of key: %ld\n", keyLength);
        // Edge case: If key length > 4 Mb terminate connection
        if (keyLength < 1 || keyLength > STRING_LENGTH_LIMIT) {
          memset(sendBuffer, 0, STRING_LENGTH_LIMIT);
          sendBuffer[0] = 'E';
          sendBuffer[1] = 'R';
          sendBuffer[2] = 'R';
          sendBuffer[3] = '\n';
          sendBufferLength = 4;
          printf("Sending message to client: %s\n", sendBuffer);
          // send message to client socket based on executed operation
          send(clientSocket, sendBuffer, sendBufferLength, 0);
          printf("Invalid operation %d from client, exiting.\n", operation);
          break;
        }

        // create and initialize buffer of read size
        char* keyBuffer = (char*)malloc((keyLength + 1) * sizeof(char));
        memset(keyBuffer, 'Z', keyLength);
        keyBuffer[keyLength] = '\0';

        ++location;  // increment beyond current $ to enable next dollar search
        // update location to start reading key value
        while (recvBuffer[location - 1] != '$') ++location;

        printf("Location: %ld\n", location);

        // copy key into buffer until keyLength
        for (int i = location; i < (location + keyLength); ++i) {
          // printf("recvbuffer: %c\n", recvBuffer[i]);
          keyBuffer[i - location] = recvBuffer[i];
        }
        // increment location to $ after key
        location += keyLength;
        printf("Key read from SET: %s\n", keyBuffer);
        printf("Location: %ld\n", location);

        // read value
        ++location;  // increment beyond current $ to read value
        valueLength = atoi(recvBuffer + location);
        ++location;  // increment beyond current $ to read string value
        printf("Length of value: %ld\n", valueLength);

        // create and initialize buffer of read size
        char* valueBuffer = (char*)malloc((valueLength + 1) * sizeof(char));
        memset(valueBuffer, 'Z', valueLength);
        valueBuffer[valueLength] = '\0';

        // update location to start reading key value
        while (recvBuffer[location - 1] != '$') ++location;

        printf("Location: %ld\n", location);

        // copy key into buffer until keyLength
        for (int i = location; i < (location + valueLength); ++i) {
          // printf("recvbuffer: %c\n", recvBuffer[i]);
          valueBuffer[i - location] = recvBuffer[i];
        }
        // increment location to $ after key
        location += valueLength;
        printf("Value read from SET: %s\n", valueBuffer);
        printf("Location: %ld\n", location);

        // get lock on table, execute WRITE operation
        // variables for hash table ops
        currentInsertionLocation->strLen = valueLength;
        currentInsertionLocation->string = valueBuffer;
        ENTRY e, *ep;
        e.key = keyBuffer;
        e.data = currentInsertionLocation;
        ++currentInsertionLocation;
        int retVal = hsearch_r(e, ENTER, &ep, htab);
        printf("SET RetVal: %d\n", retVal);
        if (retVal == 0) {  // error inserting KEY == OOM
          printf("Value of errno: %d\n ", errno);
          printf("The error message is : %s\n", strerror(errno));
          perror("Message from perror");
        } else {  // value inserted successfully
          memset(sendBuffer, 0, STRING_LENGTH_LIMIT);
          sendBuffer[0] = 'O';
          sendBuffer[1] = 'K';
          sendBuffer[2] = '\n';
          sendBufferLength = 3;
        }
        printf("Sending message to client: %s\n", sendBuffer);
        // send message to client socket based on executed operation
        send(clientSocket, sendBuffer, sendBufferLength, 0);
      }  // SET operation ends

      // //////////////////////////////////////////BACK IN MAIN
      // /////////////////////////

      // increment location to next operation
      if (recvBuffer[location] == '\n') ++location;
      printf("Location: %ld value: %c\n", location, recvBuffer[location]);
      continue;
    }
  }
  // Close the socket
  close(servSockD);
  // Destroy the hash table free heap memory
  hdestroy_r(htab);
  free(htab);
  // TODO: Iterate over dataValues and free all allocated pointers
  return 0;
}

/*
TODO:
0. Support multiple messages with same client --> DONE
0.4 Binary safe strings: Length matters, not the \0 character
0.5 Implement internal database to SET (add or replace) and GET (in parallel)
keys  --> ALMOST
0.6 Edge case: sending weird characters = sanitize input 0.7 Edge case:
Sending commands other than SET GET 0.8 Edge case: Sending extremely long
strings for key (longer than 4 M) 0.9 Edge case: Sending extremely long string
for key AND value (longer than 4 M) 0.91 Edge case: Terminate connection when
message length is 0
1. Support talking to multiple clients --> multi-threading support for 1000
threads
1.7 Edge Case: client terminating the connection before finishing a request.
1.8. Misbehaving client should be terminated --> close connection
*/