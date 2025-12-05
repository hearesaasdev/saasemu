// libretro_loader.cpp
// Responsible for dlopen core, resolving libretro symbols, callbacks, retro_run thread.
// Exposes C-style functions that native_bridge.cpp will call.

#include <dlfcn.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstring>

#define LOG_TAG "LibRetroLoader"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Minimal libretro typedefs
typedef bool (*retro_environment_t)(unsigned, void*);
typedef void (*retro_set_environment_t)(retro_environment_t);
typedef void (*retro_set_video_refresh_t)(void (*)(const void*, unsigned, unsigned, size_t));
typedef void (*retro_set_audio_sample_t)(void (*)(int16_t, int16_t));
typedef void (*retro_set_audio_sample_batch_t)(size_t (*)(const int16_t*, size_t));
typedef void (*retro_set_input_poll_t)(void (*)(void));
typedef void (*retro_set_input_state_t)(int16_t (*)(unsigned, unsigned, unsigned, unsigned));
typedef void (*retro_init_t)(void);
typedef void (*retro_deinit_t)(void);
typedef int (*retro_api_version_t)(void);
typedef bool (*retro_load_game_t)(const struct retro_game_info *);
typedef void (*retro_unload_game_t)(void);
typedef void (*retro_run_t)(void);

struct retro_game_info {
    const char *path;
    const void *data;
    size_t size;
    const char *meta;
};

enum {
    RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY = 8,
    RETRO_ENVIRONMENT_SET_PIXEL_FORMAT = 10
};

enum {
    RETRO_PIXEL_FORMAT_XRGB8888 = 1,
    RETRO_PIXEL_FORMAT_RGB565 = 2
};

// Internal state
static void* gCoreHandle = nullptr;
static retro_set_environment_t g_set_environment = nullptr;
static retro_set_video_refresh_t g_set_video = nullptr;
static retro_set_audio_sample_t g_set_audio = nullptr;
static retro_set_audio_sample_batch_t g_set_audio_batch = nullptr;
static retro_set_input_poll_t g_set_poll = nullptr;
static retro_set_input_state_t g_set_input_state = nullptr;
static retro_init_t g_retro_init = nullptr;
static retro_deinit_t g_retro_deinit = nullptr;
static retro_load_game_t g_retro_load_game = nullptr;
static retro_unload_game_t g_retro_unload_game = nullptr;
static retro_run_t g_retro_run = nullptr;
static retro_api_version_t g_retro_api_version = nullptr;

static ANativeWindow* gWindow = nullptr;
static std::mutex gWindowMutex;

static std::atomic<bool> gRunning(false);
static std::thread gEmuThread;
static int gPixelFormat = RETRO_PIXEL_FORMAT_XRGB8888;

static std::mutex gInputLock;
static std::vector<int> gButtons(512, 0);

// Forward callbacks
static bool environment_cb(unsigned cmd, void* data);
static void video_cb(const void* data, unsigned width, unsigned height, size_t pitch);
static void audio_cb(int16_t left, int16_t right);
static size_t audio_batch_cb(const int16_t* data, size_t frames);
static void input_poll_cb(void);
static int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id);

template<typename T>
static bool resolve_sym(void* handle, const char* name, T &out) {
    dlerror();
    void* s = dlsym(handle, name);
    const char* err = dlerror();
    if (err) {
        LOGE("dlsym(%s) error: %s", name, err);
        return false;
    }
    out = reinterpret_cast<T>(s);
    LOGI("Resolved %s", name);
    return true;
}

// Post frame to ANativeWindow
static void post_frame_to_window(const void* data, unsigned width, unsigned height, size_t pitch) {
    std::lock_guard<std::mutex> lk(gWindowMutex);
    if (!gWindow || !data) return;

    // set geometry to native size and RGBA_8888
    ANativeWindow_setBuffersGeometry(gWindow, width, height, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer buf;
    if (ANativeWindow_lock(gWindow, &buf, nullptr) != 0) return;

    if (gPixelFormat == RETRO_PIXEL_FORMAT_XRGB8888) {
        // copy rows (assume 4 bytes/pixel)
        for (unsigned y = 0; y < height; ++y) {
            const void* src = (const uint8_t*)data + y * pitch;
            void* dst = (uint8_t*)buf.bits + y * buf.stride * 4;
            memcpy(dst, src, width * 4);
        }
    } else if (gPixelFormat == RETRO_PIXEL_FORMAT_RGB565) {
        uint16_t* src = (uint16_t*)data;
        uint32_t* dst = (uint32_t*)buf.bits;
        for (unsigned y = 0; y < height; ++y) {
            for (unsigned x = 0; x < width; ++x) {
                uint16_t p = src[y * (pitch/2) + x];
                uint8_t r = ((p >> 11) & 0x1F) << 3;
                uint8_t g = ((p >> 5) & 0x3F) << 2;
                uint8_t b = (p & 0x1F) << 3;
                dst[y * buf.stride + x] = (0xFFu << 24) | (r << 16) | (g << 8) | b;
            }
        }
    } else {
        // fallback copy 32-bit
        for (unsigned y = 0; y < height; ++y) {
            const void* src = (const uint8_t*)data + y * pitch;
            void* dst = (uint8_t*)buf.bits + y * buf.stride * 4;
            memcpy(dst, src, width * 4);
        }
    }

    ANativeWindow_unlockAndPost(gWindow);
}

// libretro callbacks
static bool environment_cb(unsigned cmd, void* data) {
    switch (cmd) {
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
            if (!data) return false;
            {
                // We'll let native_bridge set system dir via setSystemDir; if not set, return null.
                const char** out = (const char**)data;
                *out = nullptr; // default null; native_bridge may implement setSystemDir if needed
                LOGI("env GET_SYSTEM_DIRECTORY -> (null)");
            }
            return true;
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
            if (!data) return false;
            gPixelFormat = *(int*)data;
            LOGI("env SET_PIXEL_FORMAT -> %d", gPixelFormat);
            return true;
        default:
            return false;
    }
}

static void video_cb(const void* data, unsigned width, unsigned height, size_t pitch) {
    post_frame_to_window(data, width, height, pitch);
}

static void audio_cb(int16_t left, int16_t right) {
    (void)left; (void)right; // stub - audio handler not implemented here
}

static size_t audio_batch_cb(const int16_t* data, size_t frames) {
    (void)data; (void)frames;
    return frames;
}

static void input_poll_cb(void) {
    // no-op
}

static int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id) {
    std::lock_guard<std::mutex> lk(gInputLock);
    if (id < gButtons.size()) return gButtons[id] ? 1 : 0;
    return 0;
}

// Emulation thread
static void emu_thread_main() {
    LOGI("Emu thread started");
    while (gRunning.load()) {
        if (g_retro_run) {
            g_retro_run();
        } else {
            break;
        }
    }
    LOGI("Emu thread stopped");
}

// Public API for native_bridge.cpp
extern "C" {

// Load core .so and resolve symbols, register callbacks, call retro_init
bool load_core_internal(const char* path) {
    if (!path) return false;
    if (gCoreHandle) {
        // unload first
        if (g_retro_unload_game) g_retro_unload_game();
        if (g_retro_deinit) g_retro_deinit();
        dlclose(gCoreHandle);
        gCoreHandle = nullptr;
    }

    LOGI("dlopen core: %s", path);
    void* h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        LOGE("dlopen failed: %s", dlerror());
        return false;
    }
    gCoreHandle = h;

    bool ok = true;
    ok &= resolve_sym(h, "retro_api_version", g_retro_api_version);
    ok &= resolve_sym(h, "retro_set_environment", g_set_environment);
    ok &= resolve_sym(h, "retro_set_video_refresh", g_set_video);
    ok &= resolve_sym(h, "retro_set_audio_sample", g_set_audio);
    ok &= resolve_sym(h, "retro_set_audio_sample_batch", g_set_audio_batch);
    ok &= resolve_sym(h, "retro_set_input_poll", g_set_poll);
    ok &= resolve_sym(h, "retro_set_input_state", g_set_input_state);
    ok &= resolve_sym(h, "retro_init", g_retro_init);
    ok &= resolve_sym(h, "retro_deinit", g_retro_deinit);
    ok &= resolve_sym(h, "retro_load_game", g_retro_load_game);
    ok &= resolve_sym(h, "retro_unload_game", g_retro_unload_game);
    ok &= resolve_sym(h, "retro_run", g_retro_run);

    if (!ok) {
        LOGE("Failed to resolve required libretro symbols");
        dlclose(h);
        gCoreHandle = nullptr;
        return false;
    }

    // register callbacks
    if (g_set_environment) g_set_environment(environment_cb);
    if (g_set_video) g_set_video(video_cb);
    if (g_set_audio) g_set_audio(audio_cb);
    if (g_set_audio_batch) g_set_audio_batch(audio_batch_cb);
    if (g_set_poll) g_set_poll(input_poll_cb);
    if (g_set_input_state) g_set_input_state(input_state_cb);

    if (g_retro_init) g_retro_init();

    LOGI("Core initialized");
    return true;
}

bool unload_core_internal() {
    // stop emulation thread if running
    if (gRunning.load()) {
        gRunning.store(false);
        if (gEmuThread.joinable()) gEmuThread.join();
    }
    if (g_retro_unload_game) g_retro_unload_game();
    if (g_retro_deinit) g_retro_deinit();
    if (gCoreHandle) {
        dlclose(gCoreHandle);
        gCoreHandle = nullptr;
    }
    return true;
}

bool load_game_internal(const char* rompath) {
    if (!gCoreHandle || !g_retro_load_game) return false;
    retro_game_info gi;
    memset(&gi, 0, sizeof(gi));
    gi.path = rompath;
    gi.data = nullptr;
    gi.size = 0;
    gi.meta = nullptr;
    bool ok = g_retro_load_game(&gi);
    LOGI("retro_load_game -> %d", ok ? 1 : 0);
    return ok;
}

bool start_emulation_internal() {
    if (!gCoreHandle || !g_retro_run) return false;
    if (gRunning.load()) return true;
    gRunning.store(true);
    gEmuThread = std::thread(emu_thread_main);
    return true;
}

void stop_emulation_internal() {
    if (!gRunning.load()) return;
    gRunning.store(false);
    if (gEmuThread.joinable()) gEmuThread.join();
}

void set_window_internal(ANativeWindow* win) {
    std::lock_guard<std::mutex> lk(gWindowMutex);
    if (gWindow) {
        ANativeWindow_release(gWindow);
        gWindow = nullptr;
    }
    gWindow = win; // note: caller must ensure reference (ANativeWindow_fromSurface used)
}

void clear_window_internal() {
    std::lock_guard<std::mutex> lk(gWindowMutex);
    if (gWindow) {
        ANativeWindow_release(gWindow);
        gWindow = nullptr;
    }
}

void set_button_state_internal(int id, int pressed) {
    std::lock_guard<std::mutex> lk(gInputLock);
    if (id >= 0 && (size_t)id < gButtons.size()) gButtons[id] = pressed ? 1 : 0;
}

} // extern "C"