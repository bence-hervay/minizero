#!/usr/bin/env python

import sys


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    print("Set MINIZERO_SKIP_RUNTIME_COMPAT_CHECK=1 to bypass this check at your own risk.", file=sys.stderr)
    sys.exit(1)


try:
    import torch
except Exception as exc:  # pragma: no cover - shell script entrypoint
    fail(f"MiniZero runtime check failed: unable to import torch ({exc}).")

if torch.version.cuda is None:
    fail("MiniZero runtime check failed: the installed torch package has no CUDA support.")

try:
    visible_devices = torch.cuda.device_count()
except Exception as exc:  # pragma: no cover - shell script entrypoint
    fail(f"MiniZero runtime check failed: unable to query visible CUDA devices ({exc}).")

if visible_devices < 1:
    fail("MiniZero runtime check failed: no visible CUDA devices. MiniZero self-play currently requires at least one GPU.")

supported_arches = set(torch.cuda.get_arch_list())
unsupported_devices = []
for index in range(visible_devices):
    name = torch.cuda.get_device_name(index)
    capability = torch.cuda.get_device_capability(index)
    sm_tag = f"sm_{capability[0]}{capability[1]}"
    compute_tag = f"compute_{capability[0]}{capability[1]}"
    if sm_tag not in supported_arches and compute_tag not in supported_arches:
        unsupported_devices.append((index, name, sm_tag))

if unsupported_devices:
    print("MiniZero runtime check failed: the current PyTorch/libtorch build does not support the visible GPU architecture(s).", file=sys.stderr)
    print(f"Detected torch {torch.__version__} built for CUDA {torch.version.cuda}.", file=sys.stderr)
    print("Supported architectures: " + " ".join(sorted(supported_arches)), file=sys.stderr)
    for index, name, sm_tag in unsupported_devices:
        print(f"Visible GPU {index}: {name} ({sm_tag}) is unsupported by this runtime.", file=sys.stderr)
    print("This configuration commonly looks like a hang during the first self-play inference while CUDA JIT work spins.", file=sys.stderr)
    print("Rebuild the local container image with scripts/start-container.sh --rebuild-image or use a newer image.", file=sys.stderr)
    print("Set MINIZERO_SKIP_RUNTIME_COMPAT_CHECK=1 to bypass this check at your own risk.", file=sys.stderr)
    sys.exit(1)
