
#ifndef HOSTINGER_CLIENT_H
#define HOSTINGER_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Configuration pour connexion serveur Hostinger
typedef struct {
    char server_host[256];
    uint16_t server_port;
    char api_key[512];
    char ssh_key_path[256];
    bool use_ssl;
    uint32_t timeout_seconds;
    void* memory_address;
    uint32_t magic_number;
} hostinger_config_t;

// Résultat communication serveur
typedef struct {
    bool success;
    uint32_t response_code;
    char response_data[4096];
    size_t data_length;
    char error_message[256];
    uint64_t execution_time_ns;
    void* memory_address;
} hostinger_response_t;

// Fonctions principales
hostinger_config_t* hostinger_config_create(const char* host, uint16_t port, const char* api_key);
void hostinger_config_destroy(hostinger_config_t** config);

hostinger_response_t* hostinger_send_lum_data(hostinger_config_t* config, 
                                             const void* data, size_t data_size);
hostinger_response_t* hostinger_execute_remote_test(hostinger_config_t* config, 
                                                   const char* test_name);

// Préparation environnement LVAx distant
bool hostinger_prepare_lvax_environment(hostinger_config_t* config);
bool hostinger_deploy_lum_vorax_system(hostinger_config_t* config);

void hostinger_response_destroy(hostinger_response_t** response);

#endif // HOSTINGER_CLIENT_H
