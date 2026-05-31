#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <poll.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "xdg-output-unstable-v1-client-protocol.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

typedef struct wl_output WaylandOutput;
typedef struct wl_output_listener WaylandOutputListener;
typedef struct zxdg_output_v1 XDGOutput;
typedef struct zxdg_output_v1_listener XDGOutputListener;
typedef struct wl_list WaylandList;
typedef struct wl_display WaylandDisplay;
typedef struct wl_registry WaylandRegistry;
typedef struct wl_registry_listener WaylandRegistryListener;
typedef struct wl_compositor WaylandCompositor;
typedef struct zwlr_layer_shell_v1 WaylandLayerShell;
typedef struct zxdg_output_manager_v1 XDGOutputManager;
typedef struct wl_surface WaylandSurface;
typedef struct zwlr_layer_surface_v1 WaylandLayerSurface;
typedef struct zwlr_layer_surface_v1_listener WaylandLayerSurfaceListener;
typedef struct wl_egl_window EGLWindow;
typedef struct timespec TimeSpec;

typedef struct {
    WaylandOutput *wl_output;
    XDGOutput *xdg_output;
    char name[64];
    int32_t x, y, width, height, scale;
    WaylandList link;
} Output;

typedef struct {
    // wayland globals
    WaylandDisplay* display;
    WaylandRegistry* registry;
    WaylandCompositor* compositor;
    WaylandLayerShell* layer_shell;
    XDGOutputManager* xdg_output_manager;
    WaylandList outputs;

    // surface
    WaylandSurface* surface;
    WaylandLayerSurface* layer_surface;
    EGLWindow* egl_window;
    int32_t width, height;
    bool configured;

    // egl
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
    EGLConfig egl_config;

    // gl
    GLuint prog;
    GLuint vbo;

    // uniforms
    GLint u_resolution;
    GLint u_time;
    GLint u_timedelta;
    GLint u_frame;
    GLint u_mouse;
    GLint u_date;
    GLint u_samplerate;

    // timing
    TimeSpec start_time;
    TimeSpec last_frame;
    int frame_count;

    // config
    const char* shader_path;
    const char* target_output; // NULL = all
    int target_fps; // frame cap, default 30
    bool running;
} Application;

#endif
