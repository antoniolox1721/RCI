# Named Data Network (NDN) Implementation

## Project Overview

This project implements a Named Data Network (NDN) as specified for the Computer Networks and Internet course (2024/2025). The implementation creates a distributed network system where data objects are identified and retrieved by their names rather than their locations, built as an overlay network on top of TCP/IP.

## Key Features

- Tree-based network topology maintenance
- Interest-based content discovery and retrieval
- Object caching for improved performance
- Robust handling of node failures and network partitions
- Command-line interface for network and content management

## Architecture

The NDN implementation follows these core principles:

- Each node is identified by an IP address and TCP port
- Data objects are identified by globally unique names
- Network topology is maintained as a tree structure
- Content is cached along retrieval paths
- Messages travel in the opposite direction of interests

## Network Protocols

### Registration Protocol (UDP)
- `NODES <net>` - Request list of nodes in a network
- `NODESLIST <net>\n<IP1> <TCP1>\n<IP2> <TCP2>\n...` - Response with registered nodes
- `REG <net> <IP> <TCP>` - Register a node in a network
- `UNREG <net> <IP> <TCP>` - Unregister a node from a network
- `OKREG` - Confirmation of successful registration
- `OKUNREG` - Confirmation of successful unregistration

**Node Registration Sequence:**
1. Client sends `NODES <net>` to the registration server
2. Server responds with `NODESLIST <net>\n<IP1> <TCP1>\n...`
3. Client selects a node and connects via TCP
4. Client sends `REG <net> <IP> <TCP>` to register itself
5. Server confirms with `OKREG`

**Node Unregistration Sequence:**
1. Client sends `UNREG <net> <IP> <TCP>` to the registration server
2. Server confirms with `OKUNREG`
3. Client closes all TCP connections with neighbors

### Topology Protocol (TCP)
- `ENTRY <IP> <TCP>\n` - Inform a node of your entry to the network
- `SAFE <IP> <TCP>\n` - Provide safety node information

**Node Joining Sequence:**
1. New node connects to an existing node via TCP
2. New node sends `ENTRY <IP> <TCP>\n` with its listening address
3. Receiving node adds the sender as an internal neighbor
4. Receiving node responds with `SAFE <EXT_IP> <EXT_TCP>\n` containing its external neighbor info

**Node Departure Handling:**
1. When a node detects a disconnected external neighbor:
   - If not self-salvaged: Connect to safety node, send `ENTRY`, update internal neighbors
   - If self-salvaged: Choose new external from internal neighbors, send `ENTRY`, update others
2. For each internal neighbor, send `SAFE` messages with updated safety node information

### NDN Protocol (TCP)
- `INTEREST <name>\n` - Request for an object by name
- `OBJECT <name>\n` - Response containing the requested object
- `NOOBJECT <name>\n` - Notification that object is not available

**Object Retrieval Sequence:**
1. Requesting node sends `INTEREST <name>\n` to all interfaces
2. Each receiving node:
   - If it has the object: Responds with `OBJECT <name>\n`
   - If it doesn't have the object: Forwards `INTEREST <name>\n` to other interfaces
   - If all paths are closed: Responds with `NOOBJECT <name>\n`
3. Intermediate nodes:
   - Cache objects as they pass through
   - Forward responses back to the requesting interface
   - Update interest table states accordingly

**Interest Table State Transitions:**
- When receiving `INTEREST`: Mark source interface as `RESPONSE`, other interfaces as `WAITING`
- When receiving `OBJECT`: Forward to all `RESPONSE` interfaces, add to cache, remove interest entry
- When receiving `NOOBJECT`: Mark source interface as `CLOSED`, check if any `WAITING` remain

## Commands

### Network Management
- `join (j) <net>` - Join a network through the registration server
- `direct join (dj) <connectIP> <connectTCP>` - Join directly to a node
- `leave (l)` - Leave the current network
- `exit (x)` - Close the application

### Object Management
- `create (c) <name>` - Create a new object
- `delete (dl) <name>` - Delete an existing object
- `retrieve (r) <name>` - Search for and retrieve an object

### Information Display
- `show topology (st)` - Show network connections
- `show names (sn)` - List objects stored in the node
- `show interest table (si)` - Show pending interests

## Usage

### Compilation
```bash
make
```

### Running the Application
```bash
./ndn <cache_size> <IP> <TCP_port> [regIP] [regUDP]
```

### Example
```bash
# Start a node with cache size 10 on localhost:58001
./ndn 10 127.0.0.1 58001

# Start a node with a specific registration server
./ndn 10 127.0.0.1 58001 193.136.138.142 59000
```

### Testing
The project includes testing scripts to validate functionality:
```bash
# Run the complete test suite
./test.sh

# Run the NDN-specific tests
./ndn_test.sh
```

## Implementation Details

### Interest Table
The interest table tracks pending requests for objects:
- `RESPONSE` interfaces where object/absence messages should be forwarded
- `WAITING` interfaces where interest messages have been sent
- `CLOSED` interfaces where absence messages have been received

### Network Recovery
When a node disconnects:
- If the external neighbor disconnects, connect to the safety node
- If a node is its own safety node, choose a new external neighbor
- Propagate updated topology information to all internal neighbors

## Project Structure
- `main.c` - Application entry point and main loop
- `ndn.h` - Core data structures and definitions
- `commands.c/h` - User command processing
- `network.c/h` - Network communication and protocol handling
- `objects.c/h` - Object management and interest table operations
- `debug_utils.c/h` - Debugging and error reporting utilities

## Authors
Barbara Gonçalves Modesto and António Pedro Lima Loureiro Alves  
Computer Networks and Internet Course, 2024/2025

## Acknowledgments
- Course professors and teaching assistants
- IST - Instituto Superior Técnico, Universidade de Lisboa