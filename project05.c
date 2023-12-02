#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 1024
#define WEB_ROOT "www"  // Directory where web content is stored


volatile sig_atomic_t stop_server = 0;
// Signal handler
void handle_signal(int signal) {
  if (signal == SIGINT) {
    printf("Server shutting down...\n");
    stop_server = 1;
  }
}

// Function prototypes
int read_port_from_file();
int create_socket(int port);
void bind_socket(int sockfd, int port);
void listen_for_connections(int sockfd);
int accept_connection(int sockfd);
void handle_client(int client_socket);

// HTTP handling functions
void handle_http_request(int client_socket);
void send_success_response(int client_socket);
void send_error_response(int client_socket, int status_code,
                         const char *status_text, const char *error_message);
void send_response(int client_socket, int status_code, const char *status_text,
                   const char *content_type, const char *body);

// Function prototypes
void serve_static_file(int client_socket, const char *path);
void send_file_response(int client_socket, const char *file_path);

int main(int argc, char *argv[]) {
  // Set up signal handler for graceful shutdown
  signal(SIGINT, handle_signal);

  // Read port number from a file
    int port = read_port_from_file();
  
    // Create and configure the server socket
    int sockfd = create_socket(port);
    bind_socket(sockfd, port);
    listen_for_connections(sockfd);
  
    // Server main loop
    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
  
    // Server main loop
    while (!stop_server) {
      int client_socket = accept_connection(sockfd);
      if (client_socket != -1) {
        // Make the client socket non-blocking
        int flags = fcntl(client_socket, F_GETFL, 0);
        fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);
  
        handle_client(client_socket);
      }
    }
  
    // Close the server socket
    close(sockfd);
  
    return 0;
  }

  // Read the port number from a file
  int read_port_from_file() {
    FILE *file = fopen("port.txt", "r");
    if (file == NULL) {
      perror("Error opening port file");
      exit(EXIT_FAILURE);
    }
  
    int port;
    fscanf(file, "%d", &port);
    fclose(file);
  
    return port;
  }

  // Create a socket
  int create_socket(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
      perror("socket");
      exit(EXIT_FAILURE);
    }
    return sockfd;
  }
  
  // Bind a socket to a specific port
  void bind_socket(int sockfd, int port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
  
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      perror("bind");
      close(sockfd);
      exit(EXIT_FAILURE);
    }
  }

  // Listen for incoming connections
  void listen_for_connections(int sockfd) {
    if (listen(sockfd, 3) == -1) {
      perror("listen");
      close(sockfd);
      exit(EXIT_FAILURE);
    }
  }
  
  // Accept an incoming connection
  int accept_connection(int sockfd) {
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
  
    struct pollfd poll_fd;
    poll_fd.fd = sockfd;
    poll_fd.events = POLLIN;
  
    int poll_result = poll(&poll_fd, 1, 0);
  
    if (poll_result == -1) {
      perror("poll");
      return -1;
    }

    if (poll_fd.revents & POLLIN) {
        int client_socket =
            accept(sockfd, (struct sockaddr *)&client_addr, &client_addrlen);
    
        if (client_socket == -1) {
          perror("accept");
          return -1;
        }
    
        return client_socket;
      }
    
      return -1; // No incoming connection
    }
    
    // Handle a client connection
    void handle_client(int client_socket) {
      // Handle HTTP request for the client
      handle_http_request(client_socket);
    
      // Close the client socket
      close(client_socket);
    }
// Handle an HTTP request from a client
void handle_http_request(int client_socket) {
  char buffer[MAX_BUFFER_SIZE];

  // Receive the HTTP request
  ssize_t recv_val;
  while ((recv_val = recv(client_socket, buffer, MAX_BUFFER_SIZE - 1, 0)) > 0) {
    buffer[recv_val] = '\0'; // Null-terminate the received data

    // Parse the request line
    char method[MAX_BUFFER_SIZE];
    char uri[MAX_BUFFER_SIZE];
    if (sscanf(buffer, "%s %s", method, uri) != 2) {
      fprintf(stderr, "Error parsing request line\n");
      send_error_response(client_socket, 400, "Bad Request",
                          "Invalid request format");
      return;
    }

     // Ensure the method is GET
    if (strcmp(method, "GET") != 0) {
      send_error_response(client_socket, 405, "Method Not Allowed",
                          "Only GET method is allowed");
      return;
    }

   // Extract the path from the URI
    char *path = strtok(uri, "?");  // Remove query parameters if any

    // Serve static files if requested
    if (strstr(path, "/static/") == path) {
      serve_static_file(client_socket, path + strlen("/static/"));
    } else {
      // Handle other HTTP requests
      if (strcmp(path, "/") == 0) {
        send_success_response(client_socket);
      } else {
        send_error_response(client_socket, 404, "Not Found", "Resource not found");
      }
    }

    break;
      }
    
      if (recv_val < 0) {
        perror("recv");
      }
  close(client_socket);
}





void send_success_response(int client_socket) {
  const char *response_body = "<!DOCTYPE html>\n<html>\n  <body>\n    Hello CS "
                              "221\n  </body>\n</html>\n";

  send_response(client_socket, 200, "OK", "text/html", response_body);
}

void send_error_response(int client_socket, int status_code,
                         const char *status_text, const char *error_message) {
  const char *response_body = "<!DOCTYPE html>\n<html>\n<body>\nNot "
                              "found\n</body>\n</html>\n";

  send_response(client_socket, status_code, status_text, "text/plain\n\n",
                response_body);
}

void send_response(int client_socket, int status_code, const char *status_text,
                   const char *content_type, const char *body) {
  char response_header[MAX_BUFFER_SIZE];
  snprintf(response_header, MAX_BUFFER_SIZE,
           "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\n\r\n",
           status_code, status_text, content_type, strlen(body));

  ssize_t send_val =
      send(client_socket, response_header, strlen(response_header), 0);
  if (send_val < 0) {
    perror("send");
    close(client_socket);
    exit(EXIT_FAILURE);
  }

  send_val = send(client_socket, body, strlen(body), 0);
  if (send_val < 0) {
    perror("send");
    close(client_socket);
    exit(EXIT_FAILURE);
  }
}   	                  
// Serve a static file
void serve_static_file(int client_socket, const char *path) {
  char file_path[MAX_BUFFER_SIZE];
  snprintf(file_path, MAX_BUFFER_SIZE, "%s/%s", WEB_ROOT, path);

  // Send the static file
  send_file_response(client_socket, file_path);
}

// Send the content of a file as an HTTP response
void send_file_response(int client_socket, const char *file_path) {
  FILE *file = fopen(file_path, "rb");
  if (file == NULL) {
    send_error_response(client_socket, 404, "Not Found", "File not found");
    return;
  }

  // Get the file size
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);
  
    // Prepare HTTP headers
    char response_header[MAX_BUFFER_SIZE];
    snprintf(response_header, MAX_BUFFER_SIZE,"<!DOCTYPE html>\n<html>\n  <body>\n"  
    				 " Hello CS 221\n  </body>\n</html>\n", file_size);
    snprintf(response_header, MAX_BUFFER_SIZE,
             "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %lu\r\n\r\n",
             file_size);

  // Send HTTP headers
    ssize_t send_val = send(client_socket, response_header, strlen(response_header), 0);
    if (send_val < 0) {
      perror("send");
      fclose(file);
      close(client_socket);
      exit(EXIT_FAILURE);
    }

    // Send the file content
      char buffer[MAX_BUFFER_SIZE];
      size_t bytes_read;
      while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send_val = send(client_socket, buffer, bytes_read, 0);
        if (send_val < 0) {
          perror("send");
          fclose(file);
          close(client_socket);
          exit(EXIT_FAILURE);
        }
      }
    
      fclose(file);
    }