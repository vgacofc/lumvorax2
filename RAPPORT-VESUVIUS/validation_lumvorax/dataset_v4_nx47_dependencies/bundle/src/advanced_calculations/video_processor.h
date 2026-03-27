
#ifndef VIDEO_PROCESSOR_H
#define VIDEO_PROCESSOR_H

#include "../lum/lum_core.h"
#include <stdint.h>

// Structure frame vidéo comme matrice LUM 3D
typedef struct {
    uint32_t memory_address;
    uint8_t r, g, b; // Composantes couleur
    uint16_t x, y;   // Position spatiale
    uint32_t frame_number; // Position temporelle
    double motion_vector_x, motion_vector_y; // Détection mouvement
    double presence_temporal; // Présence dans le temps
    uint64_t timestamp_frame_ns;
} video_frame_lum_t;

// Configuration traitement vidéo
typedef struct {
    uint16_t width, height;
    double fps; // 24, 30, 60, 120, 240 fps
    uint32_t total_frames;
    size_t max_frames_supported; // Minimum 100M pour conformité
    char codec_type[32]; // "H264", "H265", "AV1", "LUM_VORAX"
    bool enable_motion_detection;
    bool enable_temporal_compression;
} video_config_t;

// Fonctions principales
bool video_processor_init(video_config_t* config);
video_frame_lum_t* video_to_lum_3d_matrix(uint8_t** frame_data, size_t frame_count, uint16_t width, uint16_t height);
uint8_t** lum_3d_matrix_to_video(video_frame_lum_t* lum_frames, size_t frame_count, uint16_t width, uint16_t height);

// Transformations VORAX vidéo
bool apply_vorax_temporal_split(video_frame_lum_t* frames, size_t count);
bool apply_vorax_temporal_cycle(video_frame_lum_t* frames, size_t count);
bool apply_vorax_motion_detection(video_frame_lum_t* frames, size_t frame_count, uint16_t width, uint16_t height);
bool apply_vorax_compression(video_frame_lum_t* frames, size_t count, double compression_ratio);

// Test stress obligatoire 100M+ frames
bool video_stress_test_100m_frames(video_config_t* config);

// Métriques performance temps réel
typedef struct {
    double frames_per_second_processed;
    double realtime_factor; // 1.0 = temps réel
    double compression_ratio_achieved;
    double motion_detection_accuracy;
    size_t memory_usage_bytes;
} video_performance_metrics_t;

video_performance_metrics_t video_get_performance_metrics(void);

#endif // VIDEO_PROCESSOR_H
