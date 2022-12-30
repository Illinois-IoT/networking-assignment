/**
IoT Summer Camp 2023
Networking Assignment
**/
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <stddef.h>


// ./client sp22-cs241-303.cs.illinois.edu:3456 PUT deeddddd.txt daddd.txt
// ./client sp22-cs241-303.cs.illinois.edu:3456 GET deed.txt break.txt


typedef enum { GET, PUT, DELETE, V_UNKNOWN } verb;

char **parse_args(int argc, char **argv);
verb check_args(char **args);
int connect_to_server(char *host, char *port);
void write_to_server(char** cmds, verb operation, int socket_fd);
void read_from_server(char* local, verb operation, int socket_fd);
size_t write_all_to_socket(int socket, const char *buffer, size_t count);
size_t read_all_from_socket(int socket, char *buffer, size_t count);
size_t write_message_size(size_t size, int socket);
size_t get_message_size(int socket);
void print_client_help();

int main(int argc, char **argv) {

    //parse args: in form of {host0, port1, method2, remote3, local4, NULL}
    char ** cmds = parse_args(argc, argv);  //need free

    //check args - inside will check for NULL of the argument
    verb operation = check_args(cmds);

    //connect to server
    int socket_fd = connect_to_server(cmds[0], cmds[1]);

    //write to server
    write_to_server(cmds, operation, socket_fd);

    //read from server
    read_from_server(cmds[4], operation, socket_fd);

    //close socket_fd
    close(socket_fd);
    
    //free
    free(cmds);

    return 0;
}



/**
 * This function write the request to the server, following certain format corresponding to certain request type.
 * 
 * char** cmds: {host0, port1, method2, remote3, local4, NULL}
 * verb operation: request type
 * int socket_fd: socket fd returned from "connect_to_server()"
 */
void write_to_server(char** cmds, verb operation, int socket_fd) {
    char* msg = NULL;       //need free

    //1. write header to server
    switch(operation){
    case GET: {
        msg = calloc(1, strlen(cmds[2]) + strlen(cmds[3]) + 3); // ' ' & '\n' & '\0'
        sprintf(msg, "%s %s\n", cmds[2], cmds[3]);
        break;

    }
    case PUT: {
        msg = calloc(1, strlen(cmds[2]) + strlen(cmds[3]) + 3); // ' ' & '\n' & '\0'
        sprintf(msg, "%s %s\n", cmds[2], cmds[3]);
        break;
    }
    case DELETE: {
        msg = calloc(1, strlen(cmds[2]) + strlen(cmds[3]) + 3); // ' ' & '\n' & '\0'
        sprintf(msg, "%s %s\n", cmds[2], cmds[3]);
        break;
    }
    default: {/*should not reach this line*/}
    }
    ssize_t write_ret = write_all_to_socket(socket_fd, msg, strlen(msg));
    if (write_ret == -1) exit(1);
    if (write_ret == 0) {
        printf("lost connection\n");
        exit(1);
    }

    //2. Only PUT have [File size][Binary Data]
    if (operation == PUT) {
        // ?????requirement: Your client needs to be able to handle large files (more than physical RAM) and should do so efficiently.?????? -- More Complicated Constraint

        //open file - handle error - r: The stream is positioned at the beginning of the file.
        FILE* file = fopen(cmds[4], "r");   //need close
        if (!file) {
            //On PUT, if local file does not exist, simply exiting is okay.
            exit(1);
        }

        //write [size]
        struct stat file_info;
        stat(cmds[4], &file_info);
        size_t count = file_info.st_size;
        ssize_t msg_write_ret = write_message_size(count, socket_fd);
        if (msg_write_ret == -1) exit(1);
        if(msg_write_ret == 0) {
            printf("connection closed\n");
            exit(1);
        }

        //write [binary data]
        size_t num_write = 0;
        char buffer[4096+1];
        while(num_write < count) {
            memset(buffer, 0, 4096+1);
            ssize_t size = count-num_write;
            if (size > 4096) size = 4096;    //for large file
            fread(buffer, size, 1, file);
            ssize_t temp_size = write_all_to_socket(socket_fd, buffer, size);
            if (temp_size == -1) exit(1);
            if (temp_size == 0) {
               printf("connection closed\n");
                exit(1);
            }
            num_write += temp_size;
        }

        //close file
        fclose(file);
    }

    //3. cleanup
    //free msg
    free(msg);
    //shutdown: Once the client has sent all the data to the server, it should perform a ‘half close’ by closing the write half of the socket
    if (shutdown(socket_fd, SHUT_WR) != 0) perror(NULL);
}


/**
 * This function read the response from the server, following certain format provided by the doc
 * 
 * char* local: local filename
 * verb operation: request type
 * int socket_fd: socket fd returned from "connect_to_server()"
 */
void read_from_server(char* local, verb operation, int socket_fd) {
    //check RESPONSE
    char * response = malloc(6+1);    //need free
    memset(response, 0, 7);
    
    ssize_t read_response_ret = read_all_from_socket(socket_fd, response, 3);
    if (read_response_ret == 0) {
        printf("connection closed\n");
        exit(1);
    }
    if (read_response_ret == -1) {
        printf("connection closed\n");
        exit(1);
    }

    //1. OK
    if (strcmp(response, "OK\n") == 0) {
        //PUT & DELETE - finish
        if (operation == PUT || operation == DELETE) {
            printf("PUT/DELETE Success\n");
            free(response);
            if (shutdown(socket_fd, SHUT_RD) != 0) perror(NULL);
            return;
        }
        //GET
        if (operation == GET) {
            //read [size]
            ssize_t count = get_message_size(socket_fd);

            //open local file to write - handle error
            //Truncate file to zero length or create text file for writing. 
            //a) The stream is positioned at the beginning of the file.
            //b) Any created files will have mode S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
            FILE* file = fopen(local, "w");   //need close

            //read [binary data]
            ssize_t num_read = 0;
            char buffer[4096+1];
            while(num_read < count) {
                memset(buffer, 0, 4096+1);
                ssize_t size = count-num_read;
                if (count-num_read > 4096) size = 4096;    //for large file
                ssize_t temp_size  = read_all_from_socket(socket_fd, buffer, size);
                if (temp_size == -1) {
                    printf("Invalid Response\n");
                    exit(1);
                }
                if (temp_size == 0) {
                    printf("Connection Closed \n");
                    if (num_read < count) printf("Too Little Data\n");
                    exit(1);
                }
                num_read += temp_size;  
                //write to file
                fwrite(buffer, temp_size, 1, file);
            }
            //close file
            fclose(file);

            //check for too much data ---- More Complicated Constraint
            size_t temp_size  = read_all_from_socket(socket_fd, buffer, 2);
            if (temp_size > 0) printf("Recieve Too Many Data\n");
        }
    } else {
        read_all_from_socket(socket_fd, response+3, 3);
        //2. non-exist STATUS
        if (strcmp(response, "ERROR\n") != 0) {
            printf("Invalid Response");
            exit(1);
        } 
        //3. ERROR - header will not larger than 1024 
        printf("ERROR happen when server receive the request\n");
    }

    //free response
    free(response);
    // shutdown for read:
    if (shutdown(socket_fd, SHUT_RD) != 0) perror(NULL);
}



/**
 * This function build the connection with the server through the "three steps".
 * 
 * char* host: the host name
 * char* port: port number
 * 
 * Returns socket fd on success, else the program quit with a status 1
 */
int connect_to_server(char *host, char *port) {
    struct addrinfo hints;
    struct addrinfo *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    int s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
        //print message
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    }
    int socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_fd == -1) {
        //print message
        perror(NULL);
        exit(1);
    }
    int connect_result = connect(socket_fd, result->ai_addr, result->ai_addrlen);
    if (connect_result == -1) {
        //print message
        perror(NULL);
        exit(1);
    }
    freeaddrinfo(result);
    return socket_fd;
}

/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL) {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp) {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3) {
        args[3] = argv[3];
    }
    if (argc > 4) {
        args[4] = argv[4];
    }

    return args;
}

/**
 *
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args) {
    if (args == NULL) {
        printf("./client <host>:<port> <method> [remote] [local]\n");
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "GET") == 0) {
        if (args[3] != NULL && args[4] != NULL) {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0) {
        if (args[3] != NULL) {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0) {
        if (args[3] == NULL || args[4] == NULL) {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}

/**
 * Reads the bytes of size to the socket
 *
 * Returns the number of bytes successfully read,
 * 0 if socket is disconnected, or -1 on failure
 */
// You may assume size won't be larger than a 4 byte integer
size_t get_message_size(int socket) {
    size_t size;
    size_t read_bytes =
        read_all_from_socket(socket, (char *)&size, sizeof(size_t));
    if (read_bytes == 0 || (int)read_bytes == -1) {
        printf("Invalid Response\n");
        exit(1);
    }

    return (size_t)(size);
}

/**
 * Writes the bytes of size to the socket
 *
 * Returns the number of bytes successfully written,
 * 0 if socket is disconnected, or -1 on failure
 */
// You may assume size won't be larger than a 4 byte integer
size_t write_message_size(size_t size, int socket) {
    size_t write_bytes =
        write_all_to_socket(socket, (char *)&size, sizeof(size_t));
    return write_bytes;
}

/**
 * Attempts to read all count bytes from socket into buffer.
 * Assumes buffer is large enough.
 *
 * Returns the number of bytes read, 0 if socket is disconnected,
 * or -1 on failure.
 */
size_t read_all_from_socket(int socket, char *buffer, size_t count) {
    size_t num_read = 0;
    while(num_read < count) {
        ssize_t ret = read(socket, buffer, count-num_read);
        if (ret == 0) return num_read;
        if (ret == -1 && errno == EINTR) {
            continue;
        }
        if (ret == -1) return -1;
        num_read += ret;
        buffer += ret;
    }
    return num_read;
}


/**
 * Attempts to write all count bytes from buffer to socket.
 * Assumes buffer contains at least count bytes.
 *
 * Returns the number of bytes written, 0 if socket is disconnected,
 * or -1 on failure.
 */
size_t write_all_to_socket(int socket, const char *buffer, size_t count) {
    size_t num_write = 0;
    while(num_write < count) {
        ssize_t ret = write(socket, buffer, count-num_write);
        if (ret == 0) return num_write;
        if (ret == -1 && errno == EINTR) {
            continue;
        }
        if (ret == -1) return -1;
        num_write += ret;
        buffer += ret;
    }
    return num_write;
}

/**
 * This is a client help message regarding the client usage and the request types.
 */

void print_client_help() {
    printf("./client <host>:<port> <method> [remote] [local]\n");
    printf("Methods:\n \
        PUT <remote> <local>\tUploads <local> file to serve as filename <remote>.\n \
        GET <remote>\t\tDownloads file named <remote> from server.\n \
        DELETE <remote>\tDeletes file named <remote> on server.\n");
}
