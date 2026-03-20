Deterministic Python validation helpers live here.

`sitecustomize.py` is not needed by MiniZero itself. It is only for reproducible validation runs that invoke Python training code and want deterministic seeding/configuration.

Usage:

```bash
PYTHONPATH=/workspace/tools/determinism:. python ...
```

Because Python auto-imports `sitecustomize` from `sys.path`, this avoids placing a temporary `sitecustomize.py` in the repository root.
