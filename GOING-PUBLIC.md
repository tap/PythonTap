# Going public — cut-over checklist

This branch (`claude/tap-python-public-audit-2xrgln`) contains the full modernization
pass from the July 2026 audit. This file is the checklist for taking the repository
public, and should be deleted as part of the final cut-over.

## Why a fresh start is required

The existing git history permanently contains ~111MB of pack data (two committed
Python 3.10 runtimes, removed by this branch but still in every clone) and commits
authored under a work email (`tim.place@garmin.com`). Flipping the repo public as-is
ships both forever. The plan chosen in the audit: **start public history from a
single clean root commit.**

## Cut-over procedure

0. **Rename the repository to `tap.python`** (GitHub → Settings → General →
   Rename). Old `tap/python` URLs, clones, and remotes redirect automatically.
   Rationale: a git clone's folder name *is* the Max package name (Min derives
   `C74_PACKAGE_NAME` from it), so the repo name should match the `tap.python~`
   object rather than squatting the generic name "python" — and it makes the
   repo findable once public. The badge URLs and package-info website field in
   the tree already point at `tap/tap.python`.

1. Merge (or fast-forward) this branch's tree to `main` locally and make sure you're
   happy with the result. Do **not** push yet.

2. Verify everything works (see pre-flight below).

3. Create the clean root — this keeps the working tree exactly as-is and writes one
   new commit with no parents:

   ```sh
   git checkout --orphan public-main
   git add -A
   git rm --cached GOING-PUBLIC.md && rm GOING-PUBLIC.md
   git commit -m "tap.python: Max package for writing Max objects in Python"
   git branch -M public-main main
   git push --force-with-lease origin main
   ```

4. Delete the now-stale branches on the remote (`claude/*`, and any other WIP), and
   have anyone with old clones re-clone.

5. Repository settings on GitHub:
   - make the repository public
   - default branch `main`, protect it (require CI green)
   - description + topics (`max`, `maxmsp`, `python`, `dsp`, `livecoding`)
   - enable Issues; disable Wiki/Projects unless wanted

## Pre-flight checklist

- [ ] CI green on both platforms (`.github/workflows/build.yml`)
- [ ] `scripts/install-runtime.sh` run on a real Mac; `[tap.python~]` and
      `[tap.python~ allpass]` load and pass audio in Max 9 (Apple Silicon **native**,
      not Rosetta)
- [ ] same on Windows with `scripts/install-runtime.ps1`
- [ ] hot-reload exercised in Max: edit/save `python/default.py` while DSP is on;
      confirm reload posts to the console and audio resumes
- [ ] attribute round-trip in Max: set `gain` in the object box, `getgain` works,
      an attrs validator rejection (e.g. `alpha 2.0` on allpass) posts an error
      instead of crashing
- [ ] help patcher opens clean in Max 9 (no missing objects, no stray fonts)
- [ ] secret/PII scan on the final tree (the audit found none in source; the
      vendored runtimes and hardcoded home paths are already gone)
- [ ] `License.md` third-party section still matches what the install script
      installs (Python version, packages)
- [ ] after the first push to `main`, the ReadMe build badge shows green — it
      points at `main`, so it reads "no status" until the cut-over push
      triggers the workflow there (and renders publicly only once the repo
      is public)

## Known limitations to track as issues once public (not blockers)

- single-channel only; tuple returns from `process()` are rejected with a console
  message — multichannel / multi-outlet support is future work
- per-sample Python calls are CPU-heavy; a vectorized `process(numpy array)` path
  is the planned fix
- all instances share one interpreter (CPython embedding limitation); a heavy
  script in one instance affects the others
- brief audio dropout while a source reload holds the GIL
- `docs/tap.python~.maxref.xml` is minimal; flesh out messages/attributes sections
  as the dynamic API stabilizes
