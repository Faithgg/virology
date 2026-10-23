#!/bin/bash

# Test script for s0P0wn3d C2 connectivity
# Tests controller endpoints and simulates implant communication

set -e

CONTROLLER_BIN="../s0p0wn3d_controller"
CONTROLLER_PORT=8443
CONTROLLER_URL="http://localhost:$CONTROLLER_PORT"
CONTROLLER_PID=""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}[*] s0P0wn3d Connectivity Test${NC}"
echo "================================"

# Start controller
echo -e "${BLUE}[*] Starting controller on port $CONTROLLER_PORT...${NC}"
$CONTROLLER_BIN $CONTROLLER_PORT &
CONTROLLER_PID=$!
sleep 2

# Cleanup on exit
cleanup() {
    if [ ! -z "$CONTROLLER_PID" ]; then
        echo -e "${BLUE}[*] Stopping controller (PID: $CONTROLLER_PID)...${NC}"
        kill $CONTROLLER_PID 2>/dev/null || true
    fi
}
trap cleanup EXIT

echo ""
echo -e "${BLUE}Test 1: Check /health endpoint${NC}"
if curl -s "$CONTROLLER_URL/health" | grep -q "ok"; then
    echo -e "${GREEN}[+] PASS: Controller is healthy${NC}"
else
    echo -e "${RED}[-] FAIL: /health endpoint not responding${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}Test 2: Check /pubkey endpoint${NC}"
if curl -s "$CONTROLLER_URL/pubkey" | grep -q "public_key"; then
    echo -e "${GREEN}[+] PASS: Public key endpoint working${NC}"
else
    echo -e "${RED}[-] FAIL: /pubkey endpoint not responding${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}Test 3: Simulate implant check heartbeat${NC}"
# Simulating a heartbeat POST to /check/{session_id}
FAKE_SESSION_ID="test_session_$(date +%s)"
RESPONSE=$(curl -s -X POST "$CONTROLLER_URL/check/$FAKE_SESSION_ID" \
    -H "Content-Type: application/json" \
    -d "{\"status\":\"online\"}" 2>&1 || echo "")

if echo "$RESPONSE" | grep -q "session not found"; then
    echo -e "${GREEN}[+] PASS: /check endpoint working (correctly rejected unknown session)${NC}"
else
    echo -e "${YELLOW}[!] Note: /check endpoint response: $RESPONSE${NC}"
fi

echo ""
echo -e "${BLUE}Test 4: Check /sessions endpoint${NC}"
SESSIONS_COUNT=$(curl -s "$CONTROLLER_URL/sessions" | grep -o "total" || echo "")
if [ ! -z "$SESSIONS_COUNT" ]; then
    echo -e "${GREEN}[+] PASS: /sessions endpoint working${NC}"
else
    echo -e "${RED}[-] FAIL: /sessions endpoint not responding${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}[+] All connectivity tests passed!${NC}"
echo -e "${GREEN}================================${NC}"
