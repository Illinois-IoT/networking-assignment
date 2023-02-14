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



//./client sp23-cs341-adm.cs.illinois.edu:3456 GET random1.txt random2.txt


#define BUFFER_SIZE 1024
#define HEADER_SIZE 20
#define MAX_CLIENTS 1




static char* directory_name;
static int epoll_fd;
static int socket_fd;
static int client_fd;
static char* header = NULL;   //need free
static char* request;
static char* filename;

void close_server();
int run_server(char *port);

void handle_events(struct epoll_event *events, int nfds);
void handle_client(int client_fd);
void write_error(int client_fd);
void write_errmsg(int client_fd);
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
    read_size(client_fd);
    read_data(client_fd);
    write_ok(client_fd);
    write_error(client_fd);
    write_size(client_fd);
    write_data(client_fd);
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
    char* bad_form = strtok(NULL, " ");
    if (bad_form != NULL || request == NULL) {
        flag = -1;
    } else if (!strcmp(request, "GET")) {
        if (filename == NULL) {
            flag = -1;
        } else {
            request = GET;
                status->filename = strdup(filename);
                //status->state = WRITE_OK;
                status->header_offset = 0;
                memset(status->header, 0, HEADER_SIZE);
                write_ok_preprocess(client_fd);
            }  
        } else if (!strcmp(request, "PUT")) {
            if (filename == NULL) {
                flag = -1;
            } else {
                status->request = PUT;
                status->filename = strdup(filename);
                status->state = READ_SIZE;
                status->header_offset = 0;
                memset(status->header, 0, HEADER_SIZE);
                read_size(client_fd);
            }
        } else if (!strcmp(request, "DELETE")) {
            if (filename == NULL) {
                flag = -1;
            } else {
                status->request = DELETE;
                status->filename = strdup(filename);
                //status->state = WRITE_OK;
                status->header_offset = 0;
                memset(status->header, 0, HEADER_SIZE);
                write_ok_preprocess(client_fd);
            }
        } else if (!strcmp(request, "LIST")) {
            if (filename != NULL) {
                flag = -1;
            } else {
                status->request = LIST;
                //status->state = WRITE_OK;
                status->header_offset = 0;
                memset(status->header, 0, HEADER_SIZE);
                write_ok_preprocess(client_fd);
            }
/*new*/
        } else {
            flag = -1;
        }
    }
    if (flag == -1) { 
        //error state
        write_error_preprocess(client_fd, 1);
    }
}



void write_ok_preprocess(int client_fd) {
    client_state * status = dictionary_get(client_status_dict, &client_fd);
    verb request = status->request;
    switch(request) {
        case GET: {
            char * pathname = calloc(1, strlen(directory_name)+strlen(status->filename)+2); //'/' & '\0'   //need free!!!
            sprintf(pathname, "%s/%s", directory_name, status->filename);
            status->file = fopen(pathname, "r");
            free(pathname);
            if(!status->file) {
                write_error_preprocess(client_fd, 3);
                return;
            }
            //get filesize
            fseek(status->file, 0, SEEK_END);
            status->file_size = ftell(status->file);
            rewind(status->file);
            break;
        }
        case PUT: {
            break;
        }
        case DELETE: {
            //delete first
            int delete_success = 0;
            size_t i = 0;
            for (; i < vector_size(file_list); i++) {
                char* curr_filename = vector_get(file_list,i);
                if (!strcmp(curr_filename, status->filename)) {
                    delete_success = 1;
                    break;
                }
            }
            if (!delete_success) {
                write_error_preprocess(client_fd, 3);
                return;
            }
            //now find it - delete
            delete_file_from_dir(client_fd); 
            vector_erase(file_list, i);
            break;
        }
        case LIST: {
            //get list buffer
            //1. get buffer size
            status->file_size = 0;
            if (vector_empty(file_list)) break;
            for (size_t i = 0; i < vector_size(file_list); i++) {
                char* curr_filename = vector_get(file_list,i);
                status->file_size += strlen(curr_filename)+1; //'\n'
            }
            if (status->file_size) status->file_size -= 1;
            status->list_buffer = calloc(1, status->file_size+1); //'\0'
            char* buffer = status->list_buffer;
            size_t i = 0;
            for (; i < vector_size(file_list) - 1; i++) {
                char* curr_filename = vector_get(file_list,i);
                size_t size = strlen(curr_filename);
                memcpy(buffer, curr_filename, size);
                buffer += size;
                buffer[0] = '\n';
                buffer += 1;
            }
            char* curr_filename = vector_get(file_list,i);
            size_t size = strlen(curr_filename);
            memcpy(buffer, curr_filename, size);
            break;
        }
        default: {/*should not reach here*/}
    }

    //write ok
    //1. modify event
    modify_event(client_fd, EPOLLOUT);
    //2. change state
    status->state = WRITE_OK;
    //3. reset header & header_offset
    status->header_offset = 0;
    memset(status->header, 0, HEADER_SIZE);
    //4. write_ok function call
    write_ok(client_fd);
}

void read_size(int client_fd) {
    //only for PUT
int flag = 0;
    client_state * status = dictionary_get(client_status_dict, &client_fd);
    int size_ret = server_read_all_from_socket(client_fd, ((char *)&(status->file_size))+status->header_offset, sizeof(size_t)-status->header_offset, &flag);
    if (size_ret == -1) {
        clean_up_client(client_fd);
        return;
    }
    status->header_offset += size_ret;
    if (status->header_offset == sizeof(size_t)) {
        status->state = READ_DATA;
        status->header_offset = 0;
        memset(status->header, 0, HEADER_SIZE);
        char * pathname = calloc(1, strlen(directory_name)+strlen(status->filename)+2); //'/' & '\0'   //need free!!!
        sprintf(pathname, "%s/%s", directory_name, status->filename);
        status->file = fopen(pathname, "w");
        free(pathname);
        read_data(client_fd);
    }
}

void read_data(int client_fd) {
    //only for PUT
    client_state * status = dictionary_get(client_status_dict, &client_fd);
    ssize_t num_read = 0;
    char buffer[4096+1];
    ssize_t count = status->file_size-ftell(status->file);
    while(num_read < count) {
        memset(buffer, 0, 4096+1);
        ssize_t size = count-num_read;
        if (count-num_read > 4096) size = 4096;    //for large file
int flag = 0;
        ssize_t temp_size  = server_read_all_from_socket(client_fd, buffer, size, &flag);
        if (temp_size == -1) {
            fclose(status->file);
            delete_file_from_dir(client_fd);
            clean_up_client(client_fd);
            write_error_preprocess(client_fd, 2);
            return;
        }
        
        num_read += temp_size; 
/*new*/

        //check for recieving too little data
        if (flag == 1 && temp_size < size) {
            delete_file_from_dir(client_fd);
            //error state
            write_error_preprocess(client_fd, 2);
            return;
        }    
/*new*/
        //write to file
        fwrite(buffer, temp_size, 1, status->file);
        if (temp_size < size) return; /*currently unavailable to read more*/
    }

    //no means finish reading for the required size
    //close file
    fclose(status->file);

    //check for too much data - flag == 0
int flag_ = 0;
    size_t temp_size  = server_read_all_from_socket(client_fd, buffer, 2, &flag_);
    if (temp_size > 0) {
        delete_file_from_dir(client_fd);
        //error state
        write_error_preprocess(client_fd, 2);
        return;
    }

    //now everything's good & we finished reading
    //1. push file to vector files - maybe hardlink????
    int if_exists = 0;
    for (size_t i = 0; i < vector_size(file_list); i++) {
        char* curr_filename = vector_get(file_list,i);
        if (!strcmp(curr_filename, status->filename)) {
            if_exists = 1;
            break;
        }
    }
    if (!if_exists) vector_push_back(file_list, status->filename);
    //2. new state
    write_ok_preprocess(client_fd);
}

void write_error_preprocess(int client_fd, int error_type) {
    //change event
    modify_event(client_fd, EPOLLOUT);
    //change state
    client_state * status = dictionary_get(client_status_dict, &client_fd);
    status->state = WRITE_ERROR;

    //reset header & header_off
    memset(status->header, 0, HEADER_SIZE);
    status->header_offset = 0;

    //set error_type
    status->error_type = error_type;

    // go to write_error
    write_error(client_fd);
}


void write_error(int client_fd) {
    //executing
    client_state * status = dictionary_get(client_status_dict, &client_fd);
    memcpy(status->header, "ERROR\n", 6);
int flag = 0;
    int size_ret = server_write_all_to_socket(client_fd, status->header+status->header_offset, 6-status->header_offset, &flag);
    if(size_ret == -1 || flag == 1) {
        clean_up_client(client_fd);
        return;
    }

    status->header_offset += size_ret;

    if (status->header_offset == 6) {

        status->state = WRITE_ERRMSG;
        status->header_offset = 0;
        status->buffer_offset = 0;
        memset(status->header, 0, HEADER_SIZE);
        write_errmsg(client_fd);
    }
}

void write_errmsg(int client_fd) {
    client_state * status = dictionary_get(client_status_dict, &client_fd);
    char* err_msg = calloc(1,100);        //need free
    if (status->error_type == 1) sprintf(err_msg, "%s\n", err_bad_request);
    else if (status->error_type == 2) sprintf(err_msg, "%s\n", err_bad_file_size);
    else if (status->error_type == 3) sprintf(err_msg, "%s\n", err_no_such_file);
    else {
        /*should not reach*/
        perror("write_errmsg_state");
        exit(1);
    }
int flag = 0;   
    int size_ret = server_write_all_to_socket(client_fd, err_msg+status->buffer_offset, strlen(err_msg)-status->buffer_offset, &flag);
    
    if(size_ret == -1 || flag == 1) {
        //lost connection or sth else
        free(err_msg);
        clean_up_client(client_fd);
        return;
    }
    status->buffer_offset += size_ret;

    if (status->buffer_offset == strlen(err_msg)) {
        finish_serving(client_fd);
    }
    free(err_msg);
}



void delete_file_from_dir(int client_fd) {
    client_state * status = dictionary_get(client_status_dict, &client_fd);
    //close the file - Calling fclose twice with the same stream is undefined behaviour - most likely crash.
    //So we won't do it here!!!
    //fclose(status->file);

    //create pathname
    char * pathname = calloc(1, strlen(directory_name)+strlen(status->filename)+2); //'/' & '\0'   //need free!!!
    sprintf(pathname, "%s/%s", directory_name, status->filename);
    unlink(pathname);
    free(pathname);
}








void write_ok(int client_fd) {
    client_state * status = dictionary_get(client_status_dict, &client_fd);
    memcpy(status->header, "OK\n", 3);
int flag = 0;
    int size_ret = server_write_all_to_socket(client_fd, status->header+status->header_offset, 3-status->header_offset, &flag);
    if(size_ret == -1 || flag == 1) {
        //lost connection or sth else
        clean_up_client(client_fd);
        return;
    }
    status->header_offset += size_ret;

    if (status->header_offset == 3) {
        if (status->request == PUT || status->request == DELETE) {
            finish_serving(client_fd);
            return;
        }
        status->state = WRITE_SIZE;
        status->header_offset = 0;
        memset(status->header, 0, HEADER_SIZE);
        write_size(client_fd);
    }
}



void write_size(int client_fd) {
    client_state * status = dictionary_get(client_status_dict, &client_fd);
int flag = 0;
    int size_ret = server_write_all_to_socket(client_fd, ((char *)&(status->file_size))+status->header_offset, sizeof(size_t)-status->header_offset, &flag);
    if (size_ret == -1 || flag == 1) {
        clean_up_client(client_fd);
        return;
    }
    status->header_offset += size_ret;
    if (status->header_offset == sizeof(size_t)) {
        status->state = WRITE_DATA;
        status->header_offset = 0;
        status->buffer_offset = 0;
        memset(status->header, 0, HEADER_SIZE);
        write_data(client_fd);
    }
}

void write_data(int client_fd) {
    client_state * status = dictionary_get(client_status_dict, &client_fd);
    verb request = status->request;
    if (request == LIST) {
int flag = 0;
        int size_ret = server_write_all_to_socket(client_fd, status->list_buffer+status->buffer_offset, status->file_size-status->buffer_offset, &flag);
        if(size_ret == -1 || flag == 1) {
            //lost connection or sth else
            clean_up_client(client_fd);
            return;
        }
        status->buffer_offset += size_ret;
        if (status->buffer_offset == status->file_size) {
            finish_serving(client_fd);
        }
    }

    if (request == GET) {
        ssize_t num_write = 0;
        char buffer[4096+1];
        ssize_t count = status->file_size-ftell(status->file);
        while(num_write < count) {
            memset(buffer, 0, 4096+1);
            ssize_t size = count-num_write;
            if (count-num_write > 4096) size = 4096;    //for large file
            fread(buffer, 1, size, status->file);
int flag = 0;
            ssize_t temp_size  = server_write_all_to_socket(client_fd, buffer, size, &flag);
            if (temp_size == -1 || flag == 1) {
                fclose(status->file);
                clean_up_client(client_fd);
                return;
            }
            num_write += temp_size;  
            if (temp_size < size) {
                /*currently unavailable to read more*/
                fseek(status->file, -(size-temp_size), SEEK_CUR);
                return;
            }
        }

        //now means finish reading for the required size
        //close file
        fclose(status->file);
        finish_serving(client_fd);
    }
}