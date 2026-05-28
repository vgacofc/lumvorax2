---
name: MDBAI npm persistence
description: node_modules in src/MDBAI/ do not persist between Replit sessions
---

node_modules in src/MDBAI/ are lost between Replit sessions.

**Why:** Replit's ephemeral filesystem doesn't persist node_modules across sessions.

**How to apply:** At the start of every session working on MDBAI, run:
1. `cd src/MDBAI && npm install` (760 packages, ~30s)
2. `make -C src/MDBAI/forensic` (compiles libmdbai_forensic.so, 17KB)
3. Then run tests: `NODE_OPTIONS=--experimental-vm-modules NODE_ENV=test npx jest --forceExit --no-coverage`
