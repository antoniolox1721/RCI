#!/bin/bash

# Comprehensive test script for NDN implementation
# Based on specifications from sections 4.1 and 4.2 of the project PDF

# Colors for better readability
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[0;33m"
BLUE="\033[0;34m"
NC="\033[0m" # No Color

# Test network ID
NETWORK="100"

# Print section header
print_header() {
    echo -e "\n${BLUE}====== $1 ======${NC}\n"
}

# Print test step
print_step() {
    echo -e "${YELLOW}[TEST]${NC} $1"
}

# Print success message
print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Print error message
print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Print info message
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Check if ndn executable exists
if [ ! -f "./ndn" ]; then
    print_error "ndn executable not found. Running make to compile the program."
    make || { print_error "Compilation failed. Exiting."; exit 1; }
fi

# Kill any existing instances of ndn
pkill -f "./ndn" 2>/dev/null

# Function to start a node with given cache size, IP, and TCP port
start_node() {
    local name=$1
    local cache=$2
    local ip=$3
    local port=$4
    local reg_ip=${5:-"193.136.138.142"}
    local reg_udp=${6:-"59000"}
    
    print_step "Starting $name (IP: $ip, Port: $port)"
    
    # Create a temporary script for this node
    local script_file="temp_${name}.sh"
    cat > $script_file << EOL
#!/bin/bash
./ndn $cache $ip $port $reg_ip $reg_udp
EOL
    chmod +x $script_file
    
    # Terminal command
    if command -v gnome-terminal &>/dev/null; then
        gnome-terminal -- ./$script_file &
    elif command -v xterm &>/dev/null; then
        xterm -e ./$script_file &
    elif command -v konsole &>/dev/null; then
        konsole -e ./$script_file &
    elif command -v terminal &>/dev/null; then
        terminal -e ./$script_file &
    else
        print_error "No supported terminal emulator found."
        exit 1
    fi
    
    # Store the script file name for later cleanup
    script_files+=("$script_file")
    
    sleep 1  # Give the node time to start
    print_success "$name started successfully"
}

# Function to execute test instructions
print_test_instructions() {
    print_header "TEST INSTRUCTIONS"
    
    cat << EOL
Follow these instructions to test all functionality of the NDN implementation:

${BLUE}1. NETWORK FORMATION${NC}
---------------------
Node1: IP=127.0.0.1, Port=58001
Node2: IP=127.0.0.1, Port=58002
Node3: IP=127.0.0.1, Port=58003

In Node1 terminal:
  > dj $NETWORK 0.0.0.0 0
  (This creates a new network with ID $NETWORK)

In Node2 terminal:
  > dj $NETWORK 127.0.0.1 58001
  (This joins Node2 to network $NETWORK through Node1)

In Node3 terminal:
  > j $NETWORK
  (This joins Node3 to network $NETWORK using the registration server)

In all terminals:
  > st
  (Verify topology is correct - Node1 should show Node2 and Node3 as internal neighbors)

${BLUE}2. OBJECT MANAGEMENT${NC}
----------------------
In Node1 terminal:
  > c object1
  > c object2
  > c longnamedtestobject
  (Creates objects with these names)

  > sn
  (Verify objects are listed)

In Node2 terminal:
  > r object1
  (Retrieve object1 - should find it from Node1)
  
  > sn
  (Verify object1 is now in cache)

In Node3 terminal:
  > r object2
  > r longnamedtestobject
  (Retrieve objects - should find them from Node1)
  
  > sn
  (Verify these objects are now in cache)

In Node1 terminal:
  > dl object1
  (Delete object1)
  
  > sn
  (Verify object1 is removed)

${BLUE}3. INTEREST TABLE${NC}
--------------------
In Node2 terminal:
  > c uniqueobject
  (Create an object only on Node2)

In Node1 terminal:
  > si
  (Show interest table - should be empty)
  
  > r uniqueobject
  (Request uniqueobject - should go to neighbors)
  
  > si
  (Show interest table - should have an entry for uniqueobject)

In Node3 terminal:
  > r uniqueobject
  (Request uniqueobject - should eventually find it from Node2)

In all terminals:
  > si
  (Interest tables should update accordingly)

${BLUE}4. NODE DEPARTURE${NC}
-------------------
In Node2 terminal:
  > l
  (Leave the network)

In Node1 and Node3 terminals:
  > st
  (Verify topology has been updated - Node2 should be gone)

${BLUE}5. COMPLEX COMMANDS${NC}
---------------------
In Node1 terminal:
  > c difficult_name_123
  > c another_object_456
  (Create objects with complex names)

In Node3 terminal:
  > r difficult_name_123
  > r another_object_456
  (Attempt to retrieve objects with complex names)
  
  > sn
  (Verify objects are in cache)

${BLUE}6. NODE REJOINING${NC}
------------------
In Node2 terminal:
  > j $NETWORK
  (Rejoin the network using registration server)
  
  > st
  (Verify connections have been re-established)

${BLUE}7. INVALID COMMANDS${NC}
--------------------
Try some invalid commands to test error handling:

In any terminal:
  > c invalid name with spaces
  (Should fail - spaces not allowed)
  
  > j invalid
  (Should fail - network ID must be 3 digits)
  
  > r nonexistentobject
  (Object not found - check behavior)

${BLUE}8. EXIT TEST${NC}
-------------
When done testing:

In all terminals:
  > x
  (Exit the application)

EOL
}

# Array to store script file names for cleanup
script_files=()

# Main test execution
print_header "NDN COMPREHENSIVE TEST"

# Start three nodes
start_node "Node1" 10 "127.0.0.1" 58001
start_node "Node2" 10 "127.0.0.1" 58002
start_node "Node3" 10 "127.0.0.1" 58003

# Print test instructions
print_test_instructions

# Wait for user to finish testing
print_info "Test nodes are running. Follow the instructions above to test all functionality."
print_info "Press Enter when testing is complete to clean up..."
read

# Clean up
print_header "CLEANUP"
for script_file in "${script_files[@]}"; do
    rm -f "$script_file"
    print_info "Removed $script_file"
done

pkill -f "./ndn" 2>/dev/null
print_success "All ndn processes terminated"

print_header "TEST COMPLETE"
echo "Verify all functionality worked as expected."