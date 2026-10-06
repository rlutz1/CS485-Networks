/*
 * CS 485/585 - Fall 2026
 * Lab 1: TCP Chat Room
 *
 * client.c
 *
 * See the assignment PDF for the specification and
 * for the exact output strings the autograder expects.
 *
 * Your client must, at minimum:
 *   - parse --host, --port, --username and --password from the command line
 *   - connect to the server and complete whatever join exchange you design
 *   - print incoming messages while the user is still able to type
 *   - send what the user types, and terminate on :Exit
 *
 * Print nothing the specification does not call for. Send debugging output to
 * stderr instead, or remove it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h> // as suggested by instructions for arg parsing.
#include <time.h> // again, suggested in instructions, but wasn't in orgiinal starter

// command line pieces
#define HOST_CMD "host"
#define PORT_CMD "port"
#define USER_CMD "username"
#define PASS_CMD "password"

// standardized command line prints
#define SERVER_FULL_STR "Server full\n"
#define INCORRECT_PASS_STR "Incorrect password\n" // NOTE: this may need a period?
#define CONNECTION_STR "Connected to %s on port %d\n"

#define HAPPY_STR "[feeling happy]" // server side handles the newline for me here
#define SAD_STR "[feeling sad]"

// other helpfuls for special client commands
#define EXIT ":Exit"
#define HAPPY ":)"
#define SAD ":("
#define TIME ":mytime"
#define P1_TIME ":+1hr"
#define USERS ":Users"
#define P2P ":Msg"

// consistent buffer sizing. arbitrary.
#define HOST_IP "127.0.0.1"
#define SERVER_BUFFER_SIZE 301
#define CLIENT_BUFFER_SIZE 301

/**
 * =============================================================
 * UTILITY STRUCTS
 * =============================================================
 */

/**
 * the argument to be given to any server thread handler on start.
 * contains all information needed for them to carry out their tasks.
 */
struct ServerThreadArg {
  int receiving_socket; // the socket which a thread should be listening to.
  char *buffer; // buffer which to receive messages into on recv calls.
  int buffer_size; // the size of the above buffer for clarity.
};

/**
 * =============================================================
 * FUNCTION PROTOS
 * =============================================================
 */

int stream_recv(int sockfd, char *buffer, int buffer_size, int flags);
void *server_thread_handler(void * arg);

/**
 * =============================================================
 * MAIN CLIENT PROCESS
 * =============================================================
 */

int main(int argc, char *argv[])
{
    int ret; // for ret values of socket items

    char *SERVER_HOST = "";
    int SERVER_PORT = 0;
    char *USERNAME = "";
    char *CHATROOM_PASSWORD = "";

    // parse arguments
    int c = 0;
    int opt_index = 0;

    static struct option long_options[] = {
	          {HOST_CMD,     required_argument, 0,  0 },
            {PORT_CMD,     required_argument, 0,  0 },
	    	    {USER_CMD,     required_argument, 0,  0 },
            {PASS_CMD,     required_argument, 0,  0 },
            {           0,                 0, 0,  0 }
    };

    while (c >= 0) { // control mechanism for processing the options
      c = getopt_long(argc, argv, "", long_options, &opt_index); // get the next option index
      if (c == 0) {

        if (strcmp(long_options[opt_index].name, HOST_CMD) == 0) {
	        SERVER_HOST = optarg;
        } else if (strcmp(long_options[opt_index].name, PORT_CMD) == 0) {
            SERVER_PORT = atoi(optarg);
        } else if (strcmp(long_options[opt_index].name, USER_CMD) == 0) {
            USERNAME = optarg;
        } else if (strcmp(long_options[opt_index].name, PASS_CMD) == 0) {
            CHATROOM_PASSWORD = optarg;
        } // end if
    } // end if
  } // end loop

    // create a socket
    // AF_INET -> IPv4
    // SOCK_STREAM -> a TCP connection
    // 0 -> auto choose protocol -- sock_stream == tcp
    int connection_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (connection_socket == -1) { // error has occurred
      fprintf(stderr, "STDERR: Socket creation failed!");
      exit(EXIT_FAILURE);
    } // end if

    // to avoid the socket stuck in time wait.
    const int enable = 1;
    setsockopt(connection_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    struct sockaddr_in address; // this is the "client address"
    address.sin_family = AF_INET; // same IPv4 family name
    address.sin_port = htons(0); // MY port number -- 0 means give me whatever
    address.sin_addr.s_addr = inet_addr(HOST_IP); // server addr, local host always

    // bind the address, port to the socket
    // socket -> the socket descriptor
    // address -> the sock addr made above with all connection info
    // size of the address -> need to pass
    // https://man7.org/linux/man-pages/man2/bind.2.html
    ret = bind(
        connection_socket, 
        (struct sockaddr *) &address, 
        sizeof(address)
    );

    // error checking
    if (ret == -1) {
        fprintf(stderr, "STDERR: Socket binding failed!\n");
        exit(EXIT_FAILURE);
    } // end if

    // use a sockaddr in for giving to bind
    // this is the GENERAL LISTENING SOCKET!
    // not the connecting socket for a specific client.
    // TODO may need malloc here.
    // https://man7.org/linux/man-pages/man3/sockaddr_in.3type.html
    // https://man7.org/linux/man-pages/man3/inet_addr.3p.html
    struct sockaddr_in server_address; // this is the "server address"
    server_address.sin_family = AF_INET; // same IPv4 family name
    server_address.sin_port = htons(SERVER_PORT); // legit the port number
    server_address.sin_addr.s_addr = inet_addr(SERVER_HOST); // server addr, local host always
  
    // something like this: connect to the server socket (or attempt connection)
    // making a "tunnel" between my client socket and the server socket for send/receive.
    ret = connect(connection_socket, (struct sockaddr *) &server_address, sizeof(server_address));

    // error checking
    if (ret == -1) {
        fprintf(stderr, "STDERR: Socket connecting failed!\n");
        exit(EXIT_FAILURE);
    } // end if
    
    // now need to finish the setup connection
    // by listening for a server full or not.
    // connection treated as the initial send from client.  
    char *from_server = (char *)malloc(SERVER_BUFFER_SIZE * sizeof(char));
    if (!from_server) {
        fprintf(stderr, "STDERR: Malloc null return for from_server, exiting...\n");
        exit(EXIT_FAILURE);
    }// end if

    ret = stream_recv(connection_socket, from_server, SERVER_BUFFER_SIZE, 0); // await the receipt message
    if (ret <= 0) { // something wrong here
        close(connection_socket);
        free(from_server);
        exit(EXIT_FAILURE);
    } // connection closed or something went on 

    // if the server full, nothing to do here but print expected string
    if (!strcmp(from_server, SERVER_FULL_STR)) {
        fprintf(stdout, SERVER_FULL_STR); fflush(stdout); 
        close(connection_socket);
        free(from_server);
        exit(EXIT_SUCCESS);
    } // end if
    
    // clear buffer
    memset(from_server, 0, SERVER_BUFFER_SIZE);    
    
    // if server not full, we need to send our username, then password
    ret = send(connection_socket, (void *)CHATROOM_PASSWORD, strlen(CHATROOM_PASSWORD) + 1, 0);
    if (ret < 0) { 
        close(connection_socket);
        free(from_server);
        exit(EXIT_FAILURE);
    } // end if

    // await password confirmation
    ret = stream_recv(connection_socket, from_server, SERVER_BUFFER_SIZE, 0); // await the receipt message
    if (ret <= 0) { // something wrong here
        close(connection_socket);
        free(from_server);
        exit(EXIT_FAILURE);
    } // connection closed or something went on 

    // see if password was correct
    if (!strcmp(from_server, INCORRECT_PASS_STR)) {
        // if not correct, print incorrect and shut down.
        fprintf(stdout, INCORRECT_PASS_STR); fflush(stdout);
        close(connection_socket);
        free(from_server);
        exit(EXIT_FAILURE);
    } // end if

    // clear buffer
    memset(from_server, 0, SERVER_BUFFER_SIZE); 

    // print the required string that i am connected
    fprintf(stdout, CONNECTION_STR, SERVER_HOST, SERVER_PORT);

    // now, go ahead and send the user name, we must have given correct password
    ret = send(connection_socket, (void *)USERNAME, strlen(USERNAME) + 1, 0);
    if (ret < 0) { 
        close(connection_socket);
        free(from_server);
        exit(EXIT_FAILURE);
    } // end if

    // go ahead and wait to ensure the server has accomplished your spinup successfully
    ret = stream_recv(connection_socket, from_server, SERVER_BUFFER_SIZE, 0); // await the receipt message
    if (ret <= 0) { // something wrong here
        close(connection_socket);
        free(from_server);
        exit(EXIT_FAILURE);
    } // connection closed or something went on 

    fprintf(stderr, "STDERR: %s", from_server); fflush(stdout); // i joined the chatroom message, received but don't print.
    
    memset(from_server, 0, SERVER_BUFFER_SIZE); // clear buffer    

    // we should be allowed into the chatroom if all went well server side.
    // we can now spin up a thread to handle our server reception and listen
    // to std in ourselves.

    // create a new thread to listen to server and print to stdout
    struct ServerThreadArg *arg = (struct ServerThreadArg *)malloc(sizeof(struct ServerThreadArg));
    if (!arg) {
        fprintf(stderr, "STDERR: Malloc null return for arg, exiting...\n");
        close(connection_socket);
        free(from_server);
        exit(EXIT_FAILURE);
    } // end if

    // set up arg list
    arg->receiving_socket = connection_socket;
    arg->buffer = from_server;
    arg->buffer_size = SERVER_BUFFER_SIZE;

    // start up the handler thread.
    pthread_t thread;
    ret = pthread_create(&thread, NULL, server_thread_handler, arg);
    if (ret) {
        fprintf(stderr, "STDERR: pthread_create error! Exiting...");
        close(connection_socket);
        free(from_server); free(arg);
        exit(EXIT_FAILURE);
    } // end if

    ret = pthread_detach(thread); // instead of blocking with join, let os auto clean on thread finish.
    if (ret) {
        fprintf(stderr, "STDERR: pthread_detach error! Exiting...");
        close(connection_socket);
        free(from_server); free(arg);
        exit(EXIT_FAILURE);
    } // end if

    // create a new buffer for stdin
    char *from_client = (char *)malloc(CLIENT_BUFFER_SIZE * sizeof(char));
    if (!from_client) {
        fprintf(stderr, "STDERR: Malloc null return for from_client, exiting...\n");
        close(connection_socket);
        free(from_server); free(arg);
        exit(EXIT_FAILURE);
    }// end if

    /* MAIN LOOP */
    while (1) {
        // await client input from stdin
        fgets(from_client, CLIENT_BUFFER_SIZE, stdin);
        
        // stupid newline is included in fgets! hate it!
        from_client[strlen(from_client) - 1] = '\0';       

        char *temp;

        // process the input from client, sending through string to print to server thread.
        if (!strcmp(from_client, EXIT)) {
            fprintf(stderr, "STDERR: Client exiting...\n");
            // send exit so server can clean up
            ret = send(connection_socket, EXIT, strlen(EXIT) + 1, 0); 
            break; // we're ending the session

        } else if (!strcmp(from_client, HAPPY)) { 
            ret = send(connection_socket, HAPPY_STR, strlen(HAPPY_STR) + 1, 0);
            // if something went wrong, we have to end flow
            if (ret < 0) { break; }// end if

        } else if (!strcmp(from_client, SAD)) { 
            ret = send(connection_socket, SAD_STR, strlen(SAD_STR) + 1, 0);
            // if something went wrong, we have to end flow
            if (ret < 0) { break; }// end if

        } else if (!strcmp(from_client, TIME)) {
            // get the current time!
            time_t current_time;
            time(&current_time);
            char *time_str = ctime(&current_time);
            time_str[strlen(time_str) - 1] = '\0'; // strip newline
            
            // need to append this command to beginning of the sent string
            // because is the only one that should be sent to all including the sender.
            char msg[strlen(TIME) + strlen(time_str) + 1];
            snprintf(msg, sizeof(msg), "%s%s", TIME, time_str);          
    
            ret = send(connection_socket, msg, strlen(msg) + 1, 0);
            // if something went wrong, we have to end flow
            if (ret < 0) { break; }// end if

        } else if (!strcmp(from_client, P1_TIME)) { 
            // get current time plus an hour (3600 seconds)
            time_t current_time;
            time(&current_time);
            current_time += 3600;
            char *time_str = ctime(&current_time);
            time_str[strlen(time_str) - 1] = '\0'; // strip newline

            // need to append this command to beginning of the sent string
            // because is the only one that should be sent to all including the sender.
            char msg[strlen(TIME) + strlen(time_str) + 1];
            snprintf(msg, sizeof(msg), "%s%s", TIME, time_str);
            
            ret = send(connection_socket, msg, strlen(msg) + 1, 0);
            // if something went wrong, we have to end flow
            if (ret < 0) { break; }// end if
        
        }  else { 
            // this is either a general message to print to chat
            // OR we're sending through something special for server
            // to handle, like :Users or :Msg
            ret = send(connection_socket, from_client, strlen(from_client) + 1, 0);
            // if something went wrong, we have to end flow
            if (ret < 0) { break; }// end if

        }// end if

        // clear buffer for next message
        memset(from_client, 0, CLIENT_BUFFER_SIZE);
    } // end loop
    
    // we've broken out of loop for whatever reason.

    // free up resources
    free(from_server); free(from_client); free(arg);

    // ensure my socket is closed
    close(connection_socket);

    return EXIT_SUCCESS;
} // end main

/**
 * =============================================================
 * THREAD HANDLERS
 * =============================================================
 */ 

/**
 * handler for the thread that listens for server communication.
 * this is a DUMB thread. it simply prints what is received.
 * it is up to client thread server side to format things correctly
 * for autograder.
 */
void *server_thread_handler(void * arg) {
    struct ServerThreadArg *data = (struct ServerThreadArg *)arg;
    int ret;

    while(1) {
        // listen to sever
        ret = stream_recv(data->receiving_socket, data->buffer, data->buffer_size, 0); 
        // connection closed or something went on
        if (ret <= 0) { break; } // end if 

        // print to stdout -- note i'm the only thread doing this
        // so there is no locking protection here!
        fprintf(stdout, "%s", data->buffer); fflush(stdout);
        
        // clear the buffer
        memset(data->buffer, 0, data->buffer_size);

    } // end loop

    // don't do anything on clean up, main client thread will handle all resource frees.
    fprintf(stderr, "STDERR: Server thread shutting down...\n");
    return NULL;

} // end function

/**
 * =============================================================
 * UTILITY FUNCTIONS
 * =============================================================
 */

/**
 * this is to be used to wrap recv in order to use it for tcp STREAMS.
 */
int stream_recv(int sockfd, char *buffer, int buffer_size, int flags) {
    int bytes_received = 0; // bytes received so far
    int total_received = 0; // acts as offset into buffer.

    while (
        (bytes_received = recv(sockfd, (void *)buffer, buffer_size, flags)) > 0
    ) {
        total_received += bytes_received;
       
        // total received - 1 should be null if its the end of the message sent.
        if (!buffer[total_received - 1]) {
            break; // break out, process the message
        } // end if
    } // end loop
    return bytes_received; // return this value as normal recv would
} // end function