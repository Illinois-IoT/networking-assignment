/**
IoT Summer Camp 2023
Networking Assignment
**/
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

// a enum you may find helpful
typedef enum { GET, PUT, DELETE, V_UNKNOWN } verb;

//functions we are providing, you may find them helpful
char **parse_args(int argc, char **argv);
size_t write_all_to_socket(int socket, const char *buffer, size_t count);
size_t read_all_from_socket(int socket, char *buffer, size_t count);
size_t write_message_size(size_t size, int socket);
size_t get_message_size(int socket);
void print_client_help();

int main(int argc, char **argv) {
    //Good Luck!
    return 0;
}


/**
 * This is a function that we are providing
 *
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
