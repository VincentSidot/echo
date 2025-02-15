# Echo Server

This project implements a simple echo server in C that listens for incoming connections and echoes back any data received from clients. The server uses the `poll` system call to handle multiple client connections concurrently.

## Features

- **Echo Functionality**: Echoes back any data received from clients.
- **Signal Handling**: Gracefully handles `SIGINT`, `SIGTERM`, and `SIGSEGV` signals to clean up resources.
- **Concurrent Connections**: Uses `poll` to manage multiple client connections.
- **Port Reuse**: Sets the `SO_REUSEADDR` socket option to allow the server to bind to a port immediately after it is closed.

## Prerequisites

- GCC (GNU Compiler Collection)
- Make (build automation tool)

## Building the Project

To build the project, navigate to the project directory and run the following command:

```sh
make
```

This will compile the source code and generate an executable named echo.

## Running the Server

To run the server, use the following command:

```sh
./echo
```

The server will start and listen for incoming connections on port 5000. You can connect to the server using a tool like telnet or nc (netcat).
## Code Structure

    main.c: Contains the main implementation of the echo server, including signal handling, server initialization, and the main event loop.
    array.h: Contains a simple dynamic array implementation used to store client file descriptors. (Macros only)
    Makefile: Defines the build rules for compiling the project.

## Key Functions

    init_server: Initializes the server socket and sets the SO_REUSEADDR option.
    run_server: Runs the main event loop, accepting new connections and handling client data.
    echo_server: Reads data from a client and echoes it back.
    handle_signal: Handles signals to clean up resources and terminate the server gracefully.
    cleanup: Closes all file descriptors and frees allocated memory.

## Signal Handling

The server registers signal handlers for SIGINT, SIGTERM, and SIGSEGV to ensure that resources are properly cleaned up when the server is terminated. The handle_signal function is responsible for cleaning up and restoring the default signal behavior.

## Cleaning Up

To clean up the build directory, run the following command:

```sh
make clean
```

This will remove the build directory and all compiled object files.