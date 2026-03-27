# RAPPORT FINAL D'EXÉCUTION V4
Date: Sat Feb 21 05:16:12 PM UTC 2026

## 1. Statut Dataset
Dataset updated: ndarray2000/nx47-dependencies (V4)

## 2. Statut Kernel
Kernel: ndarray2000/test-360-dep-lumvorax
ndarray2000/test-360-dep-lumvorax has status "KernelWorkerStatus.ERROR"

## 3. Liste des fichiers produits
total 16
drwxr-xr-x 1 runner runner  144 Feb 21 17:16 .
drwxrwxrwx 1 runner runner  148 Feb 21 17:16 ..
-rw-r--r-- 1 runner runner 4919 Feb 21 17:16 lumvorax_360_validation_report_v3_complete.json
-rw-r--r-- 1 runner runner 7742 Feb 21 17:16 test-360-dep-lumvorax.log

## 4. Rapport de validation JSON
{
  "report_name": "lumvorax_dependency_360_kaggle_single_cell_v3_complete",
  "timestamp_ns": 1771694132481071367,
  "runtime": {
    "python": "3.12.12 (main, Oct 10 2025, 08:52:57) [GCC 11.4.0]",
    "platform": "Linux-6.6.113+-x86_64-with-glibc2.35",
    "cwd": "/kaggle/working",
    "is_kaggle": true
  },
  "policy": {
    "strict_no_fallback": true,
    "require_native": true
  },
  "events": [
    {
      "ts_ns": 1771694132481168103,
      "step": "dependency_already_installed",
      "package": "numpy"
    },
    {
      "ts_ns": 1771694132730584209,
      "step": "dependency_already_installed",
      "package": "tifffile"
    },
    {
      "ts_ns": 1771694132735088969,
      "step": "dependency_install_attempt",
      "package": "imagecodecs",
      "root": "/kaggle/input/datasets/ndarray2000/nx47-dependencies",
      "cmd": [
        "/usr/bin/python3",
        "-m",
        "pip",
        "install",
        "--disable-pip-version-check",
        "--no-index",
        "/kaggle/input/datasets/ndarray2000/nx47-dependencies/imagecodecs-2026.1.14-cp311-abi3-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl"
      ]
    },
    {
      "ts_ns": 1771694138506123242,
      "step": "dependency_install_ok",
      "package": "imagecodecs",
      "root": "/kaggle/input/datasets/ndarray2000/nx47-dependencies"
    },
    {
      "ts_ns": 1771694138540003394,
      "step": "imports_checked",
      "imports": {
        "numpy": true,
        "tifffile": true,
        "imagecodecs": true,
        "pyarrow": true
      }
    },
    {
      "ts_ns": 1771694138546394298,
      "step": "dataset_paths_checked",
      "paths": [
        {
          "path": "/kaggle/input/datasets/ndarray2000/nx47-dependencies",
          "exists": true,
          "files_count": 16
        },
        {
          "path": "/kaggle/input/nx47-dependencies",
          "exists": false,
          "files_count": 0
        },
        {
          "path": "/kaggle/input/lum-vorax-dependencies",
          "exists": false,
          "files_count": 0
        },
        {
          "path": "/kaggle/input/lumvorax-dependencies",
          "exists": false,
          "files_count": 0
        }
      ]
    },
    {
      "ts_ns": 1771694138546413811,
      "step": "source_dir_missing",
      "directory": "/kaggle/working/src/lum"
    },
    {
      "ts_ns": 1771694138546425670,
      "step": "source_dir_missing",
      "directory": "/kaggle/working/src/file_formats"
    },
    {
      "ts_ns": 1771694138546436253,
      "step": "source_dir_missing",
      "directory": "/kaggle/working/src/vorax"
    },
    {
      "ts_ns": 1771694138546454297,
      "step": "source_dir_missing",
      "directory": "/kaggle/working/src/logger"
    },
    {
      "ts_ns": 1771694138546464293,
      "step": "source_dir_missing",
      "directory": "/kaggle/working/src/debug"
    },
    {
      "ts_ns": 1771694138546478541,
      "step": "source_dir_missing",
      "directory": "src/lum"
    },
    {
      "ts_ns": 1771694138546489373,
      "step": "source_dir_missing",
      "directory": "src/file_formats"
    },
    {
      "ts_ns": 1771694138546497920,
      "step": "source_dir_missing",
      "directory": "src/vorax"
    },
    {
      "ts_ns": 1771694138546505973,
      "step": "source_dir_missing",
      "directory": "src/logger"
    },
    {
      "ts_ns": 1771694138546514109,
      "step": "source_dir_missing",
      "directory": "src/debug"
    },
    {
      "ts_ns": 1771694138546515569,
      "step": "header_scan_done",
      "headers": 0,
      "functions": 0
    },
    {
      "ts_ns": 1771694138546524760,
      "step": "fatal_error",
      "error_type": "RuntimeError",
      "error": "no_native_headers_found"
    }
  ],
  "install_report": [
    {
      "package": "numpy",
      "status": "already_installed"
    },
    {
      "package": "tifffile",
      "status": "already_installed"
    },
    {
      "package": "imagecodecs",
      "status": "installed",
      "root": "/kaggle/input/datasets/ndarray2000/nx47-dependencies"
    }
  ],
  "imports": {
    "numpy": true,
    "tifffile": true,
    "imagecodecs": true,
    "pyarrow": true
  },
  "dependency_dataset_paths": [
    {
      "path": "/kaggle/input/datasets/ndarray2000/nx47-dependencies",
      "exists": true,
      "files_count": 16
    },
    {
      "path": "/kaggle/input/nx47-dependencies",
      "exists": false,
      "files_count": 0
    },
    {
      "path": "/kaggle/input/lum-vorax-dependencies",
      "exists": false,
      "files_count": 0
    },
    {
      "path": "/kaggle/input/lumvorax-dependencies",
      "exists": false,
      "files_count": 0
    }
  ],
  "module_inventory": {
    "summary": {
      "headers": 0,
      "functions": 0
    },
    "modules": []
  },
  "status": "fail",
  "error_type": "RuntimeError",
  "error": "no_native_headers_found",
  "elapsed_ns": 6065456063,
  "elapsed_s": 6.065456063
}