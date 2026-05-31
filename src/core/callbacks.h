#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "core/definitions.h"

void LayerSurfaceConfigure(void *data, WaylandLayerSurface *surface, uint32_t serial, uint32_t width, uint32_t height);

void LayerSurfaceClosed(void *data, WaylandLayerSurface *surface);

void OutputGeometry(void *data, WaylandOutput *wl_output, int32_t x, int32_t y, int32_t physical_w, int32_t physical_h, int32_t subpixel, const char *make, const char *model, int32_t transform);

void OutputMode(void *data, WaylandOutput *wl_output, uint32_t flags, int32_t width, int32_t height, int32_t refresh);

void OutputScale(void *data, WaylandOutput *wl_output, int32_t factor);

void OutputDone(void *data, WaylandOutput *wl_output);

void XDGOutputName(void *data, XDGOutput *xdg_output, const char *name);

void XDGOutputDescription(void *data, XDGOutput *xdg_output, const char *description);

void XDGOutputLogicalPosition(void *data, XDGOutput *xdg_output, int32_t x, int32_t y);

void XDGOutputLogicalSize(void *data, XDGOutput *xdg_output, int32_t width, int32_t height);

void XDGOutputDone(void *data, XDGOutput *xdg_output);

void RegistryGlobal(void *data, WaylandRegistry *registry, uint32_t name, const char *interface, uint32_t version);

void RegistryGlobalRemove(void *data, WaylandRegistry *registry, uint32_t name);

#endif
