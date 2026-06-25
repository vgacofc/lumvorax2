#!/usr/bin/env python3
"""
Test de connexion API OVH avec NOUVEAUX credentials (MDBAI-Production-1)
"""
import ovh
import sys

def test_ovh_api():
    print("=" * 80)
    print("TEST CONNEXION API OVH - NOUVEAUX CREDENTIALS")
    print("=" * 80)
    
    # NOUVEAUX credentials (MDBAI-Production-1)
    APPLICATION_KEY = "59f86de7e76ab0e7"
    APPLICATION_SECRET = "504272afdef5c00709cf38e653741d43"
    CONSUMER_KEY = "08fd2bca229ce4d34ecb1f91edc84268"
    ENDPOINT = "ovh-eu"
    
    print(f"\n📋 Configuration:")
    print(f"   Application: MDBAI-Production-1")
    print(f"   Application Key: {APPLICATION_KEY}")
    print(f"   Consumer Key: {CONSUMER_KEY[:20]}...")
    print(f"   Endpoint: {ENDPOINT}")
    
    try:
        client = ovh.Client(
            endpoint=ENDPOINT,
            application_key=APPLICATION_KEY,
            application_secret=APPLICATION_SECRET,
            consumer_key=CONSUMER_KEY
        )
        print("\n✅ Client OVH initialisé")
        
        # Test 1: /me
        print("\n" + "=" * 80)
        print("TEST 1: Identité (/me)")
        print("=" * 80)
        try:
            me = client.get('/me')
            print(f"✅ SUCCÈS!")
            print(f"   Nom: {me.get('firstname', 'N/A')} {me.get('name', 'N/A')}")
            print(f"   Email: {me.get('email', 'N/A')}")
            print(f"   NIC: {me.get('nichandle', 'N/A')}")
        except Exception as e:
            print(f"❌ ÉCHEC: {e}")
            return False
        
        # Test 2: Serveurs dédiés
        print("\n" + "=" * 80)
        print("TEST 2: Serveurs dédiés (/dedicated/server)")
        print("=" * 80)
        try:
            servers = client.get('/dedicated/server')
            print(f"✅ {len(servers)} serveur(s) trouvé(s)")
            for server in servers:
                print(f"   - {server}")
                try:
                    details = client.get(f'/dedicated/server/{server}')
                    print(f"     IP: {details.get('ip', 'N/A')}")
                    print(f"     État: {details.get('state', 'N/A')}")
                except Exception as e:
                    print(f"     ⚠️ Détails: {e}")
        except Exception as e:
            print(f"❌ ÉCHEC: {e}")
        
        # Test 3: VPS
        print("\n" + "=" * 80)
        print("TEST 3: VPS (/vps)")
        print("=" * 80)
        try:
            vps_list = client.get('/vps')
            print(f"✅ {len(vps_list)} VPS trouvé(s)")
            for vps in vps_list:
                print(f"   - {vps}")
        except Exception as e:
            print(f"❌ ÉCHEC: {e}")
        
        # Test 4: Public Cloud
        print("\n" + "=" * 80)
        print("TEST 4: Public Cloud (/cloud/project)")
        print("=" * 80)
        try:
            projects = client.get('/cloud/project')
            print(f"✅ {len(projects)} projet(s) trouvé(s)")
            for project_id in projects:
                print(f"   - Projet: {project_id}")
                try:
                    instances = client.get(f'/cloud/project/{project_id}/instance')
                    print(f"     Instances: {len(instances)}")
                except Exception as e:
                    print(f"     ⚠️ {e}")
        except Exception as e:
            print(f"❌ ÉCHEC: {e}")
        
        # Test 5: Recherche serveur 51.255.22.253
        print("\n" + "=" * 80)
        print("TEST 5: Recherche serveur 51.255.22.253")
        print("=" * 80)
        target_ip = "51.255.22.253"
        found = False
        
        try:
            servers = client.get('/dedicated/server')
            for server in servers:
                try:
                    details = client.get(f'/dedicated/server/{server}')
                    if details.get('ip') == target_ip:
                        print(f"✅ TROUVÉ: {server}")
                        print(f"   IP: {details.get('ip')}")
                        print(f"   État: {details.get('state')}")
                        print(f"   Datacenter: {details.get('datacenter', 'N/A')}")
                        found = True
                        break
                except:
                    pass
        except:
            pass
        
        if not found:
            print(f"⚠️ Serveur {target_ip} non trouvé")
        
        print("\n" + "=" * 80)
        print("✅ TESTS TERMINÉS - CONNEXION OVH RÉUSSIE")
        print("=" * 80)
        return True
        
    except Exception as e:
        print(f"\n❌ ERREUR: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_ovh_api()
    sys.exit(0 if success else 1)

# Made with Bob
