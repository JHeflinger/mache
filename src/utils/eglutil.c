#include "eglutil.h"

bool InitEGL(Application* app) {
    app->egl_display = eglGetDisplay((EGLNativeDisplayType)app->display);
    if (app->egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "eglGetDisplay failed\n");
        return false;
    }
    EGLint major, minor;
    if (!eglInitialize(app->egl_display, &major, &minor)) {
        fprintf(stderr, "eglInitialize failed\n");
        return false;
    }
    fprintf(stderr, "EGL %d.%d\n", major, minor);
    eglBindAPI(EGL_OPENGL_ES_API);
    static const EGLint cfg_attrs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE,
    };
    EGLint n;
    if (!eglChooseConfig(app->egl_display, cfg_attrs, &app->egl_config, 1, &n) || n < 1) {
        fprintf(stderr, "eglChooseConfig failed\n");
        return false;
    }
    static const EGLint ctx_attrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    app->egl_context = eglCreateContext(app->egl_display, app->egl_config, EGL_NO_CONTEXT, ctx_attrs);
    if (app->egl_context == EGL_NO_CONTEXT) {
        fprintf(stderr, "eglCreateContext failed\n");
        return false;
    }
    return true;
}

bool CreateEGLSurface(Application* app) {
    app->egl_window = wl_egl_window_create(app->surface, app->width, app->height);
    if (!app->egl_window) {
        fprintf(stderr, "wl_egl_window_create failed\n");
        return false;
    }
    app->egl_surface = eglCreateWindowSurface(
        app->egl_display, app->egl_config,
        (EGLNativeWindowType)app->egl_window, NULL);
    if (app->egl_surface == EGL_NO_SURFACE) {
        fprintf(stderr, "eglCreateWindowSurface failed\n");
        return false;
    }
    if (!eglMakeCurrent(app->egl_display, app->egl_surface, app->egl_surface, app->egl_context)) {
        fprintf(stderr, "eglMakeCurrent failed\n");
        return false;
    }
    /* Disable vsync — we pace frames ourselves with clock_nanosleep */
    eglSwapInterval(app->egl_display, 0);
    return true;
}
