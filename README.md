# Named Data Network (NDN) Implementation

## Project Overview

This project implements a Named Data Network (NDN) as part of the Computer Networks and Internet course (2024/2025). The implementation provides a distributed network system where data objects are identified and retrieved by their names, rather than by their location.

## Key Features

- Distributed object storage and retrieval
- Tree-based network topology
- Peer-to-peer communication
- Caching mechanism
- Support for network joining, leaving, and object management

## Architecture

The NDN implementation follows these core principles:

- Nodes are identified by IP address and TCP port
- Objects are identified by unique names
- Network topology is maintained as a tree
- Supports object creation, deletion, and retrieval
- Implements interest-based data discovery

## Network Protocols

### Registration Protocol
- Communication with a central registration server
- Support for joining and leaving networks
- Network discovery through node listing

### Topology Protocol
- Maintenance of tree-based network structure
- Handling node entry and departure
- Management of external and internal neighbors

### NDN Protocol
- Interest-based object retrieval
- Caching of retrieved objects
- Support for multiple interfaces and routing

## Commands

### Network Management
- `join <net>`: Join an existing network
- `direct join <IP> <TCP>`: Directly connect to a specific node
- `leave`: Exit the current network

### Object Management
- `create <name>`: Create a new object
- `delete <name>`: Remove an existing object
- `retrieve <name>`: Search for and retrieve an object

### Information Commands
- `show topology`: Display network connections
- `show names`: List local and cached objects
- `show interest table`: Display pending object interests

## Usage

### Compilation
```bash
make
```

### Running the Application
```bash
./ndn <cache_size> <IP> <TCP_port> [reg_IP] [reg_UDP]
```

### Example
```bash
./ndn 10 127.0.0.1 58001
```

## Dependencies
- Linux environment
- GCC compiler
- Standard C libraries

## Project Structure
- `main.c`: Application entry point and main loop
- `network.c`: Network protocol implementations
- `commands.c`: User command handling
- `objects.c`: Object and cache management
- `debug_utils.c`: Debugging and logging utilities

## Limitations
- Maximum of 100 objects per node
- Object names must be alphanumeric
- Tree-based topology restricts network complexity

## Contributing
Contributions are welcome. Please follow the existing code style and submit pull requests for review.

## License
This project is developed as part of an academic course. Please refer to the course guidelines for usage and distribution.

## Authors
Computer Networks and Internet Course, 2024/2025

## Acknowledgments
- Course instructors and teaching assistants
- W. Richard Stevens' networking books
- Open-source networking resources
