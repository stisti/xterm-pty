# Emscripten 4.0 Migration Guide

## Overview

Emscripten 4.0 removed the `__proxy` annotation mechanism that xterm-pty previously relied on for cross-thread communication in PROXY_TO_PTHREAD mode. This guide explains how to use xterm-pty with Emscripten 4.0+.

## Changes Required

### For Applications Using PROXY_TO_PTHREAD

When compiling your application with `-pthread -s PROXY_TO_PTHREAD`, you now need to:

1. **Compile the C proxy helper along with your application:**

```bash
emcc your_app.c emscripten_proxy_helper.c \
  -pthread \
  -s PROXY_TO_PTHREAD \
  -s FORCE_FILESYSTEM \
  --js-library=node_modules/xterm-pty/emscripten-pty.js \
  -o your_app.mjs
```

The `emscripten_proxy_helper.c` file is distributed with xterm-pty and provides the necessary bridge between the JavaScript library and Emscripten 4.0's new proxying API.

2. **No changes needed for ASYNCIFY mode:**

If you're using `-s ASYNCIFY` instead of `-pthread -s PROXY_TO_PTHREAD`, no changes are required - the helper file is not used in this mode.

## Technical Details

### Why This Change Was Necessary

Emscripten 3.x used a `__proxy: 'async'` annotation in JavaScript library files to automatically proxy function calls to the main thread. This mechanism was removed in Emscripten 4.0 in favor of explicit use of the new `emscripten_proxy_*` functions from `<emscripten/proxying.h>`.

Since xterm-pty is a JavaScript library, it cannot directly call the C proxying API. The `emscripten_proxy_helper.c` file provides a small C shim that:

1. Exports a function callable from JavaScript (`_emscripten_pty_wait_for_readable_async`)
2. Uses the new `emscripten_proxy_async()` API to schedule work on the main thread
3. Handles the Atomics-based synchronization for PTY read operations

### Backward Compatibility

The JavaScript library code includes a fallback path for when the C helper is not available, maintaining compatibility with:
- Emscripten 3.x (using the old `__proxy` mechanism)
- ASYNCIFY mode (which doesn't require cross-thread proxying)
- Main-thread-only builds

## Troubleshooting

### Linking Errors

If you see errors like:
```
undefined symbol: _emscripten_pty_wait_for_readable_async
```

Make sure you're compiling `emscripten_proxy_helper.c` along with your application.

### Runtime Errors

If PTY input is not working, check that:
1. You're using Emscripten 4.0 or later
2. The C helper file is being compiled and linked
3. Your build includes `-pthread -s PROXY_TO_PTHREAD`

## Example

Complete example for Emscripten 4.0:

```bash
# Download xterm-pty files
wget https://unpkg.com/xterm-pty/emscripten-pty.js
wget https://unpkg.com/xterm-pty/emscripten_proxy_helper.c

# Compile with the helper
emcc example.c emscripten_proxy_helper.c \
  -pthread \
  -s PROXY_TO_PTHREAD \
  -s FORCE_FILESYSTEM \
  --js-library=emscripten-pty.js \
  -o example.mjs
```
