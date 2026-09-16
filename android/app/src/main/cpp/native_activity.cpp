#include "exgine/android.hpp"
#include "exgine/mobile.hpp"
#include "exworld/game.hpp"

#include <android/asset_manager.h>
#include <android/input.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <memory>
#include <string>
#include <string_view>

namespace {
constexpr const char* kTag = "EXWORLD_FIRST_LIGHT";

std::uint64_t monotonic_time_ns() {
    timespec v{};
    if (clock_gettime(CLOCK_MONOTONIC, &v) != 0) return 0;
    return static_cast<std::uint64_t>(v.tv_sec) * 1000000000ull + static_cast<std::uint64_t>(v.tv_nsec);
}

bool read_asset(AAssetManager* manager, std::string_view path, std::string& out) {
    if (!manager) return false;
    const std::string name(path);
    AAsset* asset = AAssetManager_open(manager, name.c_str(), AASSET_MODE_BUFFER);
    if (!asset) return false;
    const auto size = static_cast<std::size_t>(AAsset_getLength64(asset));
    out.resize(size);
    const auto got = AAsset_read(asset, out.data(), size);
    AAsset_close(asset);
    return got >= 0 && static_cast<std::size_t>(got) == size;
}

bool load_asset(AAssetManager* manager, std::string_view path, std::string& out) {
    if (read_asset(manager, path, out)) return true;
    std::string first_light = "first_light/";
    first_light += path;
    return read_asset(manager, first_light, out);
}

struct AppState {
    exgine::AndroidEglPresenter presenter;
    exgine::MobileRuntimeBridge mobile;
    std::unique_ptr<exworld::ExWorldGame> game;
    AAssetManager* assets = nullptr;
    double previous_time = 0.0;
    bool started = false;
};

exgine::MobileInputEvent touch_event(exgine::MobileInputType type, const AInputEvent* event, std::size_t index) {
    exgine::MobileInputEvent value;
    value.type = type;
    value.timestamp_ns = monotonic_time_ns();
    value.touch.pointer_id = AMotionEvent_getPointerId(event, index);
    value.touch.x = AMotionEvent_getX(event, index);
    value.touch.y = AMotionEvent_getY(event, index);
    value.touch.pressure = AMotionEvent_getPressure(event, index);
    return value;
}

int32_t handle_input(android_app* app, AInputEvent* input) {
    auto* state = static_cast<AppState*>(app->userData);
    if (!state || !input) return 0;

    if (AInputEvent_getType(input) == AINPUT_EVENT_TYPE_KEY) {
        const int action = AKeyEvent_getAction(input);
        if (action != AKEY_EVENT_ACTION_DOWN && action != AKEY_EVENT_ACTION_UP) return 0;
        exgine::MobileInputEvent value;
        value.type = action == AKEY_EVENT_ACTION_DOWN ? exgine::MobileInputType::KeyDown : exgine::MobileInputType::KeyUp;
        value.timestamp_ns = monotonic_time_ns();
        value.key_code = AKeyEvent_getKeyCode(input);
        value.meta_state = static_cast<std::uint32_t>(AKeyEvent_getMetaState(input));
        return state->mobile.push_input(value) ? 1 : 0;
    }

    if (AInputEvent_getType(input) != AINPUT_EVENT_TYPE_MOTION ||
        (AInputEvent_getSource(input) & AINPUT_SOURCE_CLASS_POINTER) == 0) return 0;

    const int32_t action = AMotionEvent_getAction(input);
    const int32_t type = action & AMOTION_EVENT_ACTION_MASK;
    const std::size_t index = static_cast<std::size_t>((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    const std::size_t count = AMotionEvent_getPointerCount(input);

    if (type == AMOTION_EVENT_ACTION_DOWN || type == AMOTION_EVENT_ACTION_POINTER_DOWN ||
        type == AMOTION_EVENT_ACTION_UP || type == AMOTION_EVENT_ACTION_POINTER_UP) {
        if (index >= count) return 0;
        const auto input_type = (type == AMOTION_EVENT_ACTION_UP || type == AMOTION_EVENT_ACTION_POINTER_UP)
            ? exgine::MobileInputType::TouchUp : exgine::MobileInputType::TouchDown;
        return state->mobile.push_input(touch_event(input_type, input, index)) ? 1 : 0;
    }

    if (type == AMOTION_EVENT_ACTION_MOVE || type == AMOTION_EVENT_ACTION_CANCEL) {
        bool accepted = false;
        const auto input_type = type == AMOTION_EVENT_ACTION_MOVE
            ? exgine::MobileInputType::TouchMove : exgine::MobileInputType::TouchCancel;
        for (std::size_t i = 0; i < count; ++i) accepted = state->mobile.push_input(touch_event(input_type, input, i)) || accepted;
        return accepted ? 1 : 0;
    }
    return 0;
}

void handle_cmd(android_app* app, int32_t cmd) {
    auto* state = static_cast<AppState*>(app->userData);
    if (!state) return;
    switch (cmd) {
        case APP_CMD_START: state->mobile.on_start(); break;
        case APP_CMD_RESUME: state->mobile.on_resume(); break;
        case APP_CMD_PAUSE: state->mobile.on_pause(); break;
        case APP_CMD_STOP:
            state->mobile.on_stop();
            if (state->game) state->game->engine().session().game().runtime().stop_audio();
            state->started = false;
            break;
        case APP_CMD_INIT_WINDOW:
            if (app->window && state->presenter.attach(app->window)) state->mobile.on_surface_available();
            break;
        case APP_CMD_TERM_WINDOW:
            state->mobile.on_surface_lost();
            state->presenter.detach();
            break;
        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONTENT_RECT_CHANGED:
            (void)state->presenter.resize();
            break;
        default: break;
    }
}

} // namespace

void android_main(android_app* app) {
    AppState state;
    state.assets = app->activity ? app->activity->assetManager : nullptr;

    state.game = std::make_unique<exworld::ExWorldGame>([assets = state.assets](std::string_view path, std::string& out) {
        return load_asset(assets, path, out);
    });

    if (!state.game->open("first_light")) {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "First Light project failed to open");
        return;
    }
    if (!state.game->start()) {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "First Light game failed to start");
        return;
    }

    auto& runtime = state.game->engine().session().game().runtime();
    if (!runtime.start_audio(48000)) {
        __android_log_print(ANDROID_LOG_WARN, kTag, "EXGINE audio backend unavailable; continuing silently");
    }

    app->userData = &state;
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;

    for (;;) {
        int ident = 0;
        int events = 0;
        android_poll_source* source = nullptr;
        while ((ident = ALooper_pollOnce(state.mobile.renderable() ? 0 : -1, nullptr, &events,
                                         reinterpret_cast<void**>(&source))) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                state.mobile.on_destroy();
                state.mobile.on_surface_lost();
                state.game->engine().session().game().runtime().stop_audio();
                state.presenter.detach();
                return;
            }
        }

        if (!state.mobile.renderable() || !state.presenter.ready()) continue;

        const double now = static_cast<double>(monotonic_time_ns()) / 1000000000.0;
        const double dt = state.previous_time > 0.0 ? std::clamp(now - state.previous_time, 0.0, 0.05) : 1.0 / 60.0;
        state.previous_time = now;
        const auto timing = state.mobile.begin_frame(now);
        if (timing.state == exgine::MobileFrameState::Paused) continue;

        if (!state.started) state.started = state.game->start();
        if (!state.started) continue;

        if (!state.game->update(dt)) continue;

        exgine::RenderFrame frame;
        exgine::RenderResult result;
        if (!state.game->build_frame(frame, result)) continue;
        if (!state.presenter.present(frame)) {
            __android_log_print(ANDROID_LOG_ERROR, kTag, "GPU present failed: %s", state.presenter.last_error().c_str());
        }
    }
}
