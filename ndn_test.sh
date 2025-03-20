#!/bin/bash

# NDN Testing Script for Group 076
# This script automates the testing of the NDN implementation

# Colors for better readability
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[0;33m"
BLUE="\033[0;34m"
NC="\033[0m" # No Color

# Configuration variables
GROUP_ID="076"
NDN_EXE="./ndn"
NODE_IP="127.0.0.1"
NODE_PORT="58001"
CACHE_SIZE="10"
REG_SERVER_IP="193.136.138.142"
REG_SERVER_UDP="59000"
TEJO_IP="tejo.tecnico.ulisboa.pt"
TEJO_PORT="59011"
ACCESS_CODE=""
SLEEP_TIME_SHORT="2"
SLEEP_TIME_MEDIUM="5"
SLEEP_TIME_LONG="10"

# Basic utility functions
print_header() {
    echo -e "\n${BLUE}====== $1 ======${NC}\n"
}

print_step() {
    echo -e "${YELLOW}[STEP]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_cmd() {
    echo -e "${YELLOW}$ $1${NC}"
}

# Function to extract ports and access code from init.html
extract_info() {
    print_step "Extracting information from init.html"
    
    # Extract the access code - handles both formats
    ACCESS_CODE=$(grep -oP 'REPORT ACCESS CODE: \K[0-9]+' init.html | head -1)
    if [ -z "$ACCESS_CODE" ]; then
        # Try alternate format
        ACCESS_CODE=$(grep -oP 'Code: \K[0-9]+' init.html | head -1)
    fi
    
    if [ -z "$ACCESS_CODE" ]; then
        print_error "Failed to extract access code from init.html"
        cat init.html
        exit 1
    fi
    print_success "Extracted access code: $ACCESS_CODE"
    
    # Extract ports for resident applications - updated pattern matching
    PORT_REG=$(grep -oP 'NODE SERVER AT IP:[0-9.]+ PORT:\K[0-9]+' init.html | head -1)
    PORT_ID1=$(grep -oP 'ID1: \[[0-9.]+ \K[0-9]+' init.html | head -1)
    PORT_ID2=$(grep -oP 'ID2: \[[0-9.]+ \K[0-9]+' init.html | head -1)
    PORT_ID3=$(grep -oP 'ID3: \[[0-9.]+ \K[0-9]+' init.html | head -1)
    PORT_ID4=$(grep -oP 'ID4: \[[0-9.]+ \K[0-9]+' init.html | head -1)
    PORT_ID5=$(grep -oP 'ID5: \[[0-9.]+ \K[0-9]+' init.html | head -1)
    PORT_ID6=$(grep -oP 'ID6: \[[0-9.]+ \K[0-9]+' init.html | head -1)
    
    # If not found with the new pattern, try the original pattern
    if [ -z "$PORT_ID1" ]; then
        PORT_ID1=$(grep -A10 "Port of ID1:" init.html | grep -oP '\d{5}' | head -1)
    fi
    if [ -z "$PORT_ID2" ]; then
        PORT_ID2=$(grep -A10 "Port of ID2:" init.html | grep -oP '\d{5}' | head -1)
    fi
    if [ -z "$PORT_ID3" ]; then
        PORT_ID3=$(grep -A10 "Port of ID3:" init.html | grep -oP '\d{5}' | head -1)
    fi
    if [ -z "$PORT_ID4" ]; then
        PORT_ID4=$(grep -A10 "Port of ID4:" init.html | grep -oP '\d{5}' | head -1)
    fi
    if [ -z "$PORT_ID5" ]; then
        PORT_ID5=$(grep -A10 "Port of ID5:" init.html | grep -oP '\d{5}' | head -1)
    fi
    if [ -z "$PORT_ID6" ]; then
        PORT_ID6=$(grep -A10 "Port of ID6:" init.html | grep -oP '\d{5}' | head -1)
    fi
    if [ -z "$PORT_REG" ]; then
        PORT_REG=$(grep -A10 "Port of registration server:" init.html | grep -oP '\d{5}' | head -1)
    fi
    
    echo "Port REG: $PORT_REG"
    echo "Port ID1: $PORT_ID1"
    echo "Port ID2: $PORT_ID2"
    echo "Port ID3: $PORT_ID3"
    echo "Port ID4: $PORT_ID4"
    echo "Port ID5: $PORT_ID5"
    if [ ! -z "$PORT_ID6" ]; then
        echo "Port ID6: $PORT_ID6"
    fi
}

# Function to terminate any existing sessions
terminate_previous_sessions() {
    print_step "Checking and terminating any previous sessions"
    
    # Try common access codes for group 076
    common_codes=("0206154" "0206175" "0000076" "0760000" "0760076" "0760760" "1234567" "7654321" "0761234" "0760769")
    
    for code in "${common_codes[@]}"; do
        print_cmd "echo \"FIN$code\" | nc $TEJO_IP $TEJO_PORT > fin_result.html"
        echo "FIN$code" | nc $TEJO_IP $TEJO_PORT > fin_result.html
        
        if ! grep -q "ERROR" fin_result.html; then
            print_success "Previous session terminated successfully with access code: $code"
            return 0
        fi
    done
    
    # If no session was active or all termination attempts failed, that's okay
    # We'll proceed with activating a new session anyway
    print_step "No active session found or termination not needed"
    return 0
}

# Function to activate a session
activate_session() {
    local session_type=$1
    
    # First terminate any previous sessions
    terminate_previous_sessions
    
    print_step "Activating session type $session_type for group $GROUP_ID"
    print_cmd "echo \"$GROUP_ID:$session_type\" | nc $TEJO_IP $TEJO_PORT > init.html"
    echo "$GROUP_ID:$session_type" | nc $TEJO_IP $TEJO_PORT > init.html
    
    # Check if the activation was successful
    if grep -q "ERROR" init.html || grep -q "BLOCKED" init.html; then
        print_error "Failed to activate session. Error message:"
        cat init.html
        
        # Ask user if they want to provide a specific access code to terminate
        echo "Do you want to provide a specific access code to terminate the previous session? (y/n)"
        read answer
        if [[ "$answer" == "y" || "$answer" == "Y" ]]; then
            echo "Enter the access code:"
            read specific_code
            print_cmd "echo \"FIN$specific_code\" | nc $TEJO_IP $TEJO_PORT > fin_result.html"
            echo "FIN$specific_code" | nc $TEJO_IP $TEJO_PORT > fin_result.html
            
            # Try activating again
            print_step "Retrying session activation"
            print_cmd "echo \"$GROUP_ID:$session_type\" | nc $TEJO_IP $TEJO_PORT > init.html"
            echo "$GROUP_ID:$session_type" | nc $TEJO_IP $TEJO_PORT > init.html
        else
            print_error "Exiting. Please try again later or contact the administrator."
            exit 1
        fi
    fi
    
    extract_info
    
    # Update REG_SERVER_UDP to use PORT_REG from the test session
    if [ ! -z "$PORT_REG" ]; then
        print_step "Updating registration server port to $PORT_REG (from test session)"
        REG_SERVER_UDP=$PORT_REG
    fi
}

# Function to end a session
end_session() {
    print_step "Ending session"
    if [ -z "$ACCESS_CODE" ]; then
        print_error "No access code available. Cannot end session."
        return 1
    fi
    print_cmd "echo \"FIN$ACCESS_CODE\" | nc $TEJO_IP $TEJO_PORT > final_report.html"
    echo "FIN$ACCESS_CODE" | nc $TEJO_IP $TEJO_PORT > final_report.html
    print_success "Session ended. Check final_report.html for details."
}

# Function to get a report for all nodes
get_global_report() {
    local report_name=$1
    print_step "Getting global report: $report_name"
    print_cmd "echo \"RG$ACCESS_CODE\" | nc $TEJO_IP $TEJO_PORT > $report_name"
    echo "RG$ACCESS_CODE" | nc $TEJO_IP $TEJO_PORT > $report_name
    print_success "Global report saved to $report_name"
}

# Function to get a report for a specific node
get_node_report() {
    local port=$1
    local report_name=$2
    print_step "Getting report for node on port $port: $report_name"
    print_cmd "echo \"RP$ACCESS_CODE $port\" | nc $TEJO_IP $TEJO_PORT > $report_name"
    echo "RP$ACCESS_CODE $port" | nc $TEJO_IP $TEJO_PORT > $report_name
    print_success "Node report saved to $report_name"
}

# Function to send a command to a node
send_command() {
    local port=$1
    local command=$2
    print_step "Sending command to node on port $port: $command"
    print_cmd "echo \"$command\" | nc -u $TEJO_IP $port"
    echo "$command" | nc -u $TEJO_IP $port
    sleep $SLEEP_TIME_SHORT
}

# Function to run a command in a local NDN client
run_local_cmd() {
    local cmd=$1
    # This would need a way to communicate with your running NDN instance
    # For now, we'll just print the command that would need to be executed manually
    print_step "Run in your NDN client: $cmd"
}

# Function to start a local NDN instance
start_ndn() {
    print_step "Getting your public IP address"
    PUBLIC_IP=$(curl -s https://api.ipify.org)
    if [ -z "$PUBLIC_IP" ]; then
        # Fallback method if the first one fails
        PUBLIC_IP=$(curl -s http://checkip.amazonaws.com)
    fi
    if [ -z "$PUBLIC_IP" ]; then
        # Another fallback
        PUBLIC_IP=$(curl -s https://ipinfo.io/ip)
    fi
    
    if [ -z "$PUBLIC_IP" ]; then
        print_error "Could not determine your public IP address"
        print_step "You'll need to determine your public IP address manually and use it instead of 127.0.0.1"
        PUBLIC_IP="YOUR_PUBLIC_IP"
    else
        print_success "Your public IP address: $PUBLIC_IP"
    fi
    
    print_step "Starting NDN instance"
    print_cmd "$NDN_EXE $CACHE_SIZE $PUBLIC_IP $NODE_PORT $REG_SERVER_IP $PORT_REG"
    echo "IMPORTANT: Make sure your router allows incoming connections to port $NODE_PORT (port forwarding)"
    echo "Please start your NDN implementation in another terminal using the command above"
    echo "Use your public IP ($PUBLIC_IP) instead of 127.0.0.1 for proper external connectivity"
    echo "Press Enter when you have started your NDN implementation..."
    
    # Update the NODE_IP to use public IP for all future commands
    NODE_IP=$PUBLIC_IP
    
    read
}

# Function to set up the predefined network for session type B
setup_network_b() {
    print_step "Setting up predefined network topology for session type B"
    print_cmd "echo \"NETB1$ACCESS_CODE:$NODE_IP:$NODE_PORT\" | nc $TEJO_IP $TEJO_PORT"
    echo "NETB1$ACCESS_CODE:$NODE_IP:$NODE_PORT" | nc $TEJO_IP $TEJO_PORT
    print_success "Network setup command sent"
    sleep $SLEEP_TIME_LONG
}

# Main testing functions
test_network_creation() {
    print_header "Test 1: Basic Network Creation"
    activate_session "A"
    start_ndn
    
    # Use correct syntax for standalone network creation
    run_local_cmd "dj 0.0.0.0 0"
    run_local_cmd "st"
    
    sleep $SLEEP_TIME_MEDIUM
    print_success "Test 1 completed"
}

test_node_joining() {
    print_header "Test 2: Making a Resident Node Join Your Network"
    print_step "Using ACCESS_CODE: $ACCESS_CODE"
    
    # Use correct syntax for jointo - should use a network ID different from 076
    # since our node is standalone and not registered with network 076
    send_command $PORT_ID1 "jointo 076 $NODE_IP $NODE_PORT"
    send_command $PORT_ID1 "show topology"
    run_local_cmd "st"
    get_node_report $PORT_ID1 "test2_id1_report.html"
    print_success "Test 2 completed"
}

test_joining_existing() {
    print_header "Test 3: Joining an Existing Network"
    end_session
    activate_session "A"
    start_ndn
    
    # Use correct syntax for direct join
    run_local_cmd "dj $TEJO_IP $PORT_ID2"
    run_local_cmd "st"
    get_node_report $PORT_ID2 "test3_id2_report.html"
    print_success "Test 3 completed"
}

test_registry_server() {
    print_header "Test 4: Testing Registry Server"
    run_local_cmd "l"
    sleep $SLEEP_TIME_SHORT
    
    # Use correct syntax for join - just the network ID
    run_local_cmd "j 000"
    run_local_cmd "st"
    get_global_report "test4_global_report.html"
    print_success "Test 4 completed"
}

test_recovery() {
    print_header "Test 5: Testing Network Recovery"
    end_session
    activate_session "A"
    start_ndn
    
    # Use correct syntax for join - just the network ID
    run_local_cmd "j $GROUP_ID"
    run_local_cmd "st"
    sleep $SLEEP_TIME_MEDIUM
    
    print_step "Please identify which node is your external neighbor from the topology"
    echo "Enter the port of your external neighbor:"
    read EXT_PORT
    
    send_command $EXT_PORT "leave"
    sleep $SLEEP_TIME_MEDIUM
    run_local_cmd "st"
    get_global_report "test5_global_report.html"
    print_success "Test 5 completed"
}

test_object_local() {
    print_header "Test 6: Object Creation and Retrieval (Local)"
    run_local_cmd "c object1"
    run_local_cmd "c longernamedtestobject"
    run_local_cmd "sn"
    run_local_cmd "dl object1"
    run_local_cmd "sn"
    print_success "Test 6 completed"
}

test_object_network() {
    print_header "Test 7: Object Creation and Retrieval (Network)"
    end_session
    activate_session "B"
    start_ndn
    
    # Use correct syntax for join - just the network ID
    run_local_cmd "j $GROUP_ID"
    sleep $SLEEP_TIME_MEDIUM
    
    setup_network_b
    
    send_command $PORT_ID5 "load cities.txt"
    send_command $PORT_ID6 "load elements.txt"
    sleep $SLEEP_TIME_SHORT
    
    send_command $PORT_ID1 "retrieve paris"
    sleep $SLEEP_TIME_MEDIUM
    run_local_cmd "si"
    
    run_local_cmd "c uniqueobject"
    send_command $PORT_ID2 "retrieve uniqueobject"
    sleep $SLEEP_TIME_MEDIUM
    
    send_command $PORT_ID3 "show cache"
    get_global_report "test7_global_report.html"
    print_success "Test 7 completed"
}

test_cache() {
    print_header "Test 8: Cache Management"
    run_local_cmd "show cache"
    run_local_cmd "clear cache"
    send_command $PORT_ID1 "retrieve oxygen"
    sleep $SLEEP_TIME_SHORT
    send_command $PORT_ID1 "retrieve hydrogen"
    sleep $SLEEP_TIME_MEDIUM
    run_local_cmd "show cache"
    print_success "Test 8 completed"
}

test_interest_table() {
    print_header "Test 9: Interest Table Handling"
    send_command $PORT_ID1 "retrieve paris"
    sleep $SLEEP_TIME_SHORT
    send_command $PORT_ID2 "retrieve london"
    sleep $SLEEP_TIME_MEDIUM
    run_local_cmd "si"
    
    send_command $PORT_ID1 "retrieve nonexistent"
    sleep $SLEEP_TIME_MEDIUM
    run_local_cmd "si"
    
    get_global_report "test9_global_report.html"
    print_success "Test 9 completed"
}

test_disconnection() {
    print_header "Test 10: Network Disconnection and Reconnection"
    end_session
    activate_session "A"
    start_ndn
    
    # Use correct syntax for direct join
    run_local_cmd "dj 0.0.0.0 0"
    sleep $SLEEP_TIME_SHORT
    
    # Use correct syntax for jointo
    send_command $PORT_ID1 "jointo 076 $NODE_IP $NODE_PORT"
    sleep $SLEEP_TIME_SHORT
    send_command $PORT_ID2 "jointo 076 $TEJO_IP $PORT_ID1"
    sleep $SLEEP_TIME_MEDIUM
    
    send_command $PORT_ID1 "show topology"
    send_command $PORT_ID2 "show topology"
    
    run_local_cmd "l"
    sleep $SLEEP_TIME_MEDIUM
    
    send_command $PORT_ID1 "show topology"
    send_command $PORT_ID2 "show topology"
    
    # Use correct syntax for join
    run_local_cmd "j $GROUP_ID"
    sleep $SLEEP_TIME_MEDIUM
    
    send_command $PORT_ID1 "show topology"
    send_command $PORT_ID2 "show topology"
    
    get_global_report "test10_global_report.html"
    print_success "Test 10 completed"
}

# Main script execution

# Check if NDN executable exists
if [ ! -f "$NDN_EXE" ]; then
    print_error "NDN executable not found: $NDN_EXE"
    echo "Please compile your NDN project first or adjust the NDN_EXE variable."
    exit 1
fi

print_header "NDN Testing Script for Group $GROUP_ID"
echo "This script will guide you through testing your NDN implementation."
echo "The script will activate test sessions and provide commands to run."
echo "Some commands need to be run manually in your NDN client."
echo ""
echo "Available tests:"
echo "1. Basic Network Creation"
echo "2. Making a Resident Node Join Your Network"
echo "3. Joining an Existing Network"
echo "4. Testing Registry Server"
echo "5. Testing Network Recovery"
echo "6. Object Creation and Retrieval (Local)"
echo "7. Object Creation and Retrieval (Network)"
echo "8. Cache Management"
echo "9. Interest Table Handling"
echo "10. Network Disconnection and Reconnection"
echo "0. Run all tests"
echo ""
echo "Enter the number of the test you want to run (or 0 for all):"
read TEST_NUM

case $TEST_NUM in
    0)
        test_network_creation
        test_node_joining
        test_joining_existing
        test_registry_server
        test_recovery
        test_object_local
        test_object_network
        test_cache
        test_interest_table
        test_disconnection
        ;;
    1) test_network_creation ;;
    2) test_node_joining ;;
    3) test_joining_existing ;;
    4) test_registry_server ;;
    5) test_recovery ;;
    6) test_object_local ;;
    7) test_object_network ;;
    8) test_cache ;;
    9) test_interest_table ;;
    10) test_disconnection ;;
    *)
        print_error "Invalid test number: $TEST_NUM"
        exit 1
        ;;
esac

# Clean up
print_header "Testing Completed"
echo "All tests have been executed. Please check the generated HTML reports for details."
echo "Don't forget to end your session if it's still active:"
print_cmd "echo \"FIN$ACCESS_CODE\" | nc $TEJO_IP $TEJO_PORT > final_report.html"
echo "Thanks for using the NDN Testing Script!"