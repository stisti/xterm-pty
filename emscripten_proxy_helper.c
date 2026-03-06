#ifdef __EMSCRIPTEN__
#include <emscripten/threading.h>
#include <emscripten/proxying.h>
#include <pthread.h>
#include <emscripten.h>

// Data structure for proxying PTY wait operation to main thread
typedef struct {
    int atomic_index;
    int result;
} PTYWaitData;

// This function will be called on the main thread
static void pty_wait_on_main_thread(void *arg) {
    PTYWaitData *data = (PTYWaitData *)arg;

    // Call JavaScript function to wait for PTY readable
    int result = EM_ASM_INT({
        var atomicIndex = $0;
        var result = -1;

        if (Module.pty && Module.pty.onReadable) {
            Module.PTY_waitForReadableWithCallback(function(type) {
                Atomics.store(HEAP32, atomicIndex, type);
                Atomics.notify(HEAP32, atomicIndex);
            });
            result = 0;
        }

        return result;
    }, data->atomic_index);

    data->result = result;
}

// Exported function that JavaScript can call to proxy PTY wait to main thread
EMSCRIPTEN_KEEPALIVE
int emscripten_pty_wait_for_readable_async(int atomic_index) {
    PTYWaitData data = {
        .atomic_index = atomic_index,
        .result = -1
    };

    em_proxying_queue *queue = emscripten_proxy_get_system_queue();
    pthread_t main_thread = emscripten_main_runtime_thread_id();

    if (!emscripten_proxy_async(queue, main_thread, pty_wait_on_main_thread, &data)) {
        return -1;
    }

    return 0;
}

#endif // __EMSCRIPTEN__
