/*
 * CS 485/585 - Fall 2026
 * Lab 1: TCP Chat Room
 *
 * server.c
 *
 * See the assignment PDF for the specification and
 * for the exact output strings the autograder expects.
 *
 * Your server must, at minimum:
 *   - parse --port and --password from the command line
 *   - listen on 127.0.0.1 and accept connections up to MAX_CLIENTS
 *   - run each connected client on its own thread
 *   - protect the shared client list against concurrent access
 *   - reassemble complete lines from a byte stream
 *   - interpret the chat commands and route the results
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h> // remove for submission, debugging only
#include <getopt.h> // as suggested by instructions for arg parsing.

// IO helpfuls
#define PORT_CMD "port"
#define PASSWORD_CMD "password"
#define INIT_CONNECT_STR "Server started on port %d. Accepting connections"
#define SERVER_FULL_STR "Server full"
#define INCORRECT_PASS_STR "Incorrect password" // NOTE: this may need a period?

#define USER_STR "Username"
#define PASS_STR "Password"

#define HOST_IP "127.0.0.1"
#define MAX_CLIENTS 10
#define USERNAME_BUFFER_SIZE 9
#define PASSWORD_BUFFER_SIZE 6
#define CLIENT_BUFFER_SIZE 301

/**
 * =============================================================
 * UTILITY STRUCTS
 * =============================================================
 */

/**
 * a structure to contain all details which make up a client.
 * a pointer to this structure is what should be stuffed in the client list.
 */
struct Client {
  int sending_socket; // socket file descriptor associated to send to this client.
  char *username; // readable username of the client. 
};


/**
 * a structure to hold a client list, current capacity, and the lock for access
 */
struct ClientList {
  struct Client *clients[MAX_CLIENTS]; // list of clients 
  int current_capacity; // current number of clients in the chatroom
  pthread_mutex_t lock; // lock for accessing either one of these pieces
};


/**
 * the argument to be given to any client thread handler on start.
 * contains all information needed for them to carry out their tasks.
 */
struct ClientThreadArg {
  int receiving_socket; // the socket which a thread should be listening to.
  char *username; // username of the associated client.
  char *buffer; // buffer which to receive messages into on recv calls.
  int buffer_size; // the size of the above buffer for clarity.
  pthread_mutex_t *server_out_lock; // a lock for stdout printing. this may be changed to use snprintf.
  struct ClientList *client_list; // to avoid globals, all have pointer to the client list.
};

/**
 * =============================================================
 * GLOBALS
 * =============================================================
 */

/**
 * =============================================================
 * THREAD HANDLERS
 * =============================================================
 */ 

/**
 * the main handler for a client connection manager thread.
 */
void *client_thread_handler(void * arg) {
    struct ClientThreadArg *data = (struct ClientThreadArg *)arg;
    // TODO
    return NULL;
} // end function

/**
 * =============================================================
 * UTILITY FUNCTIONS
 * =============================================================
 */

/**
 * this is a general method for client threads to 
 * use when wanting to send a message to the general chatroom,
 * or all live clients in the chat.
 */
void broadcast(char *msg, struct ClientList *client_list) {
  pthread_mutex_lock(&(client_list->lock));
  
  struct Client *c;
 
  // dumb function: going to iterate through, and send message
  // to all non-null entries in client list.
  for (int i = 0; i < MAX_CLIENTS; i++) {
    c = client_list->clients[i];

    if (c) {
      // if a live client, send
      send(c->sending_socket, msg, strlen(msg), 0);
      // TODO: handle send error? likely just ignore.
    } // end if
  } // end loop

  pthread_mutex_unlock(&(client_list->lock));
} // end function


/**
 * this is a general message to send a message to a specific user
 * as detected by the client threads.
 */
void peer_to_peer(char *username, char *msg, struct ClientList *client_list) {
  pthread_mutex_lock(&(client_list->lock));

  struct Client *c;

  // dumb function: going to iterate through, and send message
  // to all non-null entries in client list.
  for (int i = 0; i < MAX_CLIENTS; i++) {
    c = client_list->clients[i];

    if (c && !strcmp(username, c->username)) {
      // if a live client, send
      send(c->sending_socket, msg, strlen(msg), 0);
      // TODO: handle send error? likely just ignore.
      break; // we're done, move on with life
    } // end if
  } // end loop

  pthread_mutex_unlock(&(client_list->lock));
} // end function


/**
 * this may be replaced to use snprintf if this messes with autograder at all.
 * unsure yet.
 * but this is the threadsafe way all server components
 * should be printing to stdout stream.
 */
void print_to_server(char *msg, pthread_mutex_t *server_out_lock) {
  pthread_mutex_lock(server_out_lock);
  fprintf(stdout, msg);
  pthread_mutex_unlock(server_out_lock);
} // end function




/**
 * this is to only ever used by the main server process.
 * a utility method to be able to add a client
 * to the client list upon a connection acceptance
 * and password verification on a non-full chatroom.
 */
void add_client(int new_socket, char *username, struct ClientList *client_list, pthread_mutex_t *server_out_lock) {
  pthread_mutex_lock(&(client_list->lock));
  
  struct Client *c; 

  // dummy iteration: find the first non-null spot for the new client.
  for (int i = 0; i < MAX_CLIENTS; i++) {
    c = client_list->clients[i];

    if (!c) { // first null found, save here.
        struct Client *new_client = (struct Client *) malloc(sizeof(struct Client));

        if (!new_client) {
            fprintf(stderr, "Malloc null return for client, exiting...\n");
            exit(EXIT_FAILURE);
        } // end if

        // init new client info
        new_client->sending_socket = new_socket;
        new_client->username = username;

        // create new argument pack to give to the thread handler
        struct ClientThreadArg *arg = (struct ClientThreadArd *)malloc(sizeof(struct ClientThreadArg));
        if (!arg) {
            fprintf(stderr, "Malloc null return for arg, exiting...\n");
            exit(EXIT_FAILURE);
        } // end if

        // init a buffer for receiving messages into
        char *buffer = (char *)malloc(CLIENT_BUFFER_SIZE * sizeof(char));
        if (!buffer) {
            fprintf(stderr, "Malloc null return for buffer, exiting...\n");
            exit(EXIT_FAILURE);
        } // end if

        // set up entire argument list for client handler.
        arg->receiving_socket = new_socket;
        arg->username = username;
        arg->buffer = buffer;
        arg->buffer_size = CLIENT_BUFFER_SIZE;
        arg->server_out_lock = server_out_lock;
        arg->client_list = client_list;

        // add client to the list 
        client_list->clients[i] = new_client;

        // start up the handler thread.
        pthread_t thread;
        int ret = ptread_create(&thread, NULL, client_thread_handler, arg);
        if (ret) {
            fprintf(stderr, "pthread_create error! Exiting...");
            exit(EXIT_FAILURE);
        } // end if
        ret = pthread_detach(thread); // instead of blocking with join, let os auto clean on thread finish.
        if (ret) {
            fprintf(stderr, "pthread_detach error! Exiting...");
            exit(EXIT_FAILURE);
        } // end if

        break; // done, new client added
    } // end if
  } // end loop

  pthread_mutex_unlock(&(client_list->lock));
} // end function


/**
 * =============================================================
 * MAIN SERVER PROCESS
 * =============================================================
 */


int main(int argc, char *argv[])
{
  // set up some helpful values to hold on to
  char *CHATROOM_PASSWORD = "";
  int SERVER_PORT = -1;

  pthread_mutex_t capacity_lock; // lock for capacity count
  pthread_mutex_init(&capacity_lock, NULL); 
  int current_capacity = 0; // this can be accessed by other threads aside from main process


  // parse arguments
  int c = 0;
  int opt_index = 0;  

  static struct option long_options[] = {
                  {PORT_CMD,     required_argument, 0,  0 },
                  {PASSWORD_CMD, required_argument, 0,  0 },
                  {           0,                 0, 0,  0 }    
  };

  while (c >= 0) { // control mechanism for processing the options
    c = getopt_long(argc, argv, "", long_options, &opt_index); // get the next option index
    
    if (c == 0) {

      if (strcmp(long_options[opt_index].name, PORT_CMD) == 0) {
        SERVER_PORT = atoi(optarg);    
      } else if (strcmp(long_options[opt_index].name, PASSWORD_CMD) == 0) {
        CHATROOM_PASSWORD = optarg;
      } // end if 
      
      /*
      fprintf(stderr, "option: %s\n", long_options[opt_index].name);
      if (optarg) {
        fprintf(stderr, "with arg %s\n", optarg);
      } // end if
      */

    } // end if
  
  } // end loop
    
  fprintf(stderr, "Chatroom password: %s, Port: %d\n", CHATROOM_PASSWORD, SERVER_PORT);

  // create a socket
  // AF_INET -> IPv4
  // SOCK_STREAM -> a TCP connection
  // 0 -> assumed protocols from above two commands
  int listening_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (listening_socket == -1) { // error has occurred
    fprintf(stderr, "Socket creation failed!\n");
    exit(EXIT_FAILURE);
  } // end if

  // use a sockaddr in for giving to bind
  // this is the GENERAL LISTENING SOCKET!
  // not the connecting socket for a specific client.
  // https://man7.org/linux/man-pages/man3/sockaddr_in.3type.html
  // https://man7.org/linux/man-pages/man3/inet_addr.3p.html
  struct sockaddr_in address;
  address.sin_family = AF_INET; // same IPv4 family name
  address.sin_port = SERVER_PORT; // legit the port number
  address.sin_addr.s_addr = inet_addr(HOST_IP); // server addr, local host always


  // bind the address, port to the socket
  // socket -> the socket descriptor
  // address -> the sock addr made above with all connection info
  // size of the address -> need to pass
  // https://man7.org/linux/man-pages/man2/bind.2.html
  int bind_int = bind(
    listening_socket, 
    (struct sockaddr *) &address, 
    sizeof(address)
  );

  // error checking
  if (bind_int == -1) {
    fprintf(stderr, "Socket binding failed!\n");
    exit(EXIT_FAILURE);
  } // end if

  // then listen
  // mark socket as passive -- socket will be able to accept
  // incoming connection requests using accept
  // https://man7.org/linux/man-pages/man2/listen.2.html
  // listening_socket -> our socket file descriptor (sockfd)
  // backlog -> how many pending connections can grow in a queue
  //            for this socket.
  //            for right now, making 1? but play
  int listen_success = listen(listening_socket, 1);

  if (listen_success == -1) { // error occurred
    fprintf(stderr, "Listening setup failed!"); 
    exit(EXIT_FAILURE);
  } // end if

  fprintf(stdout, INIT_CONNECT_STR, SERVER_PORT);
  fflush(stdout);

  // server is now up and running, ready for connections

  char *username = (char *)malloc(USERNAME_BUFFER_SIZE * sizeof(char)); // max of 8 characters for username
  char *password = (char *)malloc(PASSWORD_BUFFER_SIZE * sizeof(char)); // max of 5 characters for password						     

  while (1) {
    // main process of the server:
    // await a new connection request
    // verify room in the chatroom
    // verify password
    // spin up new threads to handle this connection

    // then accept once knock on door -- likely threading here.
    // return file descriptor for a NEW client socket created
    // on connection accept.
    // original socket is unaffected by this call. 
    // will pull the first from the queue for connections.
    // if socket is not marked "non-blocking", this call blocks
    // the caller, hence the while true is a "quiet wait", not busy
    // https://man7.org/linux/man-pages/man2/accept.2.html
  
    // ignoring for now since on the same machine for this testing environment:	  
    // struct sockaddr_in client_address; // this will be filled in by accept with client info

    int client_socket = accept(
      listening_socket, 
      0, 
      0
    );

    // quick error check
    if (client_socket == -1) {
      fprintf(stderr, "Accepting of new client connection failed!\n");
      close(listening_socket);
      exit(EXIT_FAILURE);
    } // end if
      
    // server at capacity -- immediately reject connection with rejection string
    pthread_mutex_lock(&capacity_lock); // lock capacity count
    if (current_capacity == MAX_CLIENTS) {
      fprintf(stderr, "Server full!\n");
      send(client_socket, SERVER_FULL_STR, strlen(SERVER_FULL_STR), 0);
      close(client_socket); // clean up
      pthread_mutex_unlock(&capacity_lock);
      continue; // skip all the following iteration
    } // end if
      
    // we're not at capacity -- initiate a new connection
    current_capacity += 1;
    pthread_mutex_unlock(&capacity_lock); // release capacity count

    int ret = send(client_socket, USER_STR, strlen(USER_STR), 0); // send through req for username
    fprintf(stderr, "\nserver send ret: %d\n", ret); 

    ret = recv(client_socket, (void *)username, USERNAME_BUFFER_SIZE, 0); // await response
    // TODO: safety check for recv return	
    fprintf(stderr, "server recv ret: %d\n", ret);


    send(client_socket, PASS_STR, strlen(PASS_STR), 0); // send through req for password
    recv(client_socket, (void *)password, PASSWORD_BUFFER_SIZE, 0); // await response
    // TODO: safety check for recv return

    fprintf(stderr, "Client gave user/pass: %s, %s\n", username, password);

    // verify that the password is correct (nothing to verify about username)
    if (strcmp(password, CHATROOM_PASSWORD) != 0) {
      fprintf(stderr, "Incorrect password given.\n");
      send(client_socket, INCORRECT_PASS_STR, strlen(INCORRECT_PASS_STR), 0);
      close(client_socket); // clean up, close connection
      continue; // back to acepting
    } // end if

    // password correct, spin up new handlers for this connection
			
    fprintf(stderr, "Correct password!\n");
    char *welcome = "Welcome to chat!";
    send(client_socket, welcome, strlen(welcome), 0);

    // clear?
    memset(username, 0, USERNAME_BUFFER_SIZE);
    memset(password, 0, PASSWORD_BUFFER_SIZE);
    fprintf(stderr, "Client gave user/pass: %s, %s\n", username, password);
 
   

    /*

    int len_msg = 1; // control mechanism for this ingestion
    char *buffer = (char *)malloc(1025);
    char *msg = (char *)malloc(1025);

    while (len_msg > 0) {
      // for testing (this is what a thread should do instead): 
//      buffer = (char *) malloc(1025); // make a buffer for 1024 bytes (?) with one for null char at end of str?
      len_msg = recv(client_socket, (void *)buffer, 1025, 0); // quiet wait to receive a character string

      if (len_msg <= 0) {
        break;
      }

      fprintf(stderr, "Received from client: %s\n", buffer); // print raw buff
      // fflush(stdout); // need to flush here if debugging and using stdout 
  //    msg = (char *) malloc(1025);
      strcpy(msg, "Heard from server!");
      send(client_socket, msg, strlen(msg), 0);

    }
    
    free(buffer); free(msg); // clean up

    if (len_msg == 0) {
      // connecting client has shut down the connection on their end, end it.
      // TODO: clearly this needs to print something specific
      printf("Client exited.");
      close(client_socket); // close the connection
    } else {
      fprintf(stderr, "Error on recv!");
      printf("Error message: %s\n", strerror(errno)); 
      exit(2);
    }

    

    // TODO FROM HERE
    // check to see if space in client list (mutex)
    // if so:
    //   add to list, spin up a thread to handle their connection
    // else:
    //   reject the connection, avoiding the overhead of a client thread

    // client thread should hold all the handling of broadcasting changes, dealing with input of client, and closing the connection once :Exit typed.

    // TODO next: just implement the connection first and ensure the socket open/close is working correctly before getting in to the threads
     */
  }

  close(listening_socket);
  //clost(client_socket);
  free(username); free(password);

  return EXIT_SUCCESS;
}