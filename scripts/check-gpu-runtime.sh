#!/bin/bash
set -euo pipefail

if [[ ${MINIZERO_SKIP_RUNTIME_COMPAT_CHECK:-0} == 1 ]]; then
    exit 0
fi

python_bin=$(command -v python3 || command -v python || true)
if [[ -z ${python_bin} ]]; then
    echo "MiniZero runtime check failed: Python is not available." >&2
    exit 1
fi

if ! command -v nvidia-smi >/dev/null 2>&1; then
    echo "MiniZero runtime check failed: nvidia-smi is not available." >&2
    exit 1
fi

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
"${python_bin}" "${script_dir}/check_gpu_runtime.py"
