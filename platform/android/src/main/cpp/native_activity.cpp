// EXWORLD Android NativeActivity (canonical)
#include "exworld/game.hpp"

#include "exgine/android.hpp"
#include "exgine/android_audio.hpp"
#include "exgine/mobile.hpp"
#include "exgine/render.hpp"

#include <android/asset_manager.h>
#include <android/input.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <memory>
#include <string>

namespace {

constexpr const char* kTag = "EXWORLD";
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, kTag, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, kTag, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, kTag, __VA_ARGS__)

std::uint64_t monotonic_time_ns() {
    timespec v{};
    if (clock_gettime(CLOCK_MONOTONIC, &v) != 0) return 0;
    return static_cast<std::uint64_t>(v.tv_sec) * 1000000000ull +
           static_cast<std::uint64_t>(v.tv_nsec);
}

bool read_asset(AAssetManager* m, std::string_view path, std::string& out) {
    if (!m) return false;
    AAsset* a = AAssetManager_open(m, std::string(path).c_str(), AASSET_MODE_BUFFER);
    if (!a) return false;
    const auto n = static_cast<std::size_t>(AAsset_getLength64(a));
    out.resize(n);
    const auto got = AAsset_read(a, out.data(), n);
    AAsset_close(a);
    return got >= 0 && static_cast<std::size_t>(got) == n;
}

bool load_asset(AAssetManager* m, std::string_view path, std::string& out) {
    if (read_asset(m, path, out)) return true;
    std::string a = "content/"; a += path;
    if (read_asset(m, a, out)) return true;
    std::string b = "first_light/"; b += path;
    return read_asset(m, b, out);
}

struct AppState {
    exgine::AndroidEglPresenter presenter;
    exgine::MobileRuntimeBridge mobile;
    exgine::AndroidAudioBackend audio;
    std::unique_ptr<exworld::ExWorldGame> game;
    AAssetManager* assets = nullptr;
    int width = 1280;
    int height = 720;
    double previous_time = 0;
    bool started = false;
    bool boot_ok = false;
    std::string boot_error;
};

void feed_touch(AppState* s, const AInputEvent* e, std::size_t i, bool down) {
    if (!s || !s->game) return;
    const float x = AMotionEvent_getX(e, i);
    const float y = AMotionEvent_getY(e, i);
    const float nx = s->width > 0 ? x / static_cast<float>(s->width) : 0.f;
    const float ny = s->height > 0 ? y / static_cast<float>(s->height) : 0.f;
    s->game->input().set_touch(AMotionEvent_getPointerId(e, i), down, nx, ny);
}

int32_t handle_input(android_app* app, AInputEvent* input) {
    auto* s = static_cast<AppState*>(app->userData);
    if (!s || !input || !s->game) return 0;
    if (AInputEvent_getType(input) == AINPUT_EVENT_TYPE_KEY) {
        const int action = AKeyEvent_getAction(input);
        if (action != AKEY_EVENT_ACTION_DOWN && action != AKEY_EVENT_ACTION_UP) return 0;
        const int code = AKeyEvent_getKeyCode(input);
        const bool down = action == AKEY_EVENT_ACTION_DOWN;
        int mapped = 0;
        switch (code) {
        case AKEYCODE_W: mapped = 'W'; break;
        case AKEYCODE_A: mapped = 'A'; break;
        case AKEYCODE_S: mapped = 'S'; break;
        case AKEYCODE_D: mapped = 'D'; break;
        case AKEYCODE_E: mapped = 'E'; break;
        case AKEYCODE_F: mapped = 'F'; break;
        case AKEYCODE_SPACE: mapped = 32; break;
        case AKEYCODE_SHIFT_LEFT:
        case AKEYCODE_SHIFT_RIGHT: mapped = 16; break;
        case AKEYCODE_BUTTON_A: s->game->input().set_gamepad_button(0, down); return 1;
        case AKEYCODE_BUTTON_B: s->game->input().set_gamepad_button(1, down); return 1;
        default: break;
        }
        if (mapped) { s->game->input().set_key(mapped, down); return 1; }
        return 0;
    }
    if (AInputEvent_getType(input) != AINPUT_EVENT_TYPE_MOTION) return 0;
    if ((AInputEvent_getSource(input) & AINPUT_SOURCE_CLASS_POINTER) == 0) return 0;
    const int32_t action = AMotionEvent_getAction(input);
    const int32_t type = action & AMOTION_EVENT_ACTION_MASK;
    const std::size_t idx = static_cast<std::size_t>(
        (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    const std::size_t count = AMotionEvent_getPointerCount(input);
    if (type == AMOTION_EVENT_ACTION_DOWN || type == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        if (idx < count) feed_touch(s, input, idx, true); return 1;
    }
    if (type == AMOTION_EVENT_ACTION_UP || type == AMOTION_EVENT_ACTION_POINTER_UP) {
        if (idx < count) feed_touch(s, input, idx, false); return 1;
    }
    if (type == AMOTION_EVENT_ACTION_MOVE) {
        for (std::size_t i = 0; i < count; ++i) feed_touch(s, input, i, true);
        return 1;
    }
    return 0;
}

void handle_cmd(android_app* app, int32_t cmd) {
    auto* s = static_cast<AppState*>(app->userData);
    if (!s) return;
    switch (cmd) {
    case APP_CMD_START: s->mobile.on_start(); break;
    case APP_CMD_RESUME: s->mobile.on_resume(); break;
    case APP_CMD_PAUSE: s->mobile.on_pause(); break;
    case APP_CMD_STOP:
        s->mobile.on_stop(); s->audio.stop(); s->started = false; break;
    case APP_CMD_INIT_WINDOW:
        if (app->window && s->presenter.attach(app->window)) {
            s->mobile.on_surface_available();
            s->width = ANativeWindow_getWidth(app->window);
            s->height = ANativeWindow_getHeight(app->window);
        }
        break;
    case APP_CMD_TERM_WINDOW:
        s->mobile.on_surface_lost(); s->presenter.detach(); break;
    case APP_CMD_WINDOW_RESIZED:
    case APP_CMD_CONTENT_RECT_CHANGED:
        (void)s->presenter.resize();
        if (app->window) {
            s->width = ANativeWindow_getWidth(app->window);
            s->height = ANativeWindow_getHeight(app->window);
        }
        break;
    default: break;
    }
}

bool boot_game(AppState& state) {
    auto loader = [&state](std::string_view path, std::string& out) -> bool {
        std::string p(path);
        if (p.rfind("./", 0) == 0) p = p.substr(2);
        if (p.rfind("content/", 0) == 0) p = p.substr(8);
        return load_asset(state.assets, p, out) || load_asset(state.assets, path, out);
    };
    state.game = std::make_unique<exworld::ExWorldGame>(loader);
    std::string manifest;
    if (!load_asset(state.assets, "project.exg", manifest) &&
        !load_asset(state.assets, "first_light/project.exg", manifest)) {
        state.boot_error = "no project.exg in APK assets";
        LOGE("%s", state.boot_error.c_str());
        return false;
    }
    if (!state.game->open_from_manifest(manifest)) {
        state.boot_error = "open_from_manifest failed";
        LOGE("%s", state.boot_error.c_str());
        return false;
    }
    LOGI("boot OK");
    return true;
}

} // namespace

void android_main(android_app* app) {
    LOGI("android_main enter");
    AppState state;
    state.assets = app->activity ? app->activity->assetManager : nullptr;
    state.boot_ok = boot_game(state);
    if (!state.boot_ok)
        LOGE("Boot failed: %s — keeping alive", state.boot_error.c_str());
    if (!state.audio.start()) LOGW("AAudio unavailable");
    app->userData = &state;
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;
    for (;;) {
        int ident = 0, events = 0;
        android_poll_source* source = nullptr;
        while ((ident = ALooper_pollOnce(state.mobile.renderable() ? 0 : -1, nullptr, &events,
                                         reinterpret_cast<void**>(&source))) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                state.mobile.on_destroy();
                state.mobile.on_surface_lost();
                state.audio.stop();
                state.presenter.detach();
                return;
            }
        }
        if (!state.boot_ok) continue;
        if (!state.mobile.renderable() || !state.presenter.ready()) continue;
        const double now = static_cast<double>(monotonic_time_ns()) / 1e9;
        const double dt = state.previous_time > 0
                              ? std::clamp(now - state.previous_time, 0.0, 0.05) : 1.0 / 60.0;
        state.previous_time = now;
        if (state.mobile.begin_frame(now).state == exgine::MobileFrameState::Paused) continue;
        if (!state.started) {
            state.started = state.game->start();
            if (!state.started) { LOGE("start failed"); continue; }
            LOGI("game started");
        }
        if (!state.game->update(dt)) continue;
        (void)state.game->present(state.presenter, state.width, state.height);
    }
}
