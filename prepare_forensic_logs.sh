
#!/bin/bash

echo "=== PRÉPARATION LOGS FORENSIQUES HAUTE SÉCURITÉ ==="
echo "Date: $(date -u)"
echo "Session: forensic_$(date +%Y%m%d_%H%M%S)"

# Création structure logs avec protection contre écrasement
SESSION_ID="forensic_$(date +%Y%m%d_%H%M%S)"
echo "$SESSION_ID" > logs/current_session_forensic.txt

# Structure logs forensiques complète
mkdir -p logs/forensic/{compilation,execution,stress_tests,memory_analysis,performance,validation}
mkdir -p logs/forensic/compilation/{build_logs,warnings,errors}
mkdir -p logs/forensic/execution/{main_demo,console,stress_million}
mkdir -p logs/forensic/stress_tests/{100m_lums,memory_leaks,performance}
mkdir -p logs/forensic/memory_analysis/{tracker_logs,allocation_maps,leak_detection}
mkdir -p logs/forensic/performance/{benchmarks,metrics,comparisons}
mkdir -p logs/forensic/validation/{crypto,integrity,compliance}

# Protection contre écrasement - archivage des logs existants
if [ -d "logs/forensic_archive" ]; then
    mv logs/forensic_archive "logs/forensic_archive_$(date +%Y%m%d_%H%M%S)"
fi
mkdir -p logs/forensic_archive

# Template métadonnées forensiques
cat > logs/forensic/metadata_template.json << 'EOF'
{
    "session_id": "%SESSION_ID%",
    "timestamp_start": "%TIMESTAMP_START%",
    "timestamp_end": "%TIMESTAMP_END%",
    "system_info": {
        "os": "%OS_INFO%",
        "compiler": "%COMPILER_VERSION%",
        "architecture": "%ARCH%"
    },
    "test_parameters": {
        "test_type": "%TEST_TYPE%",
        "duration_seconds": "%DURATION%",
        "memory_limit_mb": "%MEMORY_LIMIT%"
    },
    "checksums": {
        "source_files": "%SOURCE_CHECKSUM%",
        "binary": "%BINARY_CHECKSUM%"
    },
    "validation": {
        "memory_leaks": "%MEMORY_STATUS%",
        "performance_baseline": "%PERF_STATUS%",
        "compliance_check": "%COMPLIANCE_STATUS%"
    }
}
EOF

echo "✅ Logs forensiques préparés avec session: $SESSION_ID"
echo "✅ Protection anti-écrasement activée"
echo "✅ Métadonnées forensiques configurées"

chmod +x prepare_forensic_logs.sh
