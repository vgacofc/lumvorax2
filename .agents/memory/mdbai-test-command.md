---
name: MDBAI test command
description: Exact command to run all 170 MDBAI tests
---

**Command (from src/MDBAI/ directory):**
```
NODE_OPTIONS=--experimental-vm-modules NODE_ENV=test npx jest --forceExit --no-coverage
```

**Expected result:** 170 passed, 20 suites

**Why:** ESM requires --experimental-vm-modules. NODE_ENV=test activates test stubs. --forceExit prevents hang on open handles (Redis, Telegram polling).

**Structure:** tests/unit/ (001-010, 90 tests) + tests/integration/ (011-015, 50 tests) + tests/e2e/ (021-025, 30 tests)
