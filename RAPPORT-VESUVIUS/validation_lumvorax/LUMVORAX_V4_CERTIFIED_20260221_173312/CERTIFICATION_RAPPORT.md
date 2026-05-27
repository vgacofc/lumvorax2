# RAPPORT DE CERTIFICATION LUMVORAX V4.4
Date: Sat Feb 21 05:33:14 PM UTC 2026

## 1. Statut Global
Dataset: ndarray2000/nx47-dependencies (V4.3 - 31 fichiers, incluant headers)
Kernel: ndarray2000/test-360-dep-lumvorax (V4.4 - Correction paths)

## 2. Analyse du JSON
{
  "report_name": "lumvorax_dependency_360_kaggle_single_cell_v3_complete",
  "timestamp_ns": 1771695143487372157,
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
      "ts_ns": 1771695143487447713,
      "step": "dependency_already_installed",
      "package": "numpy"
    },
    {
      "ts_ns": 1771695143699389413,
      "step": "dependency_already_installed",
      "package": "tifffile"
    },
    {
      "ts_ns": 1771695143706587744,
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
      "ts_ns": 1771695148729346047,
      "step": "dependency_install_ok",
      "package": "imagecodecs",
      "root": "/kaggle/input/datasets/ndarray2000/nx47-dependencies"
    },
    {
      "ts_ns": 1771695148758669870,
      "step": "imports_checked",
      "imports": {
        "numpy": true,
        "tifffile": true,
        "imagecodecs": true,
        "pyarrow": true
      }
    },
    {
      "ts_ns": 1771695148782411449,
      "step": "dataset_paths_checked",
      "paths": [
        {
          "path": "/kaggle/input/datasets/ndarray2000/nx47-dependencies",
          "exists": true,
          "files_count": 30
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
      "ts_ns": 1771695148782429019,
      "step": "source_dir_missing",
      "directory": "/kaggle/working/src/lum"
    },
    {
      "ts_ns": 1771695148782442216,
      "step": "source_dir_missing",
      "directory": "/kaggle/working/src/file_formats"
    },
    {
      "ts_ns": 1771695148782451931,
      "step": "source_dir_missing",
      "directory": "/kaggle/working/src/vorax"
    },
    {
      "ts_ns": 1771695148782461511,
      "step": "source_dir_missing",
      "directory": "/kaggle/working/src/logger"
    },
    {
      "ts_ns": 1771695148782470328,
      "step": "source_dir_missing",
      "directory": "/kaggle/working/src/debug"
    },
    {
      "ts_ns": 1771695148782482301,
      "step": "source_dir_missing",
      "directory": "src/lum"
    },
    {
      "ts_ns": 1771695148782491756,
      "step": "source_dir_missing",
      "directory": "src/file_formats"
    },
    {
      "ts_ns": 1771695148782499546,
      "step": "source_dir_missing",
      "directory": "src/vorax"
    },
    {
      "ts_ns": 1771695148782507094,
      "step": "source_dir_missing",
      "directory": "src/logger"
    },
    {
      "ts_ns": 1771695148782514008,
      "step": "source_dir_missing",
      "directory": "src/debug"
    },
    {
      "ts_ns": 1771695148782515254,
      "step": "header_scan_done",
      "headers": 0,
      "functions": 0
    },
    {
      "ts_ns": 1771695148782522361,
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
      "files_count": 30
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
  "elapsed_ns": 5295152344,
  "elapsed_s": 5.295152344
}
## 3. Logs d'exécution
,{"stream_name":"stdout","time":10.494022797,"data":"Requirement already satisfied: numpy in /usr/local/lib/python3.12/dist-packages (from imagecodecs==2026.1.14) (2.0.2)\n"}
,{"stream_name":"stdout","time":10.696660081,"data":"Requirement already satisfied: numpy in /usr/local/lib/python3.12/dist-packages (from imagecodecs==2026.1.14) (2.0.2)\n"}
,{"stream_name":"stdout","time":12.463278428,"data":"Installing collected packages: imagecodecs\n"}
,{"stream_name":"stdout","time":12.665964382,"data":"Installing collected packages: imagecodecs\n"}
,{"stream_name":"stdout","time":12.985131936,"data":"Successfully installed imagecodecs-2026.1.14\n"}
,{"stream_name":"stdout","time":13.106036093,"data":"Successfully installed imagecodecs-2026.1.14\n"}
,{"stream_name":"stdout","time":13.106064752,"data":"{\n"}
,{"stream_name":"stdout","time":13.106069148,"data":"  \"status\": \"fail\",\n"}
,{"stream_name":"stdout","time":13.106072845,"data":"  \"error_type\": \"RuntimeError\",\n"}
,{"stream_name":"stdout","time":13.106076521,"data":"  \"error\": \"no_native_headers_found\",\n"}
,{"stream_name":"stdout","time":13.10608029,"data":"  \"report\": \"/kaggle/working/lumvorax_360_validation_report_v3_complete.json\",\n"}
,{"stream_name":"stdout","time":13.106082921,"data":"  \"module_headers\": 0,\n"}
,{"stream_name":"stdout","time":13.10608521,"data":"  \"public_functions\": 0,\n"}
,{"stream_name":"stdout","time":13.106087415,"data":"  \"roundtrip_status\": null,\n"}
,{"stream_name":"stdout","time":13.106089671,"data":"  \"write_compression_used\": null,\n"}
,{"stream_name":"stdout","time":13.106091889,"data":"  \"events_count\": 18,\n"}
,{"stream_name":"stdout","time":13.106094143,"data":"  \"elapsed_ns\": 5295152344\n"}
,{"stream_name":"stdout","time":13.106106057,"data":"}\n"}
,{"stream_name":"stderr","time":13.110176732,"data":"/usr/local/lib/python3.12/dist-packages/IPython/core/interactiveshell.py:3561: UserWarning: To exit: use 'exit', 'quit', or Ctrl-D.\n"}
,{"stream_name":"stderr","time":13.11019994,"data":"  warn(\"To exit: use 'exit', 'quit', or Ctrl-D.\", stacklevel=1)\n"}
,{"stream_name":"stderr","time":13.110205849,"data":"\n"}
,{"stream_name":"stderr","time":13.110209519,"data":"/usr/local/lib/python3.12/dist-packages/IPython/core/interactiveshell.py:3561: UserWarning: To exit: use 'exit', 'quit', or Ctrl-D.\n"}
,{"stream_name":"stderr","time":13.11021399,"data":"  warn(\"To exit: use 'exit', 'quit', or Ctrl-D.\", stacklevel=1)\n"}
,{"stream_name":"stderr","time":13.434090583,"data":"Traceback (most recent call last):\n"}
,{"stream_name":"stderr","time":13.434120418,"data":"  File \"\u003cstring\u003e\", line 1, in \u003cmodule\u003e\n"}
,{"stream_name":"stderr","time":13.434125614,"data":"  File \"/usr/local/lib/python3.12/dist-packages/papermill/execute.py\", line 131, in execute_notebook\n"}
,{"stream_name":"stderr","time":13.434287267,"data":"    raise_for_execution_errors(nb, output_path)\n"}
,{"stream_name":"stderr","time":13.434317,"data":"  File \"/usr/local/lib/python3.12/dist-packages/papermill/execute.py\", line 251, in raise_for_execution_errors\n"}
,{"stream_name":"stderr","time":13.434326884,"data":"    raise error\n"}
,{"stream_name":"stderr","time":13.434576519,"data":"papermill.exceptions.PapermillExecutionError: \n"}
,{"stream_name":"stderr","time":13.434590882,"data":"---------------------------------------------------------------------------\n"}
,{"stream_name":"stderr","time":13.434594701,"data":"Exception encountered at \"In [1]\":\n"}
,{"stream_name":"stderr","time":13.434598065,"data":"An exception has occurred, use %tb to see the full traceback.\n"}
,{"stream_name":"stderr","time":13.434601623,"data":"\n"}
,{"stream_name":"stderr","time":13.434604835,"data":"SystemExit: 2\n"}
,{"stream_name":"stderr","time":13.434607945,"data":"\n"}
,{"stream_name":"stderr","time":13.434611051,"data":"\n"}
,{"stream_name":"stderr","time":15.504164564,"data":"/usr/local/lib/python3.12/dist-packages/mistune.py:435: SyntaxWarning: invalid escape sequence '\\|'\n"}
,{"stream_name":"stderr","time":15.504194017,"data":"  cells[i][c] = re.sub('\\\\\\\\\\|', '|', cell)\n"}
,{"stream_name":"stderr","time":15.647578437,"data":"/usr/local/lib/python3.12/dist-packages/nbconvert/filters/filter_links.py:36: SyntaxWarning: invalid escape sequence '\\_'\n"}
,{"stream_name":"stderr","time":15.64760506,"data":"  text = re.sub(r'_', '\\_', text) # Escape underscores in display text\n"}
,{"stream_name":"stderr","time":16.194094392,"data":"/usr/local/lib/python3.12/dist-packages/traitlets/traitlets.py:2915: FutureWarning: --Exporter.preprocessors=[\"remove_papermill_header.RemovePapermillHeader\"] for containers is deprecated in traitlets 5.0. You can pass `--Exporter.preprocessors item` ... multiple times to add items to a list.\n"}
,{"stream_name":"stderr","time":16.194130596,"data":"  warn(\n"}
,{"stream_name":"stderr","time":16.220365776,"data":"[NbConvertApp] Converting notebook __notebook__.ipynb to notebook\n"}
,{"stream_name":"stderr","time":16.506469993,"data":"[NbConvertApp] Writing 20749 bytes to __notebook__.ipynb\n"}
,{"stream_name":"stderr","time":18.784994777,"data":"/usr/local/lib/python3.12/dist-packages/traitlets/traitlets.py:2915: FutureWarning: --Exporter.preprocessors=[\"nbconvert.preprocessors.ExtractOutputPreprocessor\"] for containers is deprecated in traitlets 5.0. You can pass `--Exporter.preprocessors item` ... multiple times to add items to a list.\n"}
,{"stream_name":"stderr","time":18.785041352,"data":"  warn(\n"}
,{"stream_name":"stderr","time":18.806649392,"data":"[NbConvertApp] Converting notebook __notebook__.ipynb to html\n"}
,{"stream_name":"stderr","time":19.654624503,"data":"[NbConvertApp] Writing 349574 bytes to __results__.html\n"}
]