# Samir Hadi Cisneros
# Multi-Threaded Web Server with Thread Pool

This project aims to enhance the functionality of a basic web server (webserver.c) by implementing a multi-threaded architecture with a thread pool in `webserver_multi.c`. The thread pool approach allows the server to efficiently handle multiple incoming client requests concurrently, significantly improving its responsiveness and performance.

## Features

- Multi-threaded architecture.
- Thread pool for managing worker threads.
- Producer-consumer model using semaphores.
- Efficient handling of multiple client requests.
- Supports concurrent processing of client requests.


## Performance and Testing

Testing was done by running `client` with 100 requests to the webserver

When running `webserver_multi` with 1 thread   : "Time to handle 100 requests (0 failed): 100.045275 sec"
When running `webserver_multi` with 25 threads : "Time to handle 100 requests (0 failed): 4.008643 sec"
When running `webserver_multi` with 50 threads : "Time to handle 100 requests (0 failed): 2.010054 sec"

Also tested this using multiple seperate windows making client requests, each request beeing handled successfully and zero failing. 

## Design 
This code uses a thread pool in a producer-consumer model to efficiently synchronize shared data. The code uses mutexes and semaphores to ensure efficient and safe interaction among threads. 
Semaphores full and empty manage the number of jobs and available slots, while the mutex prevents concurrent access to the queue. Worker threads efficiently process jobs when they are available, enhancing the server's performance and resource management.

The job queue array works as a shared buffer where incoming "jobs" (sockets) are addded. The worker threads are the consumers that take jobs from the queue. When a job is added, it acts as a producer, placing the job in the queue and signaling the consumers. Worker threads then consume these jobs by processing the connections. 


## Compilation

To compile the project, use the makefile commands:

1. Compile `webserver_multi.c` and `client.c`:
   ```bash
   make clean 
   make all
   ```


## Running the Web Server

1. **Start the Web Server**:

   In one terminal window, run the compiled `webserver_multi` program. The web server will listen on a specified port (e.g., 2556) and use a thread pool for handling incoming client requests.

   ```bash
   ./webserver_multi 2556 4
   ```

   - Replace `2556` with the desired port number.
   - Replace `4` with the number of worker threads you want in the thread pool. Adjust this number according to your system's capabilities.

   The web server is now running and ready to accept incoming connections.

2. **Start the Client**:

   In another terminal window, run the compiled `client` program. This client will send HTTP GET requests to the web server to test its responsiveness.

   ```bash
   ./client 127.0.0.1 2556 12
   ```

   - Replace `127.0.0.1` with the IP address or hostname of the server.
   - Replace `2556` with the same port number used for the web server.
   - Adjust `12` to specify the number of concurrent client requests you want to send.
   - the third argument, <# of threads> is optional. The default is 10.

   The client will send requests to the web server, and you will see the response times for each request.

## Sources

- The single-threaded `webserver.c` code is a modified version of the code available at [http://www.jbox.dk/sanos/webserver.htm](http://www.jbox.dk/sanos/webserver.htm).

- The `client.c` code is sourced from [http://coding.debuntu.org/c-linux-socket-programming-tcp-simple-http-client](http://coding.debuntu.org/c-linux-socket-programming-tcp-simple-http-client).
