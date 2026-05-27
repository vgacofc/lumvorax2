# Inventaire modules/fonctions LUMVORAX (local)

- Headers scannés: **14**
- Fonctions publiques détectées: **211**

## src/lum/lum_core.h
- Rôle: LUM core structures/security/memory
- Fonctions: 27
  - `lum_create`: `lum_t*` (uint8_t presence, int32_t x, int32_t y, lum_structure_type_e type)
  - `lum_destroy`: `void` (lum_t* lum)
  - `lum_group_create`: `lum_group_t*` (size_t initial_capacity)
  - `lum_group_destroy`: `void` (lum_group_t* group)
  - `lum_group_safe_destroy`: `void` (lum_group_t** group_ptr)
  - `lum_group_add`: `bool` (lum_group_t* group, lum_t* lum)
  - `lum_group_get`: `lum_t*` (lum_group_t* group, size_t index)
  - `lum_group_size`: `size_t` (lum_group_t* group)
  - `lum_zone_create`: `lum_zone_t*` (const char* name)
  - `lum_zone_destroy`: `void` (lum_zone_t* zone)
  - `lum_zone_add_group`: `bool` (lum_zone_t* zone, lum_group_t* group)
  - `lum_zone_is_empty`: `bool` (lum_zone_t* zone)
  - `lum_memory_create`: `lum_memory_t*` (const char* name)
  - `lum_memory_destroy`: `void` (lum_memory_t* memory)
  - `lum_memory_store`: `bool` (lum_memory_t* memory, lum_group_t* group)
  - `lum_memory_retrieve`: `lum_group_t*` (lum_memory_t* memory)
  - `lum_generate_id`: `uint32_t` (void)
  - `lum_get_timestamp`: `uint64_t` (void)
  - `lum_print`: `void` (const lum_t* lum)
  - `lum_group_print`: `void` (const lum_group_t* group)
  - `lum_group_process_batch_50m_optimized`: `bool` (lum_group_t* group, lum_batch_operation_e operation)
  - `lum_group_sort_ultra_fast`: `bool` (lum_group_t* group)
  - `lum_group_defragment_zero_copy`: `bool` (lum_group_t* group)
  - `lum_safe_destroy`: `void` (lum_t** lum_ptr)
  - `lum_group_destroy_ultra_secure`: `void` (lum_group_t** group_ptr)
  - `lum_security_init`: `bool` (void)
  - `lum_security_cleanup`: `void` (void)

## src/lum/lum_optimized_variants.h
- Rôle: LUM core structures/security/memory
- Fonctions: 1
  - `lum_get_timestamp`: `extern uint64_t` (void)

## src/file_formats/lum_native_file_handler.h
- Rôle: Universal .lum format encoding/decoding and file manager
- Fonctions: 74
  - `lum_file_config_create`: `lum_file_config_t*` (lum_file_format_e format)
  - `lum_file_config_destroy`: `void` (lum_file_config_t** config_ptr)
  - `lum_file_config_set_format`: `bool` (lum_file_config_t* config, lum_file_format_e format)
  - `lum_file_config_enable_compression`: `bool` (lum_file_config_t* config, double level)
  - `lum_file_config_set_buffer_size`: `bool` (lum_file_config_t* config, size_t size)
  - `lum_file_metadata_create`: `lum_file_metadata_t*` (void)
  - `lum_file_metadata_destroy`: `void` (lum_file_metadata_t** metadata_ptr)
  - `lum_file_metadata_update`: `bool` (lum_file_metadata_t* metadata, const lum_group_t* group)
  - `lum_file_metadata_validate`: `bool` (const lum_file_metadata_t* metadata)
  - `lum_file_context_create`: `lum_file_context_t*` (const char* filename, lum_file_config_t* config, bool writing)
  - `lum_file_context_destroy`: `void` (lum_file_context_t** context_ptr)
  - `lum_file_context_open`: `bool` (lum_file_context_t* context, const char* filename, const char* mode)
  - `lum_file_context_close`: `bool` (lum_file_context_t* context)
  - `lum_export_single`: `lum_file_result_t*` (const lum_t* lum, const char* filename, lum_file_config_t* config)
  - `lum_export_single_binary`: `lum_file_result_t*` (const lum_t* lum, const char* filename, lum_file_config_t* config)
  - `lum_export_single_text`: `lum_file_result_t*` (const lum_t* lum, const char* filename, lum_file_config_t* config)
  - `lum_export_single_json`: `lum_file_result_t*` (const lum_t* lum, const char* filename, lum_file_config_t* config)
  - `lum_export_single_xml`: `lum_file_result_t*` (const lum_t* lum, const char* filename, lum_file_config_t* config)
  - `lum_export_group`: `lum_file_result_t*` (const lum_group_t* group, const char* filename, lum_file_config_t* config)
  - `lum_export_group_binary`: `lum_file_result_t*` (const lum_group_t* group, const char* filename, lum_file_config_t* config)
  - `lum_export_group_text`: `lum_file_result_t*` (const lum_group_t* group, const char* filename, lum_file_config_t* config)
  - `lum_export_group_json`: `lum_file_result_t*` (const lum_group_t* group, const char* filename, lum_file_config_t* config)
  - `lum_export_group_xml`: `lum_file_result_t*` (const lum_group_t* group, const char* filename, lum_file_config_t* config)
  - `lum_export_group_csv`: `lum_file_result_t*` (const lum_group_t* group, const char* filename, lum_file_config_t* config)
  - `lum_export_multi_groups`: `lum_file_result_t*` (lum_group_t** groups, size_t group_count, const char* filename, lum_file_config_t* config)
  - `lum_import_single`: `lum_file_result_t*` (lum_t** lum, const char* filename, lum_file_config_t* config)
  - `lum_import_single_binary`: `lum_file_result_t*` (lum_t** lum, const char* filename, lum_file_config_t* config)
  - `lum_import_single_text`: `lum_file_result_t*` (lum_t** lum, const char* filename, lum_file_config_t* config)
  - `lum_import_single_json`: `lum_file_result_t*` (lum_t** lum, const char* filename, lum_file_config_t* config)
  - `lum_import_single_xml`: `lum_file_result_t*` (lum_t** lum, const char* filename, lum_file_config_t* config)
  - `lum_import_group`: `lum_file_result_t*` (lum_group_t** group, const char* filename, lum_file_config_t* config)
  - `lum_import_group_binary`: `lum_file_result_t*` (lum_group_t** group, const char* filename, lum_file_config_t* config)
  - `lum_import_group_text`: `lum_file_result_t*` (lum_group_t** group, const char* filename, lum_file_config_t* config)
  - `lum_import_group_json`: `lum_file_result_t*` (lum_group_t** group, const char* filename, lum_file_config_t* config)
  - `lum_import_group_xml`: `lum_file_result_t*` (lum_group_t** group, const char* filename, lum_file_config_t* config)
  - `lum_import_group_csv`: `lum_file_result_t*` (lum_group_t** group, const char* filename, lum_file_config_t* config)
  - `lum_import_multi_groups`: `lum_file_result_t*` (lum_group_t*** groups, size_t* group_count, const char* filename, lum_file_config_t* config)
  - `lum_file_detect_format`: `lum_file_format_e` (const char* filename)
  - `lum_file_validate_format`: `bool` (const char* filename, lum_file_format_e expected_format)
  - `lum_file_is_valid_lum_file`: `bool` (const char* filename)
  - `lum_file_verify_integrity`: `bool` (const char* filename, lum_file_config_t* config)
  - `lum_file_calculate_checksum`: `uint32_t` (const char* filename)
  - `lum_file_repair_corruption`: `bool` (const char* filename, lum_file_config_t* config)
  - `lum_file_stats_create`: `lum_file_stats_t*` (void)
  - `lum_file_stats_destroy`: `void` (lum_file_stats_t** stats_ptr)
  - `lum_file_stats_update`: `bool` (lum_file_stats_t* stats, const lum_file_result_t* result)
  - `lum_file_get_info`: `lum_file_result_t*` (const char* filename)
  - `lum_file_stress_test_100m_export`: `bool` (lum_file_config_t* config)
  - `lum_file_stress_test_100m_import`: `bool` (lum_file_config_t* config)
  - `lum_file_stress_test_all_formats`: `bool` (size_t lum_count, lum_file_config_t* config)
  - `lum_file_benchmark_formats`: `lum_file_result_t*` (size_t lum_count, lum_file_config_t* config)
  - `lum_calculate_crc32`: `uint32_t` (const uint8_t* data, size_t length)
  - `lum_write_serialized`: `bool` (FILE* file, const lum_t* lum)
  - `lum_read_serialized`: `bool` (FILE* file, lum_t* lum)
  - `lum_write_metadata_serialized`: `bool` (FILE* file, const lum_file_metadata_t* metadata)
  - `lum_read_metadata_serialized`: `bool` (FILE* file, lum_file_metadata_t* metadata)
  - `lum_file_verify_integrity_file`: `bool` (const char* filename)
  - `lum_file_stress_test_100m_write_read`: `bool` (void)
  - `lum_file_test_corruption_recovery`: `bool` (lum_file_config_t* config)
  - `lum_file_test_large_files`: `bool` (size_t max_size_gb, lum_file_config_t* config)
  - `lum_file_test_concurrent_access`: `bool` (size_t thread_count, lum_file_config_t* config)
  - `lum_file_result_create`: `lum_file_result_t*` (void)
  - `lum_file_result_destroy`: `void` (lum_file_result_t** result_ptr)
  - `lum_file_result_set_success`: `bool` (lum_file_result_t* result, const char* filename, size_t bytes, size_t lums)
  - `lum_file_result_set_error`: `bool` (lum_file_result_t* result, const char* error_message)
  - `lum_file_config_create_default`: `lum_file_config_t*` (void)
  - `lum_file_config_create_adaptive`: `lum_file_config_t*` (size_t expected_lum_count)
  - `lum_file_config_create_high_performance`: `lum_file_config_t*` (void)
  - `lum_file_config_create_high_compression`: `lum_file_config_t*` (void)
  - `lum_file_config_create_forensic_audit`: `lum_file_config_t*` (void)
  - `lum_file_get_error_messages`: `lum_file_error_messages_t*` (void)
  - `lum_file_set_custom_error_messages`: `bool` (const lum_file_error_messages_t* messages)
  - `lum_file_get_optimal_buffer_size`: `size_t` (void)
  - `lum_file_calculate_buffer_size_for_count`: `size_t` (size_t lum_count)

## src/file_formats/lum_native_universal_format.h
- Rôle: Universal .lum format encoding/decoding and file manager
- Fonctions: 14
  - `lum_get_format_version`: `lum_format_version_t` (void)
  - `lum_set_format_version`: `bool` (uint16_t major, uint16_t minor, const char* build_info)
  - `lum_get_content_type_name`: `const char*` (lum_content_type_e type)
  - `lum_set_custom_content_type_names`: `bool` (const char* names[], size_t count)
  - `lum_universal_file_create`: `lum_universal_file_manager_t*` (const char* filepath)
  - `lum_universal_file_open`: `lum_universal_file_manager_t*` (const char* filepath, bool read_only)
  - `lum_universal_file_save_and_close`: `bool` (lum_universal_file_manager_t** manager_ptr)
  - `lum_universal_file_destroy`: `void` (lum_universal_file_manager_t** manager_ptr)
  - `lum_export_to_json`: `bool` (const char* input_lum_file, const char* output_json_file)
  - `lum_export_to_csv`: `bool` (const char* input_lum_file, const char* output_csv_file)
  - `lum_calculate_file_checksum`: `uint32_t` (const char* lum_file_path)
  - `lum_stress_test_100m_elements`: `bool` (const char* test_file_path)
  - `lum_get_file_info`: `lum_file_info_t*` (const char* lum_file_path)
  - `lum_file_info_destroy`: `void` (lum_file_info_t** info_ptr)

## src/file_formats/lum_secure_serialization.h
- Rôle: Universal .lum format encoding/decoding and file manager
- Fonctions: 8
  - `lum_secure_serialize_group`: `lum_secure_result_t*` (lum_group_t* group, bool encrypt)
  - `lum_secure_deserialize_group`: `lum_group_t*` (const uint8_t* data, size_t data_size, bool decrypt)
  - `lum_secure_save_to_file`: `bool` (const char* filename, lum_group_t* group, bool encrypt)
  - `lum_secure_load_from_file`: `lum_group_t*` (const char* filename, bool decrypt)
  - `lum_secure_calculate_checksum`: `uint32_t` (const uint8_t* data, size_t size)
  - `lum_secure_verify_integrity`: `bool` (const lum_secure_header_t* header, const uint8_t* data)
  - `lum_secure_stress_test_100m_serialization`: `bool` (void)
  - `lum_secure_result_destroy`: `void` (lum_secure_result_t** result_ptr)

## src/vorax/vorax_3d_volume.h
- Rôle: VORAX 3D operators/processing
- Fonctions: 3
  - `vorax_volume3d_validate`: `int` (const vorax_volume3d_t* vol)
  - `vorax_volume3d_normalize`: `int` (vorax_volume3d_t* vol)
  - `vorax_volume3d_threshold`: `int` (const vorax_volume3d_t* vol, float threshold, uint8_t* out_mask, size_t out_len)

## src/vorax/vorax_operations.h
- Rôle: VORAX 3D operators/processing
- Fonctions: 16
  - `vorax_fuse`: `vorax_result_t*` (lum_group_t* group1, lum_group_t* group2)
  - `vorax_split`: `vorax_result_t*` (lum_group_t* group, size_t parts)
  - `vorax_cycle`: `vorax_result_t*` (lum_group_t* group, size_t modulo)
  - `vorax_move`: `vorax_result_t*` (lum_zone_t* from_zone, lum_zone_t* to_zone, size_t lum_count)
  - `vorax_store`: `vorax_result_t*` (lum_memory_t* memory, lum_zone_t* zone, size_t lum_count)
  - `vorax_retrieve`: `vorax_result_t*` (lum_memory_t* memory, lum_zone_t* zone)
  - `vorax_compress`: `vorax_result_t*` (lum_group_t* group)
  - `vorax_expand`: `vorax_result_t*` (lum_group_t* compressed_group, size_t parts)
  - `vorax_result_create`: `vorax_result_t*` (void)
  - `vorax_result_destroy`: `void` (vorax_result_t* result)
  - `vorax_result_safe_destroy`: `void` (vorax_result_t** result_ptr)
  - `vorax_result_set_success`: `void` (vorax_result_t* result, const char* message)
  - `vorax_result_set_error`: `void` (vorax_result_t* result, const char* message)
  - `vorax_count_total_lums`: `size_t` (lum_group_t** groups, size_t group_count)
  - `vorax_create_node`: `vorax_result_t*` (lum_group_t* group1, lum_group_t* group2)
  - `vorax_emit_lums`: `vorax_result_t*` (lum_zone_t* zone, size_t count)

## src/logger/log_manager.h
- Rôle: Forensic/logging/monitoring
- Fonctions: 7
  - `log_manager_create`: `log_manager_t*` (void)
  - `log_manager_create_structure`: `bool` (log_manager_t* manager)
  - `log_manager_archive_session`: `bool` (log_manager_t* manager, const char* session_id)
  - `log_manager_get_module_logger`: `module_logger_t*` (log_manager_t* manager, const char* module_name)
  - `log_manager_destroy`: `void` (log_manager_t* manager)
  - `log_manager_get_instance`: `log_manager_t*` (void)
  - `LOG_MODULE`: `void` (const char* module, const char* level, const char* format, ...)

## src/logger/lum_logger.h
- Rôle: Forensic/logging/monitoring
- Fonctions: 25
  - `lum_logger_create`: `lum_logger_t*` (const char* log_filename, bool console_output, bool file_output)
  - `lum_logger_destroy`: `void` (lum_logger_t* logger)
  - `lum_logger_set_level`: `bool` (lum_logger_t* logger, lum_log_level_e level)
  - `lum_logger_enable_tracing`: `bool` (lum_logger_t* logger, bool enable)
  - `lum_log_error`: `void` (lum_logger_t* logger, const char* error_message)
  - `lum_log_analyze`: `lum_log_analysis_t*` (const char* log_filename)
  - `lum_log_print_analysis`: `void` (const lum_log_analysis_t* analysis)
  - `lum_log_analysis_destroy`: `void` (lum_log_analysis_t* analysis)
  - `lum_log_replay_create`: `lum_log_replay_t*` (const char* log_filename)
  - `lum_log_replay_destroy`: `void` (lum_log_replay_t* replay)
  - `lum_log_replay_execute`: `bool` (lum_log_replay_t* replay, struct vorax_execution_context* ctx)
  - `lum_set_global_logger`: `void` (lum_logger_t* logger)
  - `lum_get_global_logger`: `lum_logger_t*` (void)
  - `lum_log_message`: `void` (lum_logger_t* logger, lum_log_level_e level, const char* message)
  - `lum_log_level_to_string`: `const char*` (lum_log_level_e level)
  - `lum_log_entry_type_to_string`: `const char*` (lum_log_entry_type_e type)
  - `vorax_operation_to_string`: `const char*` (vorax_operation_e operation)
  - `lum_log_export_csv`: `bool` (const char* log_filename, const char* csv_filename)
  - `lum_log_export_json`: `bool` (const char* log_filename, const char* json_filename)
  - `lum_log_monitor_destroy`: `void` (lum_log_monitor_t* monitor)
  - `lum_log_monitor_start`: `bool` (lum_log_monitor_t* monitor)
  - `lum_log_monitor_stop`: `void` (lum_log_monitor_t* monitor)
  - `lum_log_flush`: `void` (void)
  - `lum_log`: `void` (lum_log_level_e level, const char* format, ...)
  - `lum_logf`: `void` (lum_log_level_e level, const char* format, ...)

## src/debug/enhanced_logging.h
- Rôle: Forensic/logging/monitoring
- Fonctions: 4
  - `create_log_directory`: `void` (const char* base_path)
  - `create_enhanced_log`: `void` (const char* filepath, const char* message)
  - `log_module_action_enhanced`: `void` (const char* module_name, const char* action)
  - `log_test_execution`: `void` (const char* test_name, const char* result, int line_count)

## src/debug/forensic_logger.h
- Rôle: Forensic/logging/monitoring
- Fonctions: 7
  - `forensic_logger_init`: `bool` (const char* filename)
  - `forensic_logger_init_individual_files`: `bool` (void)
  - `forensic_log_memory_operation`: `void` (const char* operation, void* ptr, size_t size)
  - `forensic_log_lum_operation`: `void` (const char* operation, uint64_t lum_count, double duration_ns)
  - `forensic_log_individual_lum`: `void` (uint32_t lum_id, const char* operation, uint64_t timestamp_ns)
  - `forensic_logger_destroy`: `void` (void)
  - `forensic_log`: `void` (forensic_level_e level, const char* function, const char* format, ...)

## src/debug/logging_system.h
- Rôle: Forensic/logging/monitoring
- Fonctions: 3
  - `create_log`: `void` (const char* filepath, const char* message)
  - `log_module_action`: `void` (const char* module_name, const char* action)
  - `log_error_safe`: `void` (const char* module_name, const char* error_msg)

## src/debug/memory_tracker.h
- Rôle: Forensic/logging/monitoring
- Fonctions: 17
  - `memory_tracker_init`: `void` (void)
  - `memory_tracker_cleanup`: `void` (void)
  - `memory_tracker_check_leaks`: `void` (void)
  - `memory_tracker_destroy`: `void` (void)
  - `memory_tracker_alloc`: `void` (void* ptr, size_t size, const char* file, int line)
  - `memory_tracker_free`: `void` (void* ptr, const char* file, int line)
  - `memory_tracker_report`: `void` (void)
  - `tracked_malloc`: `void*` (size_t size, const char* file, int line, const char* func)
  - `tracked_free`: `void` (void* ptr, const char* file, int line, const char* func)
  - `tracked_calloc`: `void*` (size_t nmemb, size_t size, const char* file, int line, const char* func)
  - `tracked_realloc`: `void*` (void* ptr, size_t size, const char* file, int line, const char* func)
  - `memory_tracker_enable`: `void` (bool enable)
  - `memory_tracker_is_enabled`: `bool` (void)
  - `memory_tracker_export_json`: `void` (const char* filename)
  - `memory_tracker_set_release_mode`: `void` (bool release_mode)
  - `memory_tracker_cleanup`: `void` (void)
  - `memory_tracker_get_current_usage`: `size_t` (void)

## src/debug/ultra_forensic_logger.h
- Rôle: Forensic/logging/monitoring
- Fonctions: 5
  - `ultra_forensic_logger_init_lum`: `void` (const char* log_file)
  - `ultra_forensic_logger_init`: `bool` (void)
  - `ultra_forensic_logger_destroy`: `void` (void)
  - `ultra_forensic_validate_all_logs_exist`: `bool` (void)
  - `ultra_forensic_generate_summary_report`: `void` (void)

