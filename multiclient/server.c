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
#include <pthread.h>
#include <netinet/tcp.h>

// TODO: Set 5555 as port number
#define PORT_NUMBER 5555
// TODO: Find out how to read long messages without stack overflow
#define STRING_LENGTH_LIMIT 32 * 1024 * 32 * 4
#define KEY_TRUNCATION 99999
#define HASHTABLE_SIZE 999

struct BSstring {
  int strLen;
  char* string;
};

// the thread function
void* connection_handler(void*);
// lock for hash table concurrency
pthread_mutex_t lock;

// GLOBAL VARIABLE TO HOLD HASH TABLE
// create and initialize hash table
struct hsearch_data* htab;
struct BSstring* currentInsertionLocation = NULL;
// Space to store data strings.
struct BSstring dataValues[HASHTABLE_SIZE];

// ///////////////////////////////////////////////////////
// //////////////////////////////////////////BEGIN MAIN
// ///////////////////////////////////////////////////////

int main(int argc, char const* argv[]) {
  /*dynamically allocate memory for a single table.*/
  htab = (struct hsearch_data*)calloc(1, sizeof(struct hsearch_data));
  assert(hcreate_r(HASHTABLE_SIZE, htab) != 0);
  memset(dataValues, 0, sizeof(struct BSstring) * HASHTABLE_SIZE);
  // Next location to insert data value.
  currentInsertionLocation = dataValues;

  int num_clients = 1001;  // accept 1000 clients

  // ///////////////////////////////////////////////////////
  // //////////////////////////////////////////SOCKET OPERATIONS
  // ///////////////////////////////////////////////////////

  int socket_desc, client_sock, c;
  struct sockaddr_in server, client;

  // Create socket
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    printf("Could not create socket");
  }
  printf("Socket created");

  // Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(PORT_NUMBER);

  // Bind
  if (bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0) {
    // print the error message
    printf("bind failed. Error");
    return 1;
  }
  printf("bind done");

  // Listen
  listen(socket_desc, 3);

  // Accept and incoming connection
  printf("Waiting for incoming connections...");
  c = sizeof(struct sockaddr_in);
  pthread_t thread_id;

  while ((client_sock =
              accept(socket_desc, (struct sockaddr*)&client, (socklen_t*)&c))) {
    printf("Connection accepted");
    int flag;
    int result = setsockopt(client_sock,            /* socket affected */
                                 IPPROTO_TCP,     /* set option at TCP level */
                                 TCP_NODELAY,     /* name of option */
                                 (char *) &flag,  /* the cast is historical cruft */
                                 sizeof(int));    /* length of option value */

    if (pthread_create(&thread_id, NULL, connection_handler,
                       (void*)&client_sock) < 0) {
      printf("could not create thread");
      return 1;
    }
    printf("Handler assigned");
    // Now join the thread , so that we dont terminate before the thread
    pthread_join(thread_id , NULL);
    close(client_sock);
  }

  if (client_sock < 0) {
    printf("accept failed");
    return 1;
  }
  // close socket
  close(socket_desc);
  // Destroy the hash table free heap memory
  hdestroy_r(htab);
  free(htab);
  // Close data leak in table
  struct BSstring* memFree = dataValues;
  for (long i = 0; i < HASHTABLE_SIZE; ++i) {
    if (memFree) free(memFree->string);
    ++memFree;
  }
  return 0;
}

// ///////////////////////////////////////////////////////
// //////////////////////////////////////////END OF MAIN FUNCTION
// ///////////////////////////////////////////////////////

// Handle connection for each client
void* connection_handler(void* socket_desc) {
  int clientSocket = *(int*)socket_desc;

  // string store data to send to client
  char* sendBuffer = (char*)calloc(1, STRING_LENGTH_LIMIT * sizeof(char));
  int sendBufferLength = 0;
  char* recvBuffer = (char*)calloc(1, STRING_LENGTH_LIMIT * sizeof(char));

  // receive message from client
  long msgLength = read(clientSocket, recvBuffer, STRING_LENGTH_LIMIT);

  // Input sanitizer
  for (long i = 0; i < STRING_LENGTH_LIMIT; i++) {
    if (recvBuffer[i] > 127) {
      printf("Invalid characters in input, exiting.\n");
      return NULL;
    }
  }

  // print buffer which contains the client contents
  printf("From client: %s\n", recvBuffer);
  printf("Message length: %ld\n", msgLength);
  if (msgLength < 1) {
    printf("No more messages to read, exiting.\n");
    return NULL;
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
      return NULL;
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
        printf("Invalid keyLength %ld from client, exiting.\n", keyLength);
        return NULL;
      }

      // create and initialize buffer of keyLength size
      char* keyBuffer = (char*)calloc(1, (keyLength + 1) * sizeof(char));
      memset(keyBuffer, 'Z', keyLength);
      keyBuffer[keyLength] = '\0';

      // increment beyond current $ to enable next dollar search
      ++location;
      // update location to start reading key value
      while (recvBuffer[location - 1] != '$') ++location;

      printf("Location: %ld Value: %c\n", location, recvBuffer[location]);

      // copy key into buffer until keyLength
      for (long i = location; i < (location + keyLength); ++i) {
        // printf("recvbuffer: %c\n", recvBuffer[i]);
        keyBuffer[i - location] = recvBuffer[i];
      }
      // increment location to match current value
      location += keyLength;
      printf("Key read from GET: %s\n", keyBuffer);
      printf("Location: %ld Value: %c\n", location, recvBuffer[location]);

      // execute READ operation on DB and print value or ERR
      // variables for hash table ops
      ENTRY e, *ep;
      e.key = keyBuffer;
      // obtain lock before accessing table
      pthread_mutex_lock(&lock);
      int retVal = hsearch_r(e, FIND, &ep, htab);
      printf("GET RetVal: %d\n", retVal);
      if (retVal == 0) {  // key does not exist
        printf("Value of errno: %d\n ", errno);
        printf("The error message is : %s\n", strerror(errno));
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
        for (long j = 0; j < numStrLength; ++j) sendBuffer[i++] = numStr[j];
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
      // release lock after sending message
      pthread_mutex_unlock(&lock);
      free(keyBuffer);
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
        printf("Invalid key length %ld from client, exiting.\n", keyLength);
        return NULL;
      }

      // create and initialize buffer of read size
      char* keyBuffer = (char*)calloc(1, (keyLength + 1) * sizeof(char));
      memset(keyBuffer, 'Z', keyLength);
      keyBuffer[keyLength] = '\0';

      // increment beyond current $ to enable next dollar search
      ++location;
      // update location to start reading key value
      while (recvBuffer[location - 1] != '$') ++location;

      printf("Location: %ld Value: %c\n", location, recvBuffer[location]);

      // copy key into buffer until keyLength
      for (long i = location; i < (location + keyLength); ++i) {
        // printf("recvbuffer: %c\n", recvBuffer[i]);
        keyBuffer[i - location] = recvBuffer[i];
      }
      // increment location to $ after key
      location += keyLength;
      printf("Key read from SET: %s\n", keyBuffer);
      printf("Location: %ld Value: %c\n", location, recvBuffer[location]);

      // read value
      ++location;  // increment beyond current $ to read value
      valueLength = atoi(recvBuffer + location);

      printf("Location: %ld Value: %c\n", location, recvBuffer[location]);

      // Edge case: If value length > 4 Mb terminate connection
      if (valueLength < 1 || valueLength > STRING_LENGTH_LIMIT) {
        memset(sendBuffer, 0, STRING_LENGTH_LIMIT);
        sendBuffer[0] = 'E';
        sendBuffer[1] = 'R';
        sendBuffer[2] = 'R';
        sendBuffer[3] = '\n';
        sendBufferLength = 4;
        printf("Sending message to client: %s\n", sendBuffer);
        // send message to client socket based on executed operation
        send(clientSocket, sendBuffer, sendBufferLength, 0);
        printf("Invalid value length %ld from client, exiting.\n", valueLength);
        return NULL;
      }

      printf("Length of value: %ld\n", valueLength);

      // create and initialize buffer of read size
      char* valueBuffer = (char*)calloc((valueLength + 1), sizeof(char));
      memset(valueBuffer, 'Z', valueLength);
      valueBuffer[valueLength] = '\0';

      // increment beyond current $ to read string value
      ++location;
      // update location to start reading key value
      while (recvBuffer[location - 1] != '$') ++location;

      printf("Location: %ld Value: %c\n", location, recvBuffer[location]);

      // copy key into buffer until keyLength
      for (long i = location; i < (location + valueLength); ++i) {
        // printf("recvbuffer: %c\n", recvBuffer[i]);
        valueBuffer[i - location] = recvBuffer[i];
      }
      // increment location to $ after key
      location += valueLength;
      printf("Value read from SET: %s\n", valueBuffer);
      printf("Location: %ld Value: %c\n", location, recvBuffer[location]);
      // get lock on table, execute WRITE operation
      pthread_mutex_lock(&lock);
      // variables for hash table ops
      currentInsertionLocation->strLen = valueLength;
      currentInsertionLocation->string = valueBuffer;
      ENTRY e, *ep;
      e.key = keyBuffer;
      // Find before inserting new value
      int retVal = hsearch_r(e, FIND, &ep, htab);
      if (retVal != 0) {  // value already exists in table
        struct BSstring* readValue = (struct BSstring*)(ep->data);
        readValue->strLen = valueLength;
        readValue->string = valueBuffer;
        memset(sendBuffer, 0, STRING_LENGTH_LIMIT);
        sendBuffer[0] = 'O';
        sendBuffer[1] = 'K';
        sendBuffer[2] = '\n';
        sendBufferLength = 3;
      } else {  // value does not exist, create new entry
        e.data = currentInsertionLocation;
        ++currentInsertionLocation;
        retVal = hsearch_r(e, ENTER, &ep, htab);
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
      }
      printf("Sending message to client: %s\n", sendBuffer);
      // send message to client socket based on executed operation
      send(clientSocket, sendBuffer, sendBufferLength, 0);
      // free lock after sending correc data
      pthread_mutex_unlock(&lock);
    }  // SET operation ends

    // //////////////////////////////////////////END OF COMMAND
    // ///////////////////////////////////////////////////////
    // increment location to next operation
    if (recvBuffer[location] == '\n') ++location;
    printf("Location: %ld value: %c\n", location, recvBuffer[location]);
    continue;
  }
    // //////////////////////////////////////////FREE MEMORY, EXIT
    // ///////////////////////////////////////////////////////

  free(sendBuffer);
  free(recvBuffer);
  return NULL;
}

/*
TODO:
0. Support multiple messages with same client --> DONE
0.4 Binary safe strings: Length matters, not the \0 character
0.5 Implement internal database to SET (add or replace) and GET (in parallel)
keys  --> DONE
0.6 Edge case: sending weird characters = sanitize input --DONE
0.7 Edge case: Sending commands other than SET GET --> DONE
0.8 Edge case: Sending extremely long strings for key (longer than 4 M) --> DONE
0.9 Edge case: Sending extremely long string for key AND value (longer than 4 M)
--> DONE
0.91 Edge case: Terminate connection when message length is 0 --> DONE
1. Support talking to multiple clients --> multi-threading support for 1000
threads --> DONE
1.7 Edge Case: client terminating the connection before finishing a request.
1.8. Misbehaving client should be terminated --> close connection
*/