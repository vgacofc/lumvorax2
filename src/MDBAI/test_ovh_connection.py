#!/usr/bin/env python3
"""
Test de connexion API OVH avec credentials Doppler
Vérifie les permissions et les endpoints disponibles
"""
import ovh
import sys
import json

def test_ovh_api():
    """Test complet de l'API OVH"""
    print("=" * 80)
    print("TEST CONNEXION API OVH - MDBAI Production")
    print("=" * 80)
    
    # Credentials depuis Doppler (prd config)
    APPLICATION_KEY = "5c081d6f5c440369"
    APPLICATION_SECRET = "28e36eb1eac62581b2134835e9ce745a"
    CONSUMER_KEY = "da67998836c9a8510bb7099bfa6f58e1"
    ENDPOINT = "ovh-eu"
    
    print(f"\n📋 Configuration:")
    print(f"   Application Key: {APPLICATION_KEY}")
    print(f"   Consumer Key: {CONSUMER_KEY[:20]}...")
    print(f"   Endpoint: {ENDPOINT}")
    
    try:
        # Initialiser client OVH
        client = ovh.Client(
            endpoint=ENDPOINT,
            application_key=APPLICATION_KEY,
            application_secret=APPLICATION_SECRET,
            consumer_key=CONSUMER_KEY
        )
        print("\n✅ Client OVH initialisé")
        
        # Test 1: Vérifier identité
        print("\n" + "=" * 80)
        print("TEST 1: Vérification identité (/me)")
        print("=" * 80)
        try:
            me = client.get('/me')
            print(f"✅ Identité récupérée:")
            print(f"   Nom: {me.get('firstname', 'N/A')} {me.get('name', 'N/A')}")
            print(f"   Email: {me.get('email', 'N/A')}")
            print(f"   NIC: {me.get('nichandle', 'N/A')}")
            print(f"   Organisation: {me.get('organisation', 'N/A')}")
        except Exception as e:
            print(f"❌ Erreur /me: {e}")
            return False
        
        # Test 2: Lister serveurs dédiés
        print("\n" + "=" * 80)
        print("TEST 2: Liste serveurs dédiés (/dedicated/server)")
        print("=" * 80)
        try:
            servers = client.get('/dedicated/server')
            print(f"✅ Serveurs trouvés: {len(servers)}")
            for server in servers:
                print(f"   - {server}")
                try:
                    details = client.get(f'/dedicated/server/{server}')
                    print(f"     IP: {details.get('ip', 'N/A')}")
                    print(f"     État: {details.get('state', 'N/A')}")
                except Exception as e:
                    print(f"     ⚠️ Détails inaccessibles: {e}")
        except Exception as e:
            print(f"❌ Erreur /dedicated/server: {e}")
        
        # Test 3: Lister VPS
        print("\n" + "=" * 80)
        print("TEST 3: Liste VPS (/vps)")
        print("=" * 80)
        try:
            vps_list = client.get('/vps')
            print(f"✅ VPS trouvés: {len(vps_list)}")
            for vps in vps_list:
                print(f"   - {vps}")
        except Exception as e:
            print(f"❌ Erreur /vps: {e}")
        
        # Test 4: Lister instances Public Cloud
        print("\n" + "=" * 80)
        print("TEST 4: Liste projets Public Cloud (/cloud/project)")
        print("=" * 80)
        try:
            projects = client.get('/cloud/project')
            print(f"✅ Projets trouvés: {len(projects)}")
            for project_id in projects:
                print(f"   - Projet: {project_id}")
                try:
                    instances = client.get(f'/cloud/project/{project_id}/instance')
                    print(f"     Instances: {len(instances)}")
                    for instance in instances:
                        print(f"       • {instance.get('name', 'N/A')} - {instance.get('status', 'N/A')}")
                except Exception as e:
                    print(f"     ⚠️ Instances inaccessibles: {e}")
        except Exception as e:
            print(f"❌ Erreur /cloud/project: {e}")
        
        # Test 5: Vérifier crédits disponibles
        print("\n" + "=" * 80)
        print("TEST 5: Crédits disponibles")
        print("=" * 80)
        try:
            balance = client.get('/me/bill/balance')
            print(f"✅ Balance: {balance}")
        except Exception as e:
            print(f"❌ Erreur /me/bill/balance: {e}")
        
        # Test 6: Tester accès au serveur 51.255.22.253
        print("\n" + "=" * 80)
        print("TEST 6: Recherche serveur 51.255.22.253")
        print("=" * 80)
        target_ip = "51.255.22.253"
        found = False
        
        # Chercher dans serveurs dédiés
        try:
            servers = client.get('/dedicated/server')
            for server in servers:
                try:
                    details = client.get(f'/dedicated/server/{server}')
                    if details.get('ip') == target_ip:
                        print(f"✅ Serveur trouvé: {server}")
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
            print(f"⚠️ Serveur {target_ip} non trouvé dans les serveurs dédiés")
            print(f"   Vérifiez que le serveur est bien associé à ce compte OVH")
        
        print("\n" + "=" * 80)
        print("RÉSUMÉ DES TESTS")
        print("=" * 80)
        print("✅ Connexion API OVH réussie")
        print("✅ Credentials valides")
        print("⚠️ Vérifiez les permissions du Consumer Key si certains endpoints échouent")
        print("\n💡 Pour créer un nouveau Consumer Key avec TOUTES les permissions:")
        print("   1. Allez sur https://eu.api.ovh.com/createToken/")
        print("   2. Cochez TOUTES les permissions (GET, POST, PUT, DELETE)")
        print("   3. Validité: Unlimited")
        print("   4. Remplacez OVH_CONSUMER_KEY dans Doppler")
        
        return True
        
    except Exception as e:
        print(f"\n❌ ERREUR FATALE: {e}")
        print(f"   Type: {type(e).__name__}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_ovh_api()
    sys.exit(0 if success else 1)

# Made with Bob
