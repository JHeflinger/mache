#include "core/application.h"
#include "core/callbacks.h"
#include "utils/general.h"
#include "utils/eglutil.h"

static const WaylandLayerSurfaceListener layer_surface_listener = {
    .configure = LayerSurfaceConfigure,
    .closed = LayerSurfaceClosed,
};

static const XDGOutputListener xdg_output_listener = {
    .name = XDGOutputName,
    .description = XDGOutputDescription,
    .logical_position = XDGOutputLogicalPosition,
    .logical_size = XDGOutputLogicalSize,
    .done = XDGOutputDone,
};

static const WaylandRegistryListener registry_listener = {
    .global = RegistryGlobal,
    .global_remove = RegistryGlobalRemove,
};

static void RenderFrame(Application* app) {
    TimeSpec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (app->target_fps > 0) {
        double frame_budget = 1.0 / app->target_fps;
        double since_last = TimeDifference(&now, &app->last_frame);
        double sleep_sec = frame_budget - since_last;
        if (sleep_sec > 0.0005) {
            TimeSpec ts = {
                .tv_sec = (time_t)sleep_sec,
                .tv_nsec = (long)((sleep_sec - (time_t)sleep_sec) * 1e9),
            };
            clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
        }
        clock_gettime(CLOCK_MONOTONIC, &now);
    }
    double elapsed = TimeDifference(&now, &app->start_time);
    double dt = TimeDifference(&now, &app->last_frame);
    app->last_frame = now;
    app->frame_count++;
    glViewport(0, 0, app->width, app->height);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(app->prog);
    if (app->u_resolution >= 0) glUniform3f(app->u_resolution, (float)app->width, (float)app->height, 1.f);
    if (app->u_time >= 0) glUniform1f(app->u_time, (float)elapsed);
    if (app->u_timedelta >= 0) glUniform1f(app->u_timedelta, (float)dt);
    if (app->u_frame >= 0) glUniform1i(app->u_frame, app->frame_count);
    if (app->u_mouse >= 0) glUniform4f(app->u_mouse, 0.f, 0.f, 0.f, 0.f);
    if (app->u_date >= 0) {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        float secs = (float)(tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec);
        glUniform4f(app->u_date, (float)(tm->tm_year + 1900), (float)(tm->tm_mon), (float)(tm->tm_mday), secs);
    }
    if (app->u_samplerate >= 0) glUniform1f(app->u_samplerate, 44100.f);
    glBindBuffer(GL_ARRAY_BUFFER, app->vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    eglSwapBuffers(app->egl_display, app->egl_surface);
}

static bool SetupLayerSurface(Application *app, WaylandOutput *wl_output) {
    app->surface = wl_compositor_create_surface(app->compositor);
    if (!app->surface) {
        fprintf(stderr, "wl_compositor_create_surface failed\n");
        return false;
    }
    app->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        app->layer_shell,
        app->surface,
        wl_output,
        ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND,
        "hyprshader");
    if (!app->layer_surface) {
        fprintf(stderr, "zwlr_layer_shell_v1_get_layer_surface failed\n");
        return false;
    }
    zwlr_layer_surface_v1_set_anchor(app->layer_surface,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP    |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT   |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(app->layer_surface, -1);
    zwlr_layer_surface_v1_set_size(app->layer_surface, 0, 0);
    zwlr_layer_surface_v1_add_listener(app->layer_surface,
        &layer_surface_listener, app);
    wl_surface_commit(app->surface);
    return true;
}

static void Usage(const char *argv0) {
    fprintf(stderr,
        "Usage: %s [options] <shader.glsl>\n"
        "\n"
        "Options:\n"
        "  -o <output>   Target output/monitor by name (e.g. DP-1, HDMI-A-1)\n"
        "                Default: first available output\n"
        "  -f <fps>      Target frame rate (default: 30, range: 1-300)\n"
        "  -h            This help text\n"
        "\n"
        "Shader format:\n"
        "  Standard Shadertoy fragment shader with mainImage(out vec4, in vec2).\n"
        "  Supported uniforms:\n"
        "    iTime, iResolution, iTimeDelta, iFrame,\n"
        "    iMouse, iDate, iSampleRate\n"
        "\n"
        "Example:\n"
        "  %s -o DP-1 my_shader.glsl\n",
        argv0, argv0);
}

int RunApplication(int argc, char* argv[]) {
    Application app = {0};
    wl_list_init(&app.outputs);
    app.running = true;
    const char *target_output = NULL;
    int opt;
    int target_fps = 30;
    while ((opt = getopt(argc, argv, "o:f:h")) != -1) {
        switch (opt) {
        case 'o': target_output = optarg; break;
        case 'f':
            target_fps = atoi(optarg);
            if (target_fps < 1 || target_fps > 300) {
                fprintf(stderr, "Invalid fps: %s (must be 1-300)\n", optarg);
                return 1;
            }
            break;
        case 'h': Usage(argv[0]); return 0;
        default:  Usage(argv[0]); return 1;
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "Error: no shader file specified\n\n");
        Usage(argv[0]);
        return 1;
    }
    app.shader_path = argv[optind];
    app.target_output = target_output;
    app.target_fps = target_fps;
    if (access(app.shader_path, R_OK) != 0) {
        fprintf(stderr, "Cannot read shader file: %s: %s\n",
                app.shader_path, strerror(errno));
        return 1;
    }
    app.display = wl_display_connect(NULL);
    if (!app.display) {
        fprintf(stderr, "Cannot connect to Wayland display\n");
        return 1;
    }
    app.registry = wl_display_get_registry(app.display);
    wl_registry_add_listener(app.registry, &registry_listener, &app);
    wl_display_roundtrip(app.display);
    if (!app.compositor) {
        fprintf(stderr, "Missing wl_compositor\n"); return 1;
    }
    if (!app.layer_shell) {
        fprintf(stderr, "Missing zwlr_layer_shell_v1 – is wlroots/Hyprland running?\n");
        return 1;
    }
    if (app.xdg_output_manager) {
        Output *out;
        wl_list_for_each(out, &app.outputs, link) {
            out->xdg_output = zxdg_output_manager_v1_get_xdg_output(
                app.xdg_output_manager, out->wl_output);
            zxdg_output_v1_add_listener(out->xdg_output, &xdg_output_listener, out);
        }
        wl_display_roundtrip(app.display);
    }
    wl_display_roundtrip(app.display);
    WaylandOutput *chosen_output = NULL;
    {
        Output *out;
        wl_list_for_each(out, &app.outputs, link) {
            fprintf(stderr, "Found output: '%s' (%dx%d)\n",
                    out->name, out->width, out->height);
            if (!chosen_output) {
                if (!target_output || strcmp(out->name, target_output) == 0) {
                    chosen_output = out->wl_output;
                    app.width  = out->width  ? out->width  : 1920;
                    app.height = out->height ? out->height : 1080;
                    fprintf(stderr, "Using output: '%s'\n", out->name);
                }
            }
        }
    }
    if (!chosen_output) {
        fprintf(stderr, "Output '%s' not found\n", target_output ? target_output : "(any)");
        return 1;
    }
    if (!InitEGL(&app)) return 1;
    if (!SetupLayerSurface(&app, chosen_output)) return 1;
    struct pollfd pfd = {
        .fd = wl_display_get_fd(app.display),
        .events = POLLIN,
    };
    while (app.running) {
        if (wl_display_flush(app.display) < 0 && errno != EAGAIN) break;
        if (app.configured) {
            RenderFrame(&app);
            wl_display_dispatch_pending(app.display);
        } else {
            if (poll(&pfd, 1, -1) < 0) break;
            if (wl_display_dispatch(app.display) < 0) break;
        }
    }
    if (app.prog) glDeleteProgram(app.prog);
    if (app.vbo) glDeleteBuffers(1, &app.vbo);
    if (app.egl_surface != EGL_NO_SURFACE) eglDestroySurface(app.egl_display, app.egl_surface);
    if (app.egl_context != EGL_NO_CONTEXT) eglDestroyContext(app.egl_display, app.egl_context);
    if (app.egl_display != EGL_NO_DISPLAY) eglTerminate(app.egl_display);
    if (app.egl_window) wl_egl_window_destroy(app.egl_window);
    if (app.layer_surface) zwlr_layer_surface_v1_destroy(app.layer_surface);
    if (app.surface) wl_surface_destroy(app.surface);
    wl_display_disconnect(app.display);
    return 0;
}
