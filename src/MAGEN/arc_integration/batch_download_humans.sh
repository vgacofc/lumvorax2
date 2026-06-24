#!/bin/bash
# MAGEN Phase 4.6.4 - Batch Download ALL Human Replays
# Progress tracking in real-time

API_KEY="1617fed2-bace-426a-b591-0e0609cf3c63"
OUTPUT_DIR="./human_replays_batch"
mkdir -p "$OUTPUT_DIR"

echo "🎯 PHASE 4.6.4 - EXTRACTION MASSIVE HUMANS"
echo "=========================================="
echo ""

# Step 1: Get list of all human sessions (0%)
echo "📡 [0%] Fetching list of human sessions..."
curl -s -H "X-API-Key: $API_KEY" "https://arcprize.org/api/sessions?tags=human&limit=100" > "$OUTPUT_DIR/sessions_list.json"

# Count total sessions
TOTAL=$(python3 -c "import json; data=json.load(open('$OUTPUT_DIR/sessions_list.json')); print(len(data.get('sessions', [])))")
echo "✅ Found $TOTAL human sessions"
echo ""

# Step 2: Download each replay (10%-90%)
echo "📥 [10%] Downloading replays..."
CURRENT=0

python3 << PYTHON
import json
import requests
import time
import sys

with open('$OUTPUT_DIR/sessions_list.json') as f:
    data = json.load(f)
    sessions = data.get('sessions', [])

total = len(sessions)
for i, session in enumerate(sessions):
    session_id = session.get('guid') or session.get('id')
    game_id = session.get('game_id') or session.get('environments', [{}])[0].get('id', 'unknown')
    
    if not session_id:
        continue
    
    # Progress
    progress = 10 + int((i / total) * 80)
    print(f"📥 [{progress}%] Downloading {i+1}/{total}: {session_id[:8]}...", flush=True)
    
    # Download replay
    url = f"https://arcprize.org/api/recordings/{game_id}/{session_id}"
    headers = {"X-API-Key": "$API_KEY"}
    
    try:
        response = requests.get(url, headers=headers, timeout=30)
        if response.status_code == 200:
            output_file = f"$OUTPUT_DIR/replay_{session_id}.bin"
            with open(output_file, 'wb') as f:
                f.write(response.content)
            print(f"   ✅ Saved: {len(response.content)} bytes", flush=True)
        else:
            print(f"   ⚠️  HTTP {response.status_code}", flush=True)
    except Exception as e:
        print(f"   ❌ Error: {e}", flush=True)
    
    time.sleep(0.5)  # Rate limiting

print(f"\n✅ [90%] Download complete!", flush=True)
PYTHON

# Step 3: Parse all replays (90%-100%)
echo ""
echo "🔍 [90%] Parsing all replays..."
for replay in "$OUTPUT_DIR"/replay_*.bin; do
    if [ -f "$replay" ]; then
        python3 parse_human_replay.py "$replay" 2>&1 | grep -E "(Session|Actions|Success)" || true
    fi
done

echo ""
echo "✅ [100%] PHASE 4.6.4 COMPLETE!"
echo "=========================================="
echo "📊 Summary:"
ls -lh "$OUTPUT_DIR"/*.bin 2>/dev/null | wc -l | xargs echo "   Replays downloaded:"
ls -lh "$OUTPUT_DIR"/demo_*.json 2>/dev/null | wc -l | xargs echo "   Demonstrations parsed:"
du -sh "$OUTPUT_DIR" | awk '{print "   Total size: " $1}'
