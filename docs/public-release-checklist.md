# Public Release Checklist

This checklist tracks the remaining work before switching the repository to public visibility.

## Current Status

- [x] Remove tracked local diagnostics and dump artifacts from the current branch
- [x] Ignore local diagnostics, dumps, and handoff files in `.gitignore`
- [x] Sanitize public-facing documentation that referenced private network details
- [x] Replace the Zigbee2MQTT helper's private default broker host with a neutral default
- [x] Re-scan the current branch for obvious private paths, host addresses, and dump content
- [x] Add an explicit `LICENSE`
- [x] Rewrite Git history to remove previously pushed diagnostics and dump artifacts
- [x] Perform a final visibility review after the history rewrite

## Checklist

### Current State

- [x] Create a list of files that should not be public
- [x] Check which of those files were already versioned
- [x] Decide which files are merely untidy versus genuinely unsuitable for a public repository

### Remove From The Current Repository State

- [x] Remove `docs/router-capacity-test-*` from the tracked repository content
- [x] Remove `docs/z2m-dump-*` from the tracked repository content
- [x] Remove `docs/*handoff*.json` from the tracked repository content
- [x] Keep `serial.log` local-only and ignored
- [x] Check public-facing docs for dead links or stale references after the cleanup

### `.gitignore`

- [x] Ignore `docs/router-capacity-test-*/`
- [x] Ignore `docs/z2m-dump-*/`
- [x] Ignore `docs/*handoff*.json`
- [x] Ignore `serial.log`
- [x] Ignore other local diagnostic working files such as `docs/garage-debug-checklist.md`

### Make The Repository Public-Ready

- [x] Add `LICENSE`
- [x] Review `README.md` for public-facing wording and remove references that imply local private artifacts belong in the repo
- [x] Remove the private default broker address from `tools/z2m-join.ps1`
- [x] Run a final content scan for local paths, private host addresses, and obvious secrets in the current branch

### History

- [x] Decide whether a history rewrite is needed
- [x] Confirm that a history rewrite is needed because previously pushed diagnostics and dumps still exist in older commits
- [x] Remove the affected paths from Git history
- [x] Validate that the cleaned history no longer exposes the removed files
- [x] Force-push the rewritten branch

### Final Go-Live Check

- [ ] `git status` is clean
- [x] No sensitive diagnostics or dump files remain in the current branch
- [x] No sensitive diagnostics or dump files remain in Git history
- [x] License file exists in the repository root
- [x] `README.md` and project docs are consistent with the current repository structure
- [ ] Only then switch the repository visibility to public

## Files Removed From The Current Branch

- `docs/router-capacity-test-*`
- `docs/z2m-dump-*`
- `docs/*handoff*.json`

These paths are now ignored for future local work, but old commits still contain them.

## History Rewrite Required

The following commits still contain files that should not remain in a future public repository:

- `4a7a6b2`
- `5dcaa19`

Before making the repository public, remove the affected paths from Git history and then force-push the rewritten branch.

## Recommended Final Sequence

1. Choose and add a project license.
2. Rewrite Git history to purge previously committed local diagnostics and dumps.
3. Verify that the rewritten history no longer exposes those files.
4. Run one last repository review for public-facing wording and metadata.
5. Switch the repository visibility to public.
