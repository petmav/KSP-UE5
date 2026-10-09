# compare

`compare.py` diffs a candidate output against the golden data (schema `ksp-golden/1`); `tolerances.json` holds the
recommended per-quantity tolerances (first matching `dataset/table/column` glob wins; a value passes if it meets any of
abs / rel / ulp). Usage:

    python -I compare.py ..\golden <candidate_dir> --tol tolerances.json [--report report.json] [--quiet]
    python -I compare.py ..\golden\<ds>\<ds>.json <candidate.json> --tol tolerances.json

Without `--tol` every value must match exactly (0 ULP); `run.ps1` uses that mode to check the rebuilt-from-decompile IL
against the shipped IL. To produce a candidate, copy a golden file and overwrite the output columns (keep `key`
columns and inputs unchanged); extra columns and tables in the candidate are ignored, missing ones are errors.
