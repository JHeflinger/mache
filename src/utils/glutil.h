#ifndef GLUTIL_H
#define GLUTIL_H

#include "core/definitions.h"

GLuint CompileShader(GLenum type, const char* src, const char* label);

bool BuildProgram(Application* app, const char* user_shader);

void InitGeometry(Application* app);

#endif
