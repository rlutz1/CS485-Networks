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

// command line pieces
#define PORT_CMD "port"
#define PASSWORD_CMD "password"

// just something handy when setting up a new client.
#define ACK "ACK"

// things to print/send to client to print
#define INIT_CONNECT_STR "Server started on port %d. Accepting connections\n"
#define SERVER_FULL_STR "Server full\n"
#define INCORRECT_PASS_STR "Incorrect password\n" // NOTE: this may need a period?
#define NEW_PERSON_STR "%s joined the chatroom\n"
#define LEAVING_PERSON_STR "%s left the chatroom\n" 
#define GEN_MSG_STR "%s: %s\n"
#define ACTIVE_USERS_STR "Active Users: %s\n"
#define USER_REQ_MADE_STR "%s: searched up active users\n"
#define P2P_MSG_STR "[Message from %s]: %s\n"
#define P2P_OCURR_STR "%s: sent message to %s\n"

// special commands could receive from client to handle diff
#define EXIT ":Exit"
#define USERS ":Users"
#define TIME ":mytime"
#define P2P ":Msg"

// numerical and value constants to adhere to
#define HOST_IP "127.0.0.1"
#define MAX_CLIENTS 10
#define USERNAME_BUFFER_SIZE 9
#define PASSWORD_BUFFER_SIZE 6
#define CLIENT_BUFFER_SIZE 301
#define NO_EXCLUSIONS -1

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
 * FUNCTION PROTOS
 * =============================================================
 */

void broadcast(char *msg, int my_socket, struct ClientList *client_list);
void peer_to_peer(char *username, char *msg, struct ClientList *client_list);
void print_to_server(char *msg, pthread_mutex_t *server_out_lock);
void add_client(int new_socket, char *username, struct ClientList *client_list, pthread_mutex_t *server_out_lock);
void remove_client(int my_socket, struct ClientList *client_list);
struct ClientList *init_client_list();
void client_thread_cleanup(struct ClientThreadArg *arg);
//int is_peer_to_peer(char *msg);
void *client_thread_handler(void * arg);
int stream_recv(int sockfd, char *buffer, int buffer_size, int flags);

/**
 * =============================================================
 * GLOBALS
 * =============================================================
 */


/**
 * =============================================================
 * UTILITY FUNCTIONS
 * =============================================================
 */

/**
 * this is a general method for client threads to 
 * use when wanting to send a message to the general chatroom,
 * or all live clients in the chat that are NOT ME (my_socket)
 */
void broadcast(char *msg, int exclude_socket, struct ClientList *client_list) {
  pthread_mutex_lock(&(client_list->lock));
  
  struct Client *c;
  int ret;
 
  // dumb function: going to iterate through, and send message
  // to all non-null entries in client list.
  for (int i = 0; i < MAX_CLIENTS; i++) {
    c = client_list->clients[i];
    if (c) { // if a live client
        // use my_socket as a flag: if i give a legitimate socket value
        // it means EXCLUDE the message from my own client.
        if (exclude_socket >= 0) {
            if (c->sending_socket != exclude_socket) {
                fprintf(stderr, "STDERR: sending to not me!\n");
                ret = send(c->sending_socket, msg, strlen(msg) + 1, 0);
                // TODO: handle send error? likely just ignore.
            } // end if
        } else {
            // if the socket given is a false neg value, it means i want to
            // send to everyone, INCLUDING myself.
            ret = send(c->sending_socket, msg, strlen(msg) + 1, 0);
        } // end if
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
  int ret;

  // dumb function: going to iterate through, and send message
  // to all non-null entries in client list.
  for (int i = 0; i < MAX_CLIENTS; i++) {
    c = client_list->clients[i];

    if (c && !strcmp(username, c->username)) {
      // if a live client, send
      ret = send(c->sending_socket, msg, strlen(msg) + 1, 0);
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
  fprintf(stdout, "%s", msg); fflush(stdout);
  pthread_mutex_unlock(server_out_lock);
} // end function


/*
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
            fprintf(stderr, "STDERR: Malloc null return for client, exiting...\n");
            exit(EXIT_FAILURE);
        } // end if

        // init new client info
        new_client->sending_socket = new_socket;
        new_client->username = (char *)malloc((strlen(username) + 1) * sizeof(char));
        strcpy(new_client->username, username);

        // create new argument pack to give to the thread handler
        struct ClientThreadArg *arg = (struct ClientThreadArg *)malloc(sizeof(struct ClientThreadArg));
        if (!arg) {
            fprintf(stderr, "STDERR: Malloc null return for arg, exiting...\n");
            exit(EXIT_FAILURE);
        } // end if

        // init a buffer for receiving messages into
        char *buffer = (char *)malloc(CLIENT_BUFFER_SIZE * sizeof(char));
        if (!buffer) {
            fprintf(stderr, "STDERR: Malloc null return for buffer, exiting...\n");
            exit(EXIT_FAILURE);
        } // end if

        // set up entire argument list for client handler.
        arg->receiving_socket = new_socket;
        arg->username = (char *)malloc((strlen(username) + 1) * sizeof(char));
        strcpy(arg->username, username);
        arg->buffer = buffer;
        arg->buffer_size = CLIENT_BUFFER_SIZE;
        arg->server_out_lock = server_out_lock;
        arg->client_list = client_list;

        // add client to the list 
        client_list->clients[i] = new_client;

        // add to capacity
        client_list->current_capacity += 1;

        // start up the handler thread.
        pthread_t thread;
        int ret = pthread_create(&thread, NULL, client_thread_handler, arg);
        if (ret) {
            fprintf(stderr, "STDERR: pthread_create error! Exiting...\n");
            exit(EXIT_FAILURE);
        } // end if
        ret = pthread_detach(thread); // instead of blocking with join, let os auto clean on thread finish.
        if (ret) {
            fprintf(stderr, "STDERR: pthread_detach error! Exiting...\n");
            exit(EXIT_FAILURE);
        } // end if

        break; // done, new client added
    } // end if
  } // end loop

  pthread_mutex_unlock(&(client_list->lock));
} // end function


/**
 * this function will be used by ALL CLIENT THREADS
 * as a cleanup operation on ":Exit" command received
 * from the client socket. 
 * my_socket should be the receiving socket held by this client thread.
 */
void remove_client(int my_socket, struct ClientList *client_list) {
    pthread_mutex_lock(&(client_list->lock));
    
    struct Client *c;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        c = client_list->clients[i];

        if (c && c->sending_socket == my_socket) {
            // if a live client and myself, conduct cleanup
            client_list->clients[i] = 0; // null out my pointer
            free(c->username); // free username mem
            free(c); // free up this memory
            client_list->current_capacity -= 1; // add a capacity slot
            break; // clean myself up, exit
        } // end if
    } // end loop

    pthread_mutex_unlock(&(client_list->lock));
} // end function


/**
 * initialize function for client list for cleanliness -- to be called by the 
 * main server process on start up.
 */
struct ClientList *init_client_list() {
    struct ClientList *client_list = (struct ClientList*)malloc(sizeof(struct ClientList));
    
    // ensure all clients are null pointers for the start
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_list->clients[i] = 0;
    } // end loop

    // set initial capacity to 0
    client_list->current_capacity = 0;

    // initialize the lock for the client list
    pthread_mutex_init(&(client_list->lock), NULL);

    return client_list;  
} // end function

/**
 * build a comma separated string of all usernames currently active.
 */
char *build_user_list(struct ClientList *client_list) {
    pthread_mutex_lock(&(client_list->lock));

    // first we're going to grab all pointers to the user names.
    char **user_list = (char **)calloc(MAX_CLIENTS, sizeof(char *));
    if (!user_list) {
        fprintf(stderr, "STDERR: calloc failed in building user list!\n");
        exit(EXIT_FAILURE);
    } // end if

    struct Client *c;
    int user_i = 0;
    int user_lens = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        c = client_list->clients[i];

        if (c) { // if a live client
            // grab their usernme ptr
            user_list[user_i] = c->username;
            user_i++;
            // grab also the length of this username for later.
            user_lens += strlen(c->username);
        } // end if
    } // end loop

    // should now have all active users' names.
    // now for the fun part: piecing together into a comma separated string.

    char *user_list_str = calloc(
        user_lens // the usernames
        + (2 * (user_i - 1)) // the comma and spaces separating intermediate names 
        + 1, // ending null character
        sizeof(char)
    );
    if (!user_list_str) {
        fprintf(stderr, "STDERR: calloc failed in building user list!\n");
        exit(EXIT_FAILURE);
    } // end if

    char *temp = user_list_str; // a walking pointer.
    
    // now, build the string
    for (int i = 0; i < user_i - 1; i++) {
        strcpy(temp, user_list[i]); // copy in the username
        temp += strlen(user_list[i]); // move pointer to end of this string
        *temp = ','; *(temp + 1) = ' '; temp += 2; // add a ", ", move ptr
    } // end loop

    // final username copied in with no comma/space
    strcpy(temp, user_list[user_i - 1]);

    free(user_list); // free up the resource
    pthread_mutex_unlock(&(client_list->lock));

    return user_list_str; // server thread will use this and free
} // end function





/**
 * given a username sent through by client, find the corresponding
 * socket if exists.
 */
int get_socket_by_username(char *username, struct ClientList* client_list) {
    pthread_mutex_lock(&(client_list->lock));
    int target_socket = -1;

    struct Client *c;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        c = client_list->clients[i];
        if (c && !strcmp(username, c->username)) {
            target_socket = c->sending_socket;
            break;
        } // end if
    } // end loop

    pthread_mutex_unlock(&(client_list->lock));
    return target_socket;
} // end function

/**
 * =============================================================
 * THREAD HANDLERS
 * =============================================================
 */ 

/**
 * convenience method to handle all the cleanup tasks on client thread end exec.
 */
void client_thread_cleanup(struct ClientThreadArg *arg) {
    remove_client(arg->receiving_socket, arg->client_list); // remove myself from the client list
    free(arg->buffer); // clean up my dynamalloc
    free(arg->username);
    free(arg);
} // end function


/**
 * the main handler for a client connection manager thread.
 */
void *client_thread_handler(void * arg) {
    struct ClientThreadArg *data = (struct ClientThreadArg *)arg;

    while(1) { // this is my forever job until connection closes for whatever reason.
        
        // i exist to read input from the client and take appropriate actions.
        
        // first, read in recv, blocking on the call until something occurs.
        // need to read recv in loop because TCP is a STREAM, and not all of message guaranteed at once.

        int ret = stream_recv(data->receiving_socket, data->buffer, data->buffer_size, 0);    
        if (ret <= 0) { // strangeness check, shouldn't get here if client shuts down gracefully.
            fprintf(stderr, "STDERR: Connection closed unexpectedly for client thread. Shutting down...\n");
            client_thread_cleanup(data); // cleanup!
            break; // end the while (1)
        } // end if 

        // we hypothetically have a complete message from client in our buffer now.
        fprintf(stderr, "STDERR: Client thread received: %s\n", data->buffer); // TODO
        char *temp;        

        if ((temp = strstr(data->buffer, TIME)) && (temp - data->buffer == 0)) { // if :mytime is at the beginning of the msg
            temp += (strlen(TIME)); // dont include the :mytime header
            char msg[strlen(GEN_MSG_STR) + strlen(data->username) + strlen(temp)+ 1];
            snprintf(msg, sizeof(msg), GEN_MSG_STR, data->username, temp);
            
            // send as general message, but don't exclude myself!
            broadcast(msg, NO_EXCLUSIONS, data->client_list); // send out to all clients.
            print_to_server(msg, data->server_out_lock); // send to the server output.

        } else if ((temp = strstr(data->buffer, P2P)) && (temp - data->buffer == 0)) {
            // if we get :Msg [username] [message here], need to parse and format.
 //            P2P_MSG_STR "[Message from %s]: %s\n"
            //#define P2P_OCURR_STR 
            temp += (strlen(P2P)); // over shoot
            if (!(*temp) || !(*(temp + 1))) { continue; } // ignore if just ":Msg" or ":Msg " sent through
            temp++; // hop over the space in ":Msg "
       
            char *to = (char *)calloc(strlen(temp), sizeof(char));// make a sending username slot
            if (!to) {
                fprintf(stderr, "STDERR: calloc failed in P2P!\n");
                exit(EXIT_FAILURE);
            } // end if
            
            int i = 0;
            while (*temp != ' ' && *temp != '\0') { // collect everything before next space or null
                to[i] = *temp;
                i++; temp++;
            } // end loop
            to[i] = '\0'; // end the string
            temp++; // move to the message

            int peer_socket = get_socket_by_username(to, data->client_list);
            if (peer_socket >= 0) { // all went well, found username.
                char msg[strlen(data->username) + strlen(temp) + strlen(P2P_MSG_STR) + 1];
                snprintf(msg, sizeof(msg), P2P_MSG_STR, data->username, temp); // send to peer
                int ret = send(peer_socket, msg, strlen(msg) + 1, 0); // send only through to receiver.
        
                // print special to server
                char for_server[strlen(data->username) + strlen(to) + strlen(P2P_OCURR_STR) + 1];   
                snprintf(for_server, sizeof(for_server), P2P_OCURR_STR, data->username, to);
                print_to_server(for_server, data->server_out_lock);
            } // end if
       
            free(to); // free resource

        } else if (!strcmp(data->buffer, USERS)) {    
            // :Users will also be sent through like :Exit to handle!
            // need to build the current list of users string and then send through the msg
            char *user_list = build_user_list(data->client_list);
            char msg[strlen(user_list) + strlen(ACTIVE_USERS_STR) + 1];
            snprintf(msg, sizeof(msg), ACTIVE_USERS_STR, user_list);

            char for_server[strlen(data->username) + strlen(USER_REQ_MADE_STR) + 1];
            snprintf(for_server, sizeof(for_server), USER_REQ_MADE_STR, data->username);

            int ret = send(data->receiving_socket, msg, strlen(msg) + 1, 0); // send only through to me.
            print_to_server(for_server, data->server_out_lock); // special print to server
          
            free(user_list); // need to free this post-use.

        } else if (!strcmp(data->buffer, EXIT)) {            
            // buid leaving string
            char msg[strlen(data->username) + strlen(LEAVING_PERSON_STR) + 1];
            snprintf(msg, sizeof(msg), LEAVING_PERSON_STR, data->username);
            
            // let everyone know i'm leaving
            broadcast(msg, data->receiving_socket, data->client_list);
            print_to_server(msg, data->server_out_lock);
  
            // client is exiting chat 
            client_thread_cleanup(data); // cleanup my stuff
          
            break; // this client thread is done, client has left the chat
        } else { // TODO: maybe use sn printf to be cleaner
            // this is a general broadcast message. send to all live clients.
            char msg[strlen(GEN_MSG_STR) + strlen(data->username) + strlen(data->buffer)+ 1];
            snprintf(msg, sizeof(msg), GEN_MSG_STR, data->username, data->buffer);
     
            broadcast(msg, data->receiving_socket, data->client_list); // send out to all clients.
            print_to_server(msg, data->server_out_lock); // send to the server output.
        } // end if
        
        // clear out the buffer!
        memset(data->buffer, 0, data->buffer_size);     

    } // end loop 

    return NULL;
} // end function

/**
 * this is to be used to wrap recv in order to use it for tcp STREAMS.
 */
int stream_recv(int sockfd, char *buffer, int buffer_size, int flags) {
    int bytes_received = 0; // bytes received so far
    int total_received = 0; // acts as offset into buffer.

    while (
        (bytes_received = recv(sockfd, (void *)buffer, buffer_size, 0)) > 0
    ) {
        total_received += bytes_received;
        // total_received - 1 should be null if its the end of the message sent.
        if (!buffer[total_received - 1]) {
            break; // break out, process the message
        } // end if
    } // end loop

    return bytes_received; // return this value as normal recv would
} // end function


// TODO: graceful kill_client_list() exit. not mentioned in pdf, but would be good practice if time.
// TODO: graceful error exit that frees all memory allocated by process

/**
 * =============================================================
 * MAIN SERVER PROCESS
 * =============================================================
 */

int main(int argc, char *argv[])
{
    int ret; // for ret values of socket items

    // set up some helpful values to hold on to
    char *CHATROOM_PASSWORD = "";
    int SERVER_PORT = -1;
    
    // set up stdout protector
    pthread_mutex_t server_out_lock;
    pthread_mutex_init(&server_out_lock, NULL);


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
        } // end if
    } // end loop
    
    fprintf(stderr, "STDERR: Chatroom password: %s, Port: %d\n", CHATROOM_PASSWORD, SERVER_PORT);

    // create a socket
    // AF_INET -> IPv4
    // SOCK_STREAM -> a TCP connection
    // 0 -> assumed protocols from above two commands
    int listening_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (listening_socket == -1) { // error has occurred
        fprintf(stderr, "STDERR: Socket creation failed!\n");
        exit(EXIT_FAILURE);
    } // end if

    // use a sockaddr in for giving to bind
    // this is the GENERAL LISTENING SOCKET!
    // not the connecting socket for a specific client.
    // https://man7.org/linux/man-pages/man3/sockaddr_in.3type.html
    // https://man7.org/linux/man-pages/man3/inet_addr.3p.html
    struct sockaddr_in address;
    address.sin_family = AF_INET; // same IPv4 family name
    address.sin_port = htons(SERVER_PORT); // legit the port number
    address.sin_addr.s_addr = inet_addr(HOST_IP); // server addr, local host always

    const int enable = 1;
    setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    // bind the address, port to the socket
    // socket -> the socket descriptor
    // address -> the sock addr made above with all connection info
    // size of the address -> need to pass
    // https://man7.org/linux/man-pages/man2/bind.2.html
    ret = bind(
        listening_socket, 
        (struct sockaddr *) &address, 
        sizeof(address)
    );

    // error checking
    if (ret == -1) {
        fprintf(stderr, "STDERR: Socket binding failed!\n");
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
    ret = listen(listening_socket, 1);
    if (ret == -1) { // error occurred
        fprintf(stderr, "STDERR: Listening setup failed!\n"); 
        exit(EXIT_FAILURE);
    } // end if

    // TODO: print_to_server instead, but okay here since no threads yet.
    fprintf(stdout, INIT_CONNECT_STR, SERVER_PORT); fflush(stdout);

    // server is now up and running, ready for connections
    // init the client list.
    // TODO: i should have a kill_client_list as well to clean this up.
    struct ClientList * client_list = init_client_list();
  
    char *username_buffer = (char *)malloc(USERNAME_BUFFER_SIZE * sizeof(char)); // max of 8 characters for username
    char *password_buffer = (char *)malloc(PASSWORD_BUFFER_SIZE * sizeof(char)); // max of 5 characters for password						     

    while (1) {
        // main process of the server:
        // await a new connection request
        // verify room in the chatroom
        // verify password
        // spin up new thread to handle this connection

        // accept once knock on door -- likely threading here.
        // return file descriptor for a NEW client socket created
        // on connection accept.
        // original socket is unaffected by this call. 
        // will pull the first from the queue for connections.
        // if socket is not marked "non-blocking", this call blocks
        // the caller, hence the while true is a "quiet wait", not busy
        // https://man7.org/linux/man-pages/man2/accept.2.html
  
        // ignoring for now since on the same machine for this testing environment:	  
        // struct sockaddr_in client_address; // this will be filled in by accept with client info

        int client_socket = accept(listening_socket, 0, 0);

        // quick error check
        if (client_socket == -1) {
            fprintf(stderr, "STDERR: Accepting of new client connection failed!\n");
            continue; // allow continuation of server if this failed
        } // end if
      
        // server at capacity -- immediately reject connection with rejection string
        pthread_mutex_lock(&(client_list->lock)); // lock capacity count
        if (client_list->current_capacity == MAX_CLIENTS) {
            fprintf(stderr, "STDERR: Server full!\n");
            ret = send(client_socket, SERVER_FULL_STR, strlen(SERVER_FULL_STR) + 1, 0);
            close(client_socket); // clean up
            pthread_mutex_unlock(&(client_list->lock)); // unlock the lock
            continue; // skip all the following iteration
        } // end if
      
        // we're not at capacity -- initiate a new connection
        // client_list->current_capacity += 1; // NOTE: moving to add_client
        pthread_mutex_unlock(&(client_list->lock)); // release client list
        
        // send through a dummy to release the client recv hang
        ret = send(client_socket, ACK, strlen(ACK) + 1, 0);
        if (ret < 0) { 
            close(client_socket);
            memset(username_buffer, 0, USERNAME_BUFFER_SIZE);
            memset(password_buffer, 0, PASSWORD_BUFFER_SIZE);   
            continue;
        } // end if
        
        // expect password next
        ret = stream_recv(client_socket, password_buffer, PASSWORD_BUFFER_SIZE, 0);       
        if (ret <= 0) { // something wrong here
            close(client_socket);
            memset(username_buffer, 0, USERNAME_BUFFER_SIZE);
            memset(password_buffer, 0, PASSWORD_BUFFER_SIZE);
            continue;
        } // connection closed or something went on 

        // verify that the password is correct (nothing to verify about username)
        if (strcmp(password_buffer, CHATROOM_PASSWORD)) {
            fprintf(stderr, "STDERR: Incorrect password given.\n"); fflush(stderr);
            ret = send(client_socket, INCORRECT_PASS_STR, strlen(INCORRECT_PASS_STR) + 1, 0);
            close(client_socket); // clean up, close connection
            continue; // back to acepting
        } // end if
        
        // password was correct, send a simple ack
        ret = send(client_socket, ACK, strlen(ACK) + 1, 0);
        if (ret < 0) { 
            close(client_socket);
            memset(username_buffer, 0, USERNAME_BUFFER_SIZE);
            memset(password_buffer, 0, PASSWORD_BUFFER_SIZE);
            continue;
        } // end if

        // password all good need a username now
        ret = stream_recv(client_socket, username_buffer, USERNAME_BUFFER_SIZE, 0); 
        if (ret <= 0) { // something wrong here
            close(client_socket);
            memset(username_buffer, 0, USERNAME_BUFFER_SIZE);
            memset(password_buffer, 0, PASSWORD_BUFFER_SIZE);
            continue;
        } // connection closed or something went on 

        // next, spin up thread to handle this client
        // client is waiting on recv during this period to be safe.
        // make a thread to handle this client.
        add_client(client_socket, username_buffer, client_list, &server_out_lock);

        // print both to server and all other clients that new person entered.
        char str_builder[strlen(NEW_PERSON_STR) + strlen(username_buffer) + 1]; // arbitrary for now
        snprintf(str_builder, sizeof(str_builder), NEW_PERSON_STR, username_buffer);
        
        print_to_server(str_builder, &server_out_lock);
        broadcast(str_builder, NO_EXCLUSIONS, client_list); // this will release the hold on the client side.

        // clear these buffers
        memset(username_buffer, 0, USERNAME_BUFFER_SIZE);
        memset(password_buffer, 0, PASSWORD_BUFFER_SIZE);

        // go back to accepting
  } // end loop

  // NOTE: there's no graceful exit here! these are just placeholders
  close(listening_socket);
  free(username_buffer); free(password_buffer); free(client_list);

  return EXIT_SUCCESS;
}