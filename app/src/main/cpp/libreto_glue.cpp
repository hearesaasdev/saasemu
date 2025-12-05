// libretro_glue.cpp
// Executor Libretro completo para Android
// - Carrega core (.so)
// - Lê BIOS por system dir
// - Carrega ROM
// - Conecta vídeo ao SurfaceView
// - Reproduz áudio via AudioTrack (Java)
// - Input básico
// - Emulação em thread separada

#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <dlfcn.h>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <cstring>
#include <mutex>
#include <condition_variable>

#define LOG_TAG "LibretroGlue"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// -----------------------------
// Mínimo de definições libretro
// -----------------------------
typedef void (*retro_set_environment_t)(bool (*)(unsigned, void*));
typedef void (*retro_set_video_refresh_t)(void (*)(const void*, unsigned, unsigned, size_t));
typedef void (*retro_set_audio_sample_t)(void (*)(int16_t, int16_t));
typedef void (*retro_set_audio_sample_batch_t)(size_t (*)(const int16_t*, size_t));
typedef void (*retro_set_input_poll_t)(void (*)(void));
typedef void (*retro_set_input_state_t)(int16_t (*)(unsigned, unsigned, unsigned, unsigned));
typedef void (*retro_init_t)(void);
typedef void (*retro_deinit_t)(void);

typedef struct retro_game_info {
    const char *path;
    const void *data;
    size_t size;
    const char *meta;
} retro_game_info;

typedef bool (*retro_load_game_t)(const struct retro_game_info *);
typedef void (*retro_unload_game_t)(void);
typedef void (*retro_run_t)(void);

typedef bool (*retro_environment_t)(unsigned cmd, void* data);

enum {
    RETRO_ENVIRONMENT_SET_PIXEL_FORMAT = 10,
    RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY = 8
};

enum {
    RETRO_PIXEL_FORMAT_XRGB8888 = 1,
    RETRO_PIXEL_FORMAT_RGB565 = 2
};

// ---------------------------------
// Variáveis globais
// ---------------------------------
static void* gCore = nullptr;

static retro_set_environment_t g_setEnv = nullptr;
static retro_set_video_refresh_t g_setVideo = nullptr;
static retro_set_audio_sample_t g_setAudio = nullptr;
static retro_set_audio_sample_batch_t g_setAudioBatch = nullptr;
static retro_set_input_poll_t g_setPoll = nullptr;
static retro_set_input_state_t g_setInput = nullptr;
static retro_init_t g_init = nullptr;
static retro_deinit_t g_deinit = nullptr;
static retro_load_game_t g_loadGame = nullptr;
static retro_unload_game_t g_unloadGame = nullptr;
static retro_run_t g_run = nullptr;

static std::atomic<bool> gRunning(false);
static std::thread gThread;

static ANativeWindow* gWindow = nullptr;
static int gPixelFormat = RETRO_PIXEL_FORMAT_XRGB8888;

static std::string gSystemDir = "";

static std::mutex gInputMutex;
static std::vector<int> gButtons(512, 0);

static JavaVM* gJvm = nullptr;
static jobject gAudioTrack = nullptr;
static jmethodID gAudioWrite = nullptr;

// ---------------------------------
// Audio → AudioTrack (Java)
// ---------------------------------
static void sendAudio(const int16_t* samples, size_t frames) {
    if (!gJvm || !gAudioTrack || !gAudioWrite) return;

    JNIEnv* env = nullptr;
    gJvm->AttachCurrentThread(&env, nullptr);

    jshortArray arr = env->NewShortArray(frames * 2);
    env->SetShortArrayRegion(arr, 0, frames * 2, samples);

    env->CallVoidMethod(gAudioTrack, gAudioWrite, arr, 0, (jint)(frames * 2));

    env->DeleteLocalRef(arr);
    gJvm->DetachCurrentThread();
}

// ---------------------------------
// Callbacks Libretro
// ---------------------------------
static bool environment_cb(unsigned cmd, void* data) {
    switch (cmd) {

        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
            const char** out = (const char**)data;
            *out = gSystemDir.c_str();
            return true;
        }

        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
            int* fmt = (int*)data;
            gPixelFormat = *fmt;
            return true;
        }

        default:
            return false;
    }
}

static void video_cb(const void* data, unsigned w, unsigned h, size_t pitch) {
    if (!gWindow || !data) return;

    ANativeWindow_setBuffersGeometry(gWindow, w, h, WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer buf;
    if (ANativeWindow_lock(gWindow, &buf, nullptr) != 0) return;

    uint8_t* dst = (uint8_t*)buf.bits;

    if (gPixelFormat == RETRO_PIXEL_FORMAT_XRGB8888) {
        for (unsigned y=0; y<h; y++) {
            memcpy(dst + y * buf.stride * 4, (uint8_t*)data + y * pitch, w * 4);
        }

    } else if (gPixelFormat == RETRO_PIXEL_FORMAT_RGB565) {
        uint16_t* src = (uint16_t*)data;
        uint32_t* out = (uint32_t*)buf.bits;

        for (unsigned y=0; y<h; y++) {
            for (unsigned x=0; x<w; x++) {
                uint16_t p = src[y*(pitch/2)+x];
                uint8_t r = ((p>>11)&0x1F)<<3;
                uint8_t g = ((p>>5)&0x3F)<<2;
                uint8_t b = (p&0x1F)<<3;
                out[y*buf.stride+x] = (0xFF<<24)|(r<<16)|(g<<8)|b;
            }
        }
    }

    ANativeWindow_unlockAndPost(gWindow);
}

static void audio_cb(int16_t l, int16_t r) {
    int16_t s[2] = {l, r};
    sendAudio(s, 1);
}

static size_t audio_batch_cb(const int16_t* data, size_t frames) {
    sendAudio(data, frames);
    return frames;
}

static void poll_cb() {}

static int16_t input_cb(unsigned p, unsigned d, unsigned i, unsigned id) {
    std::lock_guard<std::mutex> lock(gInputMutex);
    return (id < gButtons.size() && gButtons[id]) ? 1 : 0;
}

// ---------------------------------
// Thread de emulação
// ---------------------------------
static void emuLoop() {
    while (gRunning.load()) {
        g_run();
    }
}

// ---------------------------------
// JNI
// ---------------------------------
extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* vm, void* r) {
    gJvm = vm;
    return JNI_VERSION_1_6;
}

// Carregar Core
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_loadCore
(JNIEnv* env, jclass, jstring path) {

    const char* p = env->GetStringUTFChars(path, nullptr);

    if (gCore) {
        dlclose(gCore);
        gCore = nullptr;
    }

    gCore = dlopen(p, RTLD_NOW);
    env->ReleaseStringUTFChars(path, p);

    if (!gCore) {
        LOGE("dlopen core fail");
        return JNI_FALSE;
    }

    auto load = [&](auto& fn, const char* n){
        fn = (decltype(fn))dlsym(gCore, n);
        return fn != nullptr;
    };

    if (!(
        load(g_setEnv, "retro_set_environment") &&
        load(g_setVideo, "retro_set_video_refresh") &&
        load(g_setAudio, "retro_set_audio_sample") &&
        load(g_setAudioBatch, "retro_set_audio_sample_batch") &&
        load(g_setPoll, "retro_set_input_poll") &&
        load(g_setInput, "retro_set_input_state") &&
        load(g_init, "retro_init") &&
        load(g_deinit, "retro_deinit") &&
        load(g_loadGame, "retro_load_game") &&
        load(g_unloadGame, "retro_unload_game") &&
        load(g_run, "retro_run")
    )) {
        LOGE("Missing libretro symbol");
        dlclose(gCore);
        gCore = nullptr;
        return JNI_FALSE;
    }

    g_setEnv(environment_cb);
    g_setVideo(video_cb);
    g_setAudio(audio_cb);
    g_setAudioBatch(audio_batch_cb);
    g_setPoll(poll_cb);
    g_setInput(input_cb);

    g_init();
    return JNI_TRUE;
}

// BIOS path
extern "C"
JNIEXPORT void JNICALL
Java_com_saasemu_app_core_NativeBridge_setSystemDir
(JNIEnv* env, jclass, jstring d) {

    const char* p = env->GetStringUTFChars(d, nullptr);
    gSystemDir = p;
    env->ReleaseStringUTFChars(d, p);
}

// Carregar jogo
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_loadGame
(JNIEnv* env, jclass, jstring rom) {

    const char* p = env->GetStringUTFChars(rom, nullptr);

    retro_game_info info{};
    info.path = p;

    bool ok = g_loadGame(&info);

    env->ReleaseStringUTFChars(rom, p);
    return ok ? JNI_TRUE : JNI_FALSE;
}

// Iniciar
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_startEmulation
(JNIEnv*, jclass) {

    if (gRunning.load()) return JNI_TRUE;

    gRunning.store(true);
    gThread = std::thread(emuLoop);
    return JNI_TRUE;
}

// Parar
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_stopEmulation
(JNIEnv*, jclass) {
    gRunning.store(false);
    if (gThread.joinable()) gThread.join();
    return JNI_TRUE;
}

// Surface
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_attachSurface
(JNIEnv* env, jclass, jobject surface) {

    if (gWindow) {
        ANativeWindow_release(gWindow);
        gWindow = nullptr;
    }

    gWindow = ANativeWindow_fromSurface(env, surface);
    return gWindow != nullptr;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_saasemu_app_core_NativeBridge_detachSurface
(JNIEnv*, jclass) {
    if (gWindow) {
        ANativeWindow_release(gWindow);
        gWindow = nullptr;
    }
    return JNI_TRUE;
}

// Input
extern "C"
JNIEXPORT void JNICALL
Java_com_saasemu_app_core_NativeBridge_setButtonState
(JNIEnv*, jclass, jint id, jint p) {
    if (id >= 0 && id < gButtons.size())
        gButtons[id] = p ? 1 : 0;
}