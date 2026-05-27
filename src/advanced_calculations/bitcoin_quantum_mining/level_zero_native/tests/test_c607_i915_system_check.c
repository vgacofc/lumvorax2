/*
 * TEST_059 - Vérification Configuration Système i915
 * 
 * Objectif: Diagnostiquer pourquoi zeInit() échoue (0x70010000)
 * 
 * Vérifications:
 * 1. Module kernel i915 chargé
 * 2. Device DRI accessible (/dev/dri/renderD128)
 * 3. Permissions utilisateur (groupes video/render)
 * 4. Firmware GPU présent
 * 5. Version driver i915
 * 6. Logs kernel i915
 * 7. Hardware GPU détecté
 * 8. Capacités DRM
 * 
 * ROOT CAUSE #156: Level Zero zeInit() échoue
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>

// DRM headers
#include <drm/i915_drm.h>
#include <xf86drm.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

typedef struct {
    int total_checks;
    int passed_checks;
    int failed_checks;
    int warning_checks;
} test_results_t;

static test_results_t results = {0};

void print_header(const char *title) {
    printf("\n");
    printf(COLOR_CYAN "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  %-56s  ║\n", title);
    printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
}

void print_check(const char *name, int status, const char *details) {
    results.total_checks++;
    
    if (status == 0) {
        printf(COLOR_GREEN "✓" COLOR_RESET " %s", name);
        results.passed_checks++;
    } else if (status == 1) {
        printf(COLOR_YELLOW "⚠" COLOR_RESET " %s", name);
        results.warning_checks++;
    } else {
        printf(COLOR_RED "✗" COLOR_RESET " %s", name);
        results.failed_checks++;
    }
    
    if (details && strlen(details) > 0) {
        printf(": %s", details);
    }
    printf("\n");
}

int check_module_loaded(void) {
    FILE *fp = fopen("/proc/modules", "r");
    if (!fp) {
        print_check("Module i915 chargé", 2, "Impossible de lire /proc/modules");
        return -1;
    }
    
    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "i915 ", 5) == 0) {
            found = 1;
            // Extraire taille et usage
            char *token = strtok(line, " ");
            token = strtok(NULL, " "); // size
            char size[64];
            snprintf(size, sizeof(size), "Taille: %s bytes", token);
            token = strtok(NULL, " "); // usage count
            char usage[128];
            snprintf(usage, sizeof(usage), "%s, Usage: %s", size, token);
            print_check("Module i915 chargé", 0, usage);
            break;
        }
    }
    fclose(fp);
    
    if (!found) {
        print_check("Module i915 chargé", 2, "Module NON chargé");
        return -1;
    }
    
    return 0;
}

int check_module_version(void) {
    FILE *fp = popen("modinfo i915 2>/dev/null | grep '^version:' | head -1", "r");
    if (!fp) {
        print_check("Version module i915", 1, "modinfo non disponible");
        return -1;
    }
    
    char line[256];
    if (fgets(line, sizeof(line), fp)) {
        // Supprimer newline
        line[strcspn(line, "\n")] = 0;
        print_check("Version module i915", 0, line);
        pclose(fp);
        return 0;
    }
    
    pclose(fp);
    print_check("Version module i915", 1, "Version non trouvée");
    return -1;
}

int check_dri_devices(void) {
    DIR *dir = opendir("/dev/dri");
    if (!dir) {
        print_check("Répertoire /dev/dri", 2, "N'existe pas");
        return -1;
    }
    
    print_check("Répertoire /dev/dri", 0, "Existe");
    
    struct dirent *entry;
    int card_count = 0;
    int render_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "card", 4) == 0) {
            card_count++;
            char path[256];
            snprintf(path, sizeof(path), "/dev/dri/%s", entry->d_name);
            
            struct stat st;
            if (stat(path, &st) == 0) {
                char details[256];
                snprintf(details, sizeof(details), "Permissions: %o, Propriétaire: %d:%d",
                         st.st_mode & 0777, st.st_uid, st.st_gid);
                print_check(path, 0, details);
            }
        } else if (strncmp(entry->d_name, "renderD", 7) == 0) {
            render_count++;
            char path[256];
            snprintf(path, sizeof(path), "/dev/dri/%s", entry->d_name);
            
            struct stat st;
            if (stat(path, &st) == 0) {
                char details[256];
                snprintf(details, sizeof(details), "Permissions: %o, Propriétaire: %d:%d",
                         st.st_mode & 0777, st.st_uid, st.st_gid);
                print_check(path, 0, details);
            }
        }
    }
    closedir(dir);
    
    if (card_count == 0) {
        print_check("Devices card*", 2, "Aucun trouvé");
    }
    if (render_count == 0) {
        print_check("Devices renderD*", 2, "Aucun trouvé");
    }
    
    return (card_count > 0 && render_count > 0) ? 0 : -1;
}

int check_user_permissions(void) {
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    
    if (!pw) {
        print_check("Utilisateur actuel", 2, "Impossible de déterminer");
        return -1;
    }
    
    char user_info[256];
    snprintf(user_info, sizeof(user_info), "%s (UID: %d)", pw->pw_name, uid);
    print_check("Utilisateur actuel", 0, user_info);
    
    // Vérifier groupes
    int ngroups = 0;
    getgrouplist(pw->pw_name, pw->pw_gid, NULL, &ngroups);
    
    gid_t *groups = malloc(ngroups * sizeof(gid_t));
    getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups);
    
    int has_video = 0;
    int has_render = 0;
    
    for (int i = 0; i < ngroups; i++) {
        struct group *gr = getgrgid(groups[i]);
        if (gr) {
            if (strcmp(gr->gr_name, "video") == 0) has_video = 1;
            if (strcmp(gr->gr_name, "render") == 0) has_render = 1;
        }
    }
    free(groups);
    
    if (has_video) {
        print_check("Groupe 'video'", 0, "Membre");
    } else {
        print_check("Groupe 'video'", 1, "NON membre (peut causer problèmes)");
    }
    
    if (has_render) {
        print_check("Groupe 'render'", 0, "Membre");
    } else {
        print_check("Groupe 'render'", 1, "NON membre (peut causer problèmes)");
    }
    
    return (has_video || has_render) ? 0 : -1;
}

int check_firmware(void) {
    const char *firmware_paths[] = {
        "/lib/firmware/i915",
        "/usr/lib/firmware/i915",
        NULL
    };
    
    int found = 0;
    for (int i = 0; firmware_paths[i] != NULL; i++) {
        DIR *dir = opendir(firmware_paths[i]);
        if (dir) {
            print_check("Firmware i915", 0, firmware_paths[i]);
            
            // Compter fichiers
            struct dirent *entry;
            int count = 0;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_name[0] != '.') count++;
            }
            closedir(dir);
            
            char details[128];
            snprintf(details, sizeof(details), "%d fichiers firmware trouvés", count);
            print_check("Fichiers firmware", 0, details);
            
            found = 1;
            break;
        }
    }
    
    if (!found) {
        print_check("Firmware i915", 1, "Répertoire non trouvé");
    }
    
    return found ? 0 : -1;
}

int check_gpu_hardware(void) {
    // Vérifier via sysfs
    DIR *dir = opendir("/sys/class/drm");
    if (!dir) {
        print_check("GPU Hardware", 2, "Impossible de lire /sys/class/drm");
        return -1;
    }
    
    struct dirent *entry;
    int gpu_found = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "card", 4) == 0) {
            char vendor_path[512];
            snprintf(vendor_path, sizeof(vendor_path), 
                     "/sys/class/drm/%s/device/vendor", entry->d_name);
            
            FILE *fp = fopen(vendor_path, "r");
            if (fp) {
                char vendor[32];
                if (fgets(vendor, sizeof(vendor), fp)) {
                    vendor[strcspn(vendor, "\n")] = 0;
                    
                    if (strcmp(vendor, "0x8086") == 0) {
                        gpu_found = 1;
                        
                        // Lire device ID
                        char device_path[512];
                        snprintf(device_path, sizeof(device_path),
                                 "/sys/class/drm/%s/device/device", entry->d_name);
                        
                        FILE *fp2 = fopen(device_path, "r");
                        if (fp2) {
                            char device[32];
                            if (fgets(device, sizeof(device), fp2)) {
                                device[strcspn(device, "\n")] = 0;
                                
                                char details[256];
                                snprintf(details, sizeof(details),
                                         "%s - Vendor: %s, Device: %s",
                                         entry->d_name, vendor, device);
                                print_check("GPU Intel détecté", 0, details);
                            }
                            fclose(fp2);
                        }
                    }
                }
                fclose(fp);
            }
        }
    }
    closedir(dir);
    
    if (!gpu_found) {
        print_check("GPU Intel détecté", 2, "Aucun GPU Intel trouvé");
        return -1;
    }
    
    return 0;
}

int check_drm_capabilities(void) {
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        char error[256];
        snprintf(error, sizeof(error), "Impossible d'ouvrir: %s", strerror(errno));
        print_check("Ouverture /dev/dri/renderD128", 2, error);
        return -1;
    }
    
    print_check("Ouverture /dev/dri/renderD128", 0, "Succès");
    
    // Vérifier version DRM
    drmVersionPtr version = drmGetVersion(fd);
    if (version) {
        char details[256];
        snprintf(details, sizeof(details), "%s version %d.%d.%d",
                 version->name, version->version_major,
                 version->version_minor, version->version_patchlevel);
        print_check("Version DRM", 0, details);
        drmFreeVersion(version);
    } else {
        print_check("Version DRM", 1, "Impossible de lire");
    }
    
    // Vérifier capacités i915
    struct drm_i915_getparam gp;
    int value;
    
    // HAS_EXECBUF2
    gp.param = I915_PARAM_HAS_EXECBUF2;
    gp.value = &value;
    if (ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp) == 0) {
        print_check("I915_PARAM_HAS_EXECBUF2", value ? 0 : 2,
                    value ? "Supporté" : "NON supporté");
    } else {
        print_check("I915_PARAM_HAS_EXECBUF2", 2, "Erreur ioctl");
    }
    
    // HAS_GPU_RESET
    gp.param = I915_PARAM_HAS_GPU_RESET;
    gp.value = &value;
    if (ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp) == 0) {
        print_check("I915_PARAM_HAS_GPU_RESET", value ? 0 : 1,
                    value ? "Supporté" : "NON supporté");
    }
    
    // CHIPSET_ID
    gp.param = I915_PARAM_CHIPSET_ID;
    gp.value = &value;
    if (ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp) == 0) {
        char details[128];
        snprintf(details, sizeof(details), "0x%04x", value);
        print_check("I915_PARAM_CHIPSET_ID", 0, details);
    }
    
    // EU_TOTAL
    gp.param = I915_PARAM_EU_TOTAL;
    gp.value = &value;
    if (ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp) == 0) {
        char details[128];
        snprintf(details, sizeof(details), "%d EUs", value);
        print_check("I915_PARAM_EU_TOTAL", 0, details);
    } else {
        print_check("I915_PARAM_EU_TOTAL", 1, "Non disponible");
    }
    
    // SUBSLICE_TOTAL
    gp.param = I915_PARAM_SUBSLICE_TOTAL;
    gp.value = &value;
    if (ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp) == 0) {
        char details[128];
        snprintf(details, sizeof(details), "%d subslices", value);
        print_check("I915_PARAM_SUBSLICE_TOTAL", 0, details);
    }
    
    close(fd);
    return 0;
}

int check_kernel_logs(void) {
    FILE *fp = popen("dmesg | grep -i i915 | tail -20", "r");
    if (!fp) {
        print_check("Logs kernel i915", 1, "dmesg non accessible");
        return -1;
    }
    
    char line[512];
    int error_count = 0;
    int warning_count = 0;
    int info_count = 0;
    
    printf("\n" COLOR_YELLOW "Derniers logs kernel i915:\n" COLOR_RESET);
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        
        if (strstr(line, "error") || strstr(line, "ERROR")) {
            printf(COLOR_RED "  %s\n" COLOR_RESET, line);
            error_count++;
        } else if (strstr(line, "warn") || strstr(line, "WARN")) {
            printf(COLOR_YELLOW "  %s\n" COLOR_RESET, line);
            warning_count++;
        } else {
            printf("  %s\n", line);
            info_count++;
        }
    }
    pclose(fp);
    
    char summary[256];
    snprintf(summary, sizeof(summary), "%d erreurs, %d warnings, %d infos",
             error_count, warning_count, info_count);
    
    if (error_count > 0) {
        print_check("Logs kernel i915", 2, summary);
        return -1;
    } else if (warning_count > 0) {
        print_check("Logs kernel i915", 1, summary);
        return 0;
    } else {
        print_check("Logs kernel i915", 0, summary);
        return 0;
    }
}

int main(void) {
    printf(COLOR_MAGENTA "\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST_059 - VÉRIFICATION CONFIGURATION SYSTÈME i915       ║\n");
    printf("║  ROOT CAUSE #156: Level Zero zeInit() échoue              ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
    
    print_header("1. MODULE KERNEL i915");
    check_module_loaded();
    check_module_version();
    
    print_header("2. DEVICES DRI");
    check_dri_devices();
    
    print_header("3. PERMISSIONS UTILISATEUR");
    check_user_permissions();
    
    print_header("4. FIRMWARE GPU");
    check_firmware();
    
    print_header("5. HARDWARE GPU");
    check_gpu_hardware();
    
    print_header("6. CAPACITÉS DRM");
    check_drm_capabilities();
    
    print_header("7. LOGS KERNEL");
    check_kernel_logs();
    
    // Résumé final
    printf("\n");
    printf(COLOR_CYAN "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSUMÉ DIAGNOSTIC                                         ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    
    printf("\nTotal vérifications: %d\n", results.total_checks);
    printf(COLOR_GREEN "  ✓ Réussies: %d\n" COLOR_RESET, results.passed_checks);
    printf(COLOR_YELLOW "  ⚠ Warnings: %d\n" COLOR_RESET, results.warning_checks);
    printf(COLOR_RED "  ✗ Échecs: %d\n" COLOR_RESET, results.failed_checks);
    
    double success_rate = (double)results.passed_checks / results.total_checks * 100.0;
    printf("\nTaux de réussite: %.1f%%\n", success_rate);
    
    if (results.failed_checks == 0 && results.warning_checks == 0) {
        printf(COLOR_GREEN "\n✅ Configuration i915 PARFAITE\n" COLOR_RESET);
        printf("   → Le problème zeInit() est ailleurs\n");
        printf("   → Vérifier Level Zero lui-même\n");
        return 0;
    } else if (results.failed_checks > 0) {
        printf(COLOR_RED "\n❌ Configuration i915 PROBLÉMATIQUE\n" COLOR_RESET);
        printf("   → Corriger les échecs avant continuer\n");
        printf("   → zeInit() échouera tant que i915 est cassé\n");
        return 1;
    } else {
        printf(COLOR_YELLOW "\n⚠ Configuration i915 ACCEPTABLE\n" COLOR_RESET);
        printf("   → Warnings peuvent causer instabilités\n");
        printf("   → Recommandé de corriger\n");
        return 0;
    }
}

// Made with Bob
