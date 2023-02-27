/**
IoT Summer Camp 2023
Networking Assignment
**/
#include <stdio.h>
#include "helper.c"
#include "includes/dictionary.h"
#include "./includes/vector.h"

#include <signal.h>
#include <string.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// ERROR on file not exists


//./client sp23-cs341-adm.cs.illinois.edu:3456 GET random1.txt random2.txt


#define BUFFER_SIZE 1024
#define HEADER_SIZE 20
#define MAX_CLIENTS 1

static int socket_fd;
static int client_fd;

static char* directory_name;

typedef enum { GET, PUT, DELETE, LIST, V_UNKNOWN } verb;

//for client
static verb request;
static char* filename;  //need free
size_t file_size;
FILE* file;

void close_server();
int run_server(char *port);
void handle_client(int client_fd);
void write_error(int client_fd);
void read_header(int client_fd);
void delete_file_from_dir(int client_fd);
void read_data(int client_fd);
void read_size(int client_fd);
void write_ok_preprocess(int client_fd);
void write_ok(int client_fd);
void finish_serving(int client_fd);
void clean_up_client(int client_fd);
void write_size(int client_fd);
void write_data(int client_fd);
void write_error_preprocess(int lient_fd, int error_type);

int main(int argc, char **argv) {
    // good luck!

    //check argv & get port
    if (argc != 2) {
        print_server_usage();
        exit(1);
    }

    //mktempdir - print message
    char template[] = "XXXXXX";
    directory_name = mkdtemp(template);
    fprintf(stdout, "%s\n", temp_directory); //print temp dir
    
    // build server
    socket_fd = run_server(argv[1]);

    // handle client
    handle_client(client_fd);

    //close server
    close_server();
    
}

void SIGPIPE_handler(int s) { /* do nothing */ }
void SIGINT_handler(int s) {
    //Your server should exit on receiving SIGINT.
    close_server();
}

void close_server() {
    //clean up : file in the dir, dir, vetor, dictionary, everything inside client_state
    rmdir(directory_name);

    //shutdown fd
    if (shutdown(client_fd, SHUT_RDWR)){
        perror(NULL);
        exit(1);
    }
    //close client_fd
    close(client_fd);
    
    //shutdown - abstract socket_fd
    if (shutdown(socket_fd, SHUT_RDWR)){
        perror(NULL);
        exit(1);
    }
    //close socket_fd
    close(socket_fd);
}



int run_server(char *port) {
    struct addrinfo hints;
    struct addrinfo * result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE;
    int s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    }
    socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_fd == -1) {
        perror(NULL);
        exit(1);
    }
    if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror(NULL);
        exit(1);
    }
    if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) < 0) {
        perror(NULL);
        exit(1);
    }
    if (bind(socket_fd, result->ai_addr, result->ai_addrlen) != 0) {
        perror(NULL);
        exit(1);
    }
    if(listen(socket_fd, MAX_CLIENTS) != 0) {
        perror(NULL);
        exit(1);
    }
    freeaddrinfo(result);
    client_fd = accept(socket_fd, NULL, NULL);
    return socket_fd;
}


void handle_client(int client_fd) {
    header = calloc(HEADER_SIZE,1); 
    read_header(client_fd);
}


void read_header(int client_fd) {
    ssize_t read_header_ret = server_read_all_header(client_fd, header, HEADER_SIZE);
    if (read_header_ret == -1) {
        flag = -1;
    }

    int flag = 1;
    // parse the header
    // do things based on the REQUEST
    char* request = strtok(header, " ");
    char* filename = strtok(NULL, " ");
    if (!strcmp(request, "GET")) {
        request = GET;
        filename = strdup(filename);
        write_ok_preprocess(client_fd);
    } else if (!strcmp(request, "PUT")) {
        request = PUT;
        filename = strdup(filename);
        read_size(client_fd);
    } else if (!strcmp(request, "DELETE")) {
        request = DELETE;
        filename = strdup(filename);
        write_ok_preprocess(client_fd);
    }
}



void write_ok_preprocess(int client_fd) {
    switch(request) {
        case GET: {
            char * pathname = calloc(1, strlen(directory_name)+strlen(filename)+2); //'/' & '\0'   //need free!!!
            sprintf(pathname, "%s/%s", directory_name, filename);
            file = fopen(pathname, "r");
            free(pathname);
            if(!file) {
                write_error_preprocess(client_fd);
                return;
            }
            //get filesize
            fseek(file, 0, SEEK_END);
            file_size = ftell(file);
            rewind(file);
            
            break;
        }
        case PUT: {
            break;
        }
        case DELETE: {
            //check valid file
            char * pathname = calloc(1, strlen(directory_name)+strlen(filename)+2);
            sprintf(pathname, "%s/%s", directory_name, filename);
            FILE* temp = fopen(pathname, "r");
            if(!temp) {
                write_error_preprocess(client_fd);
                return;
            }
            fclose(temp);
            free(pathname);

            //now find it - delete
            delete_file_from_dir(client_fd); 
            break;
        }
        default: {/*should not reach here*/}
    }

    //write ok
    write_ok(client_fd);
}


void read_size(int client_fd) {
    //only for PUT
    file_size = get_message_size(client_fd);
    //open file for write
    char * pathname = calloc(1, strlen(directory_name)+strlen(filename)+2); //'/' & '\0'   //need free!!!
    sprintf(pathname, "%s/%s", directory_name, filename);
    file = fopen(pathname, "w");
    free(pathname);
    read_data(client_fd);
}

void read_data(int client_fd) {
    //only for PUT
    ssize_t num_read = 0;
    char buffer[4096+1];
    ssize_t count = file_size;
    while(num_read < count) {
        memset(buffer, 0, 4096+1);
        ssize_t size = count-num_read;
        if (count-num_read > 4096) size = 4096;    //for large file
        ssize_t temp_size  = read_all_from_socket(client_fd, buffer, size);
        if (temp_size == -1) {
            fclose(file);
            delete_file_from_dir(client_fd);
            clean_up_client(client_fd);
            write_error_preprocess(client_fd);
            return;
        }
        num_read += temp_size;   

        //write to file
        fwrite(buffer, temp_size, 1, file);
    }

    //now, finish reading for the required size
    //close file
    fclose(file);


    //now everything's good & we finished reading; write ok
    write_ok_preprocess(client_fd);
}

void write_error_preprocess(int client_fd) {
    //reset header & header_off
    memset(status->header, 0, HEADER_SIZE);

    // go to write_error
    write_error(client_fd);
}


void write_error(int client_fd) {
    //use header for error msg
    memcpy(header, "ERROR\n", 6);
    int size_ret = write_all_to_socket(client_fd, header, 6);
    if(size_ret == -1) {
        clean_up_client(client_fd);
        return;
    }
}

void delete_file_from_dir(int client_fd) {
    client_state * status = dictionary_get(client_status_dict, &client_fd);
    //close the file - Calling fclose twice with the same stream is undefined behaviour - most likely crash.
    //So we won't do it here!!!
    //fclose(file);

    //create pathname
    char * pathname = calloc(1, strlen(directory_name)+strlen(filename)+2); //'/' & '\0'   //need free!!!
    sprintf(pathname, "%s/%s", directory_name, filename);
    unlink(pathname); 
    free(pathname);
}



void write_ok(int client_fd) {
    //reset header & header_off
    memset(status->header, 0, HEADER_SIZE);
    //use header for Ok msg
    memcpy(header, "OK\n", 3);
    int size_ret = write_all_to_socket(client_fd, header, 3);
    if(size_ret == -1) {
        //lost connection or sth else
        clean_up_client(client_fd);
        return;
    }

    if (status->request == PUT || status->request == DELETE) {
        finish_serving(client_fd);
        return;
    }

    //for GET only
    write_size(client_fd);

}



void write_size(int client_fd) {
    //for GET only
    int size_ret = write_message_size(file_size, client_fd);
    if (size_ret == -1) {
        clean_up_client(client_fd);
        return;
    }
 
    write_data(client_fd);
}

void write_data(int client_fd) {
    //GET only
    if (request == GET) {
        ssize_t num_write = 0;
        char buffer[4096+1];
        ssize_t count = file_size;
        while(num_write < count) {
            memset(buffer, 0, 4096+1);
            ssize_t size = count-num_write;
            if (count-num_write > 4096) size = 4096;    //for large file
            fread(buffer, 1, size, file);
            ssize_t temp_size  = write_all_to_socket(client_fd, buffer, size);
            if (temp_size == -1) {
                fclose(file);
                clean_up_client(client_fd);
                return;
            }
            num_write += temp_size;  
        }

        //now means finish reading for the required size
        //close file
        fclose(status->file);
        finish_serving(client_fd);
    }
}