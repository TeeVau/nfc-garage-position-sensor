# Public Release Checklist

This checklist tracks the remaining work before switching the repository to public visibility.

## Current Status

- [x] Remove tracked local diagnostics and dump artifacts from the current branch
- [x] Ignore local diagnostics, dumps, and handoff files in `.gitignore`
- [x] Sanitize public-facing documentation that referenced private network details
- [x] Replace the Zigbee2MQTT helper's private default broker host with a neutral default
- [x] Re-scan the current branch for obvious private paths, host addresses, and dump content
- [ ] Add an explicit `LICENSE`
- [ ] Rewrite Git history to remove previously pushed diagnostics and dump artifacts
- [ ] Perform a final visibility review after the history rewrite

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
