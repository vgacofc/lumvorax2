"""
MAGEN Phase 4.6.4 - Scraper Leaderboard ARC Prize
Extrait tous les replay IDs depuis le leaderboard public
"""

import requests
from bs4 import BeautifulSoup
import json
import re
import sys

def scrape_leaderboard_page(url: str) -> list:
    """Scrape une page du leaderboard"""
    print(f"📡 Scraping: {url}")
    
    try:
        response = requests.get(url, timeout=10)
        if response.status_code != 200:
            print(f"❌ HTTP {response.status_code}")
            return []
        
        soup = BeautifulSoup(response.text, 'html.parser')
        
        # Chercher les liens vers /replay/
        replay_links = soup.find_all('a', href=re.compile(r'/replay/[a-f0-9-]+'))
        
        replay_ids = []
        for link in replay_links:
            href = link.get('href', '')
            match = re.search(r'/replay/([a-f0-9-]+)', href)
            if match:
                replay_id = match.group(1)
                if replay_id not in replay_ids:
                    replay_ids.append(replay_id)
        
        print(f"✅ Found {len(replay_ids)} replay IDs")
        return replay_ids
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return []

def scrape_all_leaderboards() -> dict:
    """Scrape tous les leaderboards disponibles"""
    print("=" * 60)
    print("🎯 SCRAPING ARC PRIZE LEADERBOARDS")
    print("=" * 60)
    
    leaderboards = {
        'community': 'https://arcprize.org/leaderboard/community',
        'verified': 'https://arcprize.org/leaderboard',
    }
    
    all_replays = {}
    
    for name, url in leaderboards.items():
        print(f"\n📊 Leaderboard: {name}")
        replay_ids = scrape_leaderboard_page(url)
        all_replays[name] = replay_ids
        print(f"   Total: {len(replay_ids)} replays")
    
    # Combine and deduplicate
    unique_replays = set()
    for replays in all_replays.values():
        unique_replays.update(replays)
    
    print(f"\n✅ Total unique replays: {len(unique_replays)}")
    
    return {
        'by_leaderboard': all_replays,
        'unique_replay_ids': sorted(list(unique_replays)),
        'total_count': len(unique_replays)
    }

if __name__ == "__main__":
    result = scrape_all_leaderboards()
    
    # Save to JSON
    output_file = "human_replays_batch/replay_ids.json"
    with open(output_file, 'w') as f:
        json.dump(result, f, indent=2)
    
    print(f"\n💾 Saved to: {output_file}")
    print("\n" + "=" * 60)
    print("✅ SCRAPING COMPLETE")
    print("=" * 60)

# Made with Bob
