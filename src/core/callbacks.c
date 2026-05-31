#include "callbacks.h"
#include "utils/general.h"
#include "utils/glutil.h"
#include "utils/eglutil.h"

static const WaylandOutputListener output_listener = {
    .geometry = OutputGeometry,
    .mode = OutputMode,
    .scale = OutputScale,
    .done = OutputDone,
};

void LayerSurfaceConfigure(void *data, WaylandLayerSurface *surface, uint32_t serial, uint32_t width, uint32_t height) {
    Application *app = data;
    zwlr_layer_surface_v1_ack_configure(surface, serial);
    if ((int32_t)width != app->width || (int32_t)height != app->height) {
        app->width  = width;
        app->height = height;
        if (app->egl_window) wl_egl_window_resize(app->egl_window, width, height, 0, 0);
    }
    if (!app->configured) {
        app->configured = true;
        if (!CreateEGLSurface(app)) {
            fprintf(stderr, "Failed to create EGL surface\n");
            app->running = false;
            return;
        }
        char *shader = ReadFile(app->shader_path);
        if (!shader) {
            fprintf(stderr, "Cannot read shader: %s\n", app->shader_path);
            app->running = false;
            return;
        }
        if (!BuildProgram(app, shader)) {
            fprintf(stderr, "Failed to build shader program\n");
            free(shader);
            app->running = false;
            return;
        }
        free(shader);
        InitGeometry(app);
        clock_gettime(CLOCK_MONOTONIC, &app->start_time);
        app->last_frame = app->start_time;
        fprintf(stderr, "Shader loaded. Rendering %dx%d wallpaper...\n", app->width, app->height);
    }

    wl_surface_commit(app->surface);
}

void LayerSurfaceClosed(void *data, WaylandLayerSurface *surface) {
    Application *app = data;
    fprintf(stderr, "Layer surface closed\n");
    app->running = false;
}

void OutputGeometry(void *data, WaylandOutput *wl_output, int32_t x, int32_t y, int32_t physical_w, int32_t physical_h, int32_t subpixel, const char *make, const char *model, int32_t transform) {}

void OutputMode(void *data, WaylandOutput *wl_output, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {}

void OutputScale(void *data, WaylandOutput *wl_output, int32_t factor) {
    Output *out = data;
    out->scale = factor;
}

void OutputDone(void *data, WaylandOutput *wl_output) {}

void XDGOutputName(void *data, XDGOutput *xdg_output, const char *name) {
    Output *out = data;
    strncpy(out->name, name, sizeof(out->name) - 1);
}

void XDGOutputDescription(void *data, XDGOutput *xdg_output, const char *description) {}

void XDGOutputLogicalPosition(void *data, XDGOutput *xdg_output, int32_t x, int32_t y) {
    Output *out = data;
    out->x = x; out->y = y;
}

void XDGOutputLogicalSize(void *data, XDGOutput *xdg_output, int32_t width, int32_t height) {
    Output *out = data;
    out->width = width; out->height = height;
}

void XDGOutputDone(void *data, XDGOutput *xdg_output) {}

void RegistryGlobal(void *data, WaylandRegistry *registry, uint32_t name, const char *interface, uint32_t version) {
    Application *app = data;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        app->compositor = wl_registry_bind(registry, name,
            &wl_compositor_interface, 4);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        app->layer_shell = wl_registry_bind(registry, name,
            &zwlr_layer_shell_v1_interface, 1);
    } else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
        app->xdg_output_manager = wl_registry_bind(registry, name,
            &zxdg_output_manager_v1_interface, 2);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        Output *out = calloc(1, sizeof(*out));
        out->scale = 1;
        out->wl_output = wl_registry_bind(registry, name,
            &wl_output_interface, 3);
        wl_output_add_listener(out->wl_output, &output_listener, out);
        wl_list_insert(&app->outputs, &out->link);
    }
}

void RegistryGlobalRemove(void *data, WaylandRegistry *registry, uint32_t name) {}
