#!/usr/bin/env python3
import requests
import os
import json
from datetime import datetime

API_BASE = "https://blockstream.info/testnet/api"

def fetch_latest_block():
    """Récupère le dernier bloc testnet3"""
    try:
        r = requests.get(f"{API_BASE}/blocks/tip/height", timeout=10)
        height = r.json()
        
        r = requests.get(f"{API_BASE}/block-height/{height}", timeout=10)
        block_hash = r.text.strip()
        
        r = requests.get(f"{API_BASE}/block/{block_hash}", timeout=10)
        block = r.json()
        
        return {
            "height": height,
            "hash": block_hash,
            "timestamp": block["timestamp"],
            "difficulty": block.get("difficulty", "N/A"),
            "nonce": block.get("nonce", "N/A"),
            "tx_count": len(block.get("tx", []))
        }
    except Exception as e:
        return {"error": str(e)}

if __name__ == "__main__":
    print("=== Bitcoin Testnet3 Fetch ===")
    print(f"Timestamp: {datetime.now().isoformat()}")
    
    block = fetch_latest_block()
    
    if "error" in block:
        print(f"❌ Erreur: {block['error']}")
    else:
        print(f"✅ Latest block: {block['height']}")
        print(f"   Hash: {block['hash']}")
        print(f"   Difficulty: {block['difficulty']}")
        print(f"   Transactions: {block['tx_count']}")
        
        # Sauvegarde résultats
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_file = f"test_results_c153/testnet3_{timestamp}.json"
        with open(output_file, "w") as f:
            json.dump(block, f, indent=2)
        
        print(f"✅ Résultats sauvegardés: {output_file}")
