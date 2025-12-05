// native_bridge.cpp
// JNI bridge that connects Android / Kotlin to libretro loader functions.

#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <string>

#define LOG_TAG "SaaSEmuNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// forward declarations of loader functions (defined in libretro_loader.cpp)
extern "C" {
    bool load_core_internal(const char* path);
    bool unload_core_internal();
    bool load_game_internal(const char* rompath);
    bool start_emulation_internal();
    void stop_emulation_internal();
    void set_window_internal(ANativeWindow* win);
    void clear_window_internal();
    void set_button_state_internal(int id, int pressed);
}

// Cache JavaVM for potential future use
static JavaVM* gJvm = nullptr;

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    (void)reserved;
    gJvm = vm;
    LOGI("JNI_OnLoad");
    return JNI_VERSION_1_6;
}

// initNative(datapath) - optional: we keep for compatibility
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_initNative(JNIEnv* env, jobject /*clazz*/, jstring datapath) {
    const char* p = env->GetStringUTFChars(datapath, nullptr);
    if (p) {
        LOGI("initNative datapath: %s", p);
        env->ReleaseStringUTFChars(datapath, p);
    }
    return JNI_TRUE;
}

// attachSurface(surface)
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_attachSurface(JNIEnv* env, jobject /*clazz*/, jobject surface) {
    if (!surface) {
        LOGE("attachSurface: null");
        return JNI_FALSE;
    }
    ANativeWindow* win = ANativeWindow_fromSurface(env, surface);
    if (!win) {
        LOGE("ANativeWindow_fromSurface failed");
        return JNI_FALSE;
    }
    set_window_internal(win);
    LOGI("Surface attached");
    return JNI_TRUE;
}

// detachSurface()
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_detachSurface(JNIEnv* env, jobject /*clazz*/) {
    // stop emulation first to avoid callbacks using the window
    stop_emulation_internal();
    clear_window_internal();
    LOGI("Surface detached");
    return JNI_TRUE;
}

// loadCore(corePath)
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_loadCore(JNIEnv* env, jobject /*clazz*/, jstring corePath) {
    const char* p = env->GetStringUTFChars(corePath, nullptr);
    if (!p) {
        LOGE("loadCore: null path");
        return JNI_FALSE;
    }
    bool ok = load_core_internal(p);
    env->ReleaseStringUTFChars(corePath, p);
    return ok ? JNI_TRUE : JNI_FALSE;
}

// loadGame(romPath)
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_loadGame(JNIEnv* env, jobject /*clazz*/, jstring romPath) {
    const char* p = env->GetStringUTFChars(romPath, nullptr);
    if (!p) {
        LOGE("loadGame: null path");
        return JNI_FALSE;
    }
    bool ok = load_game_internal(p);
    env->ReleaseStringUTFChars(romPath, p);
    return ok ? JNI_TRUE : JNI_FALSE;
}

// startEmulation()
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_startEmulation(JNIEnv* env, jobject /*clazz*/) {
    bool ok = start_emulation_internal();
    return ok ? JNI_TRUE : JNI_FALSE;
}

// stopEmulation()
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_stopEmulation(JNIEnv* env, jobject /*clazz*/) {
    stop_emulation_internal();
    return JNI_TRUE;
}

// unloadCore()
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_unloadCore(JNIEnv* env, jobject /*clazz*/) {
    bool ok = unload_core_internal();
    return ok ? JNI_TRUE : JNI_FALSE;
}

// setButtonState(id, pressed)
extern "C" JNIEXPORT void JNICALL
Java_com_saasemu_app_core_NativeBridge_setButtonState(JNIEnv* env, jobject /*clazz*/, jint id, jint pressed) {
    set_button_state_internal((int)id, (int)pressed);
}

// setFastForward (stub)
extern "C" JNIEXPORT void JNICALL
Java_com_saasemu_app_core_NativeBridge_setFastForward(JNIEnv* env, jobject /*clazz*/, jboolean enabled) {
    (void)enabled;
    LOGI("setFastForward called (stub)");
}

// rewindFrames (stub)
extern "C" JNIEXPORT void JNICALL
Java_com_saasemu_app_core_NativeBridge_rewindFrames(JNIEnv* env, jobject /*clazz*/, jint frames) {
    (void)frames;
    LOGI("rewindFrames called (stub)");
}

// setSystemDir (optional helper)
extern "C" JNIEXPORT void JNICALL
Java_com_saasemu_app_core_NativeBridge_setSystemDir(JNIEnv* env, jobject /*clazz*/, jstring dir) {
    const char* p = env->GetStringUTFChars(dir, nullptr);
    if (p) {
        LOGI("setSystemDir: %s", p);
        env->ReleaseStringUTFChars(dir, p);
    }
}  void* sym = dlsym(handle, name);
    const char* err = dlerror();
    if (err) {
        LOGE("dlsym(%s) error: %s", name, err);
        return false;
    }
    out = reinterpret_cast<T>(sym);
    LOGI("Resolved %s", name);
    return true;
}

// ---------------------------
// Libretro callbacks
// ---------------------------
static bool environment_cb(unsigned cmd, void* data) {
    switch (cmd) {
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
            if (!data) return false;
            {
                const char** out = (const char**)data;
                if (gSystemDir.empty()) {
                    *out = nullptr;
                    LOGI("env GET_SYSTEM_DIRECTORY -> (null)");
                } else {
                    *out = gSystemDir.c_str();
                    LOGI("env GET_SYSTEM_DIRECTORY -> %s", gSystemDir.c_str());
                }
                return true;
            }
            break;
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
            if (!data) return false;
            {
                int fmt = *(int*)data;
                gPixelFormat = fmt;
                LOGI("env SET_PIXEL_FORMAT -> %d", fmt);
                return true;
            }
            break;
        default:
            // Not handled
            return false;
    }
}

static void retro_video_cb(const void* data, unsigned width, unsigned height, size_t pitch) {
    if (!gWindow || !data) return;

    ANativeWindow_setBuffersGeometry(gWindow, width, height, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer buf;
    if (ANativeWindow_lock(gWindow, &buf, nullptr) != 0) return;

    // handle known pixel formats
    if (gPixelFormat == RETRO_PIXEL_FORMAT_XRGB8888) {
        // copy rows (assume 4 bytes per pixel)
        for (unsigned y = 0; y < height; y++) {
            const void* src = (const uint8_t*)data + y * pitch;
            void* dst = (uint8_t*)buf.bits + y * buf.stride * 4;
            memcpy(dst, src, width * 4);
        }
    } else if (gPixelFormat == RETRO_PIXEL_FORMAT_RGB565) {
        // convert RGB565 -> RGBA_8888
        uint16_t* src = (uint16_t*)data;
        uint32_t* dst = (uint32_t*)buf.bits;
        for (unsigned y = 0; y < height; y++) {
            for (unsigned x = 0; x < width; x++) {
                uint16_t p = src[y * (pitch/2) + x];
                uint8_t r = ((p >> 11) & 0x1F) << 3;
                uint8_t g = ((p >> 5) & 0x3F) << 2;
                uint8_t b = (p & 0x1F) << 3;
                dst[y * buf.stride + x] = (0xFFu << 24) | (r << 16) | (g << 8) | b;
            }
        }
    } else {
        // fallback copy as 32-bit
        for (unsigned y = 0; y < height; y++) {
            const void* src = (const uint8_t*)data + y * pitch;
            void* dst = (uint8_t*)buf.bits + y * buf.stride * 4;
            memcpy(dst, src, width * 4);
        }
    }

    ANativeWindow_unlockAndPost(gWindow);
}

static void retro_audio_cb(int16_t left, int16_t right) {
    // stub: log or later forward to AudioTrack via JNI
    (void)left; (void)right;
    // For performance don't log every sample; consider batching in audio_batch callback.
}

static size_t retro_audio_batch_cb(const int16_t* data, size_t frames) {
    // stub: we don't output audio here. If you want, implement AudioTrack write via JNI.
    (void)data; (void)frames;
    return frames;
}

static void retro_input_poll_cb(void) {
    // no-op; input_state will be called to retrieve individual buttons
}

static int16_t retro_input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id) {
    std::lock_guard<std::mutex> lk(gInputLock);
    if (id < gButtonStates.size()) return gButtonStates[id] ? 1 : 0;
    return 0;
}

// ---------------------------
// Emulation thread loop
// ---------------------------
static void emu_loop() {
    LOGI("Emu thread started");
    while (gEmulating.load()) {
        if (g_retro_run) {
            g_retro_run();
        } else {
            // no run symbol, break
            break;
        }
        // allow scheduler breathing
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    LOGI("Emu thread exiting");
}

// ---------------------------
// JNI lifecycle
// ---------------------------

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    (void)reserved;
    gJvm = vm;
    LOGI("JNI_OnLoad");
    return JNI_VERSION_1_6;
}

// initNative(datapath) - we use datapath as system dir if provided
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_initNative(JNIEnv* env, jobject /*clazz*/, jstring datapath) {
    const char* p = env->GetStringUTFChars(datapath, nullptr);
    if (p) {
        gSystemDir = std::string(p);
        LOGI("initNative: systemDir set to %s", gSystemDir.c_str());
        env->ReleaseStringUTFChars(datapath, p);
    } else {
        LOGI("initNative: datapath null");
    }
    return JNI_TRUE;
}

// attachSurface(surface)
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_attachSurface(JNIEnv* env, jobject /*clazz*/, jobject surface) {
    if (gWindow) {
        ANativeWindow_release(gWindow);
        gWindow = nullptr;
    }
    if (surface == nullptr) {
        LOGE("attachSurface: surface == null");
        return JNI_FALSE;
    }
    gWindow = ANativeWindow_fromSurface(env, surface);
    if (!gWindow) {
        LOGE("ANativeWindow_fromSurface failed");
        return JNI_FALSE;
    }
    LOGI("Surface attached");
    return JNI_TRUE;
}

// detachSurface()
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_detachSurface(JNIEnv* env, jobject /*clazz*/) {
    // stop emulation first if running so callbacks stop using window
    if (gEmulating.load()) {
        gEmulating.store(false);
        if (gEmuThread.joinable()) gEmuThread.join();
    }
    if (gWindow) {
        ANativeWindow_release(gWindow);
        gWindow = nullptr;
    }
    LOGI("Surface detached");
    return JNI_TRUE;
}

// loadCore(corePath) - dlopen and resolve symbols, call retro_init
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_loadCore(JNIEnv* env, jobject /*clazz*/, jstring corePath) {
    const char* path = env->GetStringUTFChars(corePath, nullptr);
    if (!path) {
        LOGE("loadCore: path null");
        return JNI_FALSE;
    }

    // If already loaded, unload first
    if (gCoreHandle) {
        if (g_retro_unload_game) g_retro_unload_game();
        if (g_retro_deinit) g_retro_deinit();
        dlclose(gCoreHandle);
        gCoreHandle = nullptr;
    }

    LOGI("Attempt dlopen: %s", path);
    void* h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    env->ReleaseStringUTFChars(corePath, path);

    if (!h) {
        LOGE("dlopen failed: %s", dlerror());
        return JNI_FALSE;
    }
    gCoreHandle = h;

    // Resolve the commonly required symbols
    bool ok = true;
    ok &= resolve_sym<void*>(h, "retro_api_version", (void*&)g_retro_api_version);
    ok &= resolve_sym<void*>(h, "retro_set_environment", (void*&)g_retro_set_environment);
    ok &= resolve_sym<void*>(h, "retro_set_video_refresh", (void*&)g_retro_set_video_refresh);
    ok &= resolve_sym<void*>(h, "retro_set_audio_sample", (void*&)g_retro_set_audio_sample);
    ok &= resolve_sym<void*>(h, "retro_set_audio_sample_batch", (void*&)g_retro_set_audio_sample_batch);
    ok &= resolve_sym<void*>(h, "retro_set_input_poll", (void*&)g_retro_set_input_poll);
    ok &= resolve_sym<void*>(h, "retro_set_input_state", (void*&)g_retro_set_input_state);
    ok &= resolve_sym<void*>(h, "retro_init", (void*&)g_retro_init);
    ok &= resolve_sym<void*>(h, "retro_deinit", (void*&)g_retro_deinit);
    ok &= resolve_sym<void*>(h, "retro_load_game", (void*&)g_retro_load_game);
    ok &= resolve_sym<void*>(h, "retro_unload_game", (void*&)g_retro_unload_game);
    ok &= resolve_sym<void*>(h, "retro_run", (void*&)g_retro_run);

    if (!ok) {
        LOGE("Failed to resolve required libretro symbols");
        dlclose(h);
        gCoreHandle = nullptr;
        return JNI_FALSE;
    }

    // Wire callbacks
    if (g_retro_set_environment) g_retro_set_environment(environment_cb);
    if (g_retro_set_video_refresh) g_retro_set_video_refresh(retro_video_cb);
    if (g_retro_set_audio_sample) g_retro_set_audio_sample(retro_audio_cb);
    if (g_retro_set_audio_sample_batch) g_retro_set_audio_sample_batch(retro_audio_batch_cb);
    if (g_retro_set_input_poll) g_retro_set_input_poll(retro_input_poll_cb);
    if (g_retro_set_input_state) g_retro_set_input_state(retro_input_state_cb);

    // Initialize core
    if (g_retro_init) g_retro_init();

    LOGI("Core loaded and initialized");
    return JNI_TRUE;
}

// loadGame(romPath) - uses retro_game_info.path (many cores accept)
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_loadGame(JNIEnv* env, jobject /*clazz*/, jstring romPath) {
    if (!gCoreHandle || !g_retro_load_game) {
        LOGE("loadGame: core not loaded or retro_load_game missing");
        return JNI_FALSE;
    }
    const char* rpath = env->GetStringUTFChars(romPath, nullptr);
    if (!rpath) {
        LOGE("loadGame: romPath null");
        return JNI_FALSE;
    }
    retro_game_info gi;
    memset(&gi, 0, sizeof(gi));
    gi.path = rpath;
    gi.data = nullptr;
    gi.size = 0;
    gi.meta = nullptr;

    bool ok = g_retro_load_game(&gi);
    env->ReleaseStringUTFChars(romPath, rpath);
    LOGI("retro_load_game returned %d", ok ? 1 : 0);
    return ok ? JNI_TRUE : JNI_FALSE;
}

// startEmulation()
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_startEmulation(JNIEnv* env, jobject /*clazz*/) {
    if (!gCoreHandle || !g_retro_run) {
        LOGE("startEmulation: core not loaded or retro_run missing");
        return JNI_FALSE;
    }
    if (gEmulating.load()) return JNI_TRUE;
    gEmulating.store(true);
    gEmuThread = std::thread(emu_loop);
    LOGI("Emulation started");
    return JNI_TRUE;
}

// stopEmulation()
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_stopEmulation(JNIEnv* env, jobject /*clazz*/) {
    if (!gEmulating.load()) return JNI_TRUE;
    gEmulating.store(false);
    if (gEmuThread.joinable()) gEmuThread.join();
    LOGI("Emulation stopped");
    return JNI_TRUE;
}

// unload core and cleanup
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_unloadCore(JNIEnv* env, jobject /*clazz*/) {
    // stop emulation
    if (gEmulating.load()) {
        gEmulating.store(false);
        if (gEmuThread.joinable()) gEmuThread.join();
    }
    if (g_retro_unload_game) g_retro_unload_game();
    if (g_retro_deinit) g_retro_deinit();
    if (gCoreHandle) {
        dlclose(gCoreHandle);
        gCoreHandle = nullptr;
    }
    LOGI("Core unloaded");
    return JNI_TRUE;
}

// setFastForward(enabled) - stub (core-specific speed handling can be optimized)
extern "C" JNIEXPORT void JNICALL
Java_com_saasemu_app_core_NativeBridge_setFastForward(JNIEnv* env, jobject /*clazz*/, jboolean enabled) {
    (void)env;
    (void)enabled;
    LOGI("setFastForward called (%d) - not implemented natively", enabled ? 1 : 0);
}

// rewindFrames(frames) - stub: requires savestate stack implementation
extern "C" JNIEXPORT void JNICALL
Java_com_saasemu_app_core_NativeBridge_rewindFrames(JNIEnv* env, jobject /*clazz*/, jint frames) {
    (void)env;
    (void)frames;
    LOGI("rewindFrames called (%d) - not implemented (needs savestate system)", frames);
}

// setButtonState(id, pressed) - helper to let Kotlin toggle inputs
extern "C" JNIEXPORT void JNICALL
Java_com_saasemu_app_core_NativeBridge_setButtonState(JNIEnv* env, jobject /*clazz*/, jint id, jint pressed) {
    std::lock_guard<std::mutex> lk(gInputLock);
    if (id >= 0 && (size_t)id < gButtonStates.size()) {
        gButtonStates[id] = pressed ? 1 : 0;
    }
}

// Optional: setSystemDir from Java (if you prefer to set system dir separately)
extern "C" JNIEXPORT void JNICALL
Java_com_saasemu_app_core_NativeBridge_setSystemDir(JNIEnv* env, jobject /*clazz*/, jstring dir) {
    const char* d = env->GetStringUTFChars(dir, nullptr);
    if (d) {
        gSystemDir = std::string(d);
        env->ReleaseStringUTFChars(dir, d);
        LOGI("setSystemDir -> %s", gSystemDir.c_str());
    }
}

// Keep a simple dlopen-only loader for compatibility
extern "C" JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_loadCoreCheck(JNIEnv* env, jobject /*clazz*/, jstring corePath) {
    // legacy helper (simply dlopen and close to check viability)
    const char* p = env->GetStringUTFChars(corePath, nullptr);
    void* h = dlopen(p, RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        LOGE("dlopen failed: %s", dlerror());
        env->ReleaseStringUTFChars(corePath, p);
        return JNI_FALSE;
    }
    LOGI("dlopen success: %s", p);
    dlclose(h);
    env->ReleaseStringUTFChars(corePath, p);
    return JNI_TRUE;
}