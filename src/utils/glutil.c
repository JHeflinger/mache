#include "glutil.h"

static const char* VERT_SRC =
    "#version 100\n"
    "attribute vec2 a_pos;\n"
    "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }\n";

static const char* FRAG_PREFIX =
    "#version 100\n"
    "precision mediump float;\n"
    "uniform vec3  iResolution;\n" // viewport res (px)
    "uniform float iTime;\n" // shader playback time (s)
    "uniform float iTimeDelta;\n" // render time (s)
    "uniform int   iFrame;\n" // shader playback frame
    "uniform vec4  iMouse;\n" // mouse pixel coords
    "uniform vec4  iDate;\n" // (year, month, day, time)
    "uniform float iSampleRate;\n" // sound sample rate
    "\n"
    "#define texture2D texture2D\n"
    "float tanh(float x) { float e2x = exp(2.0 * x); return (e2x - 1.0) / (e2x + 1.0); }\n"
    "vec2 tanh(vec2 x) { vec2 e2x = exp(2.0 * x); return (e2x - 1.0) / (e2x + 1.0); }\n"
    "vec3 tanh(vec3 x) { vec3 e2x = exp(2.0 * x); return (e2x - 1.0) / (e2x + 1.0); }\n"
    "vec4 tanh(vec4 x) { vec4 e2x = exp(2.0 * x); return (e2x - 1.0) / (e2x + 1.0); }\n"
    "\n";

static const char* FRAG_SUFFIX =
    "\n"
    "void main() { vec4 fragColor = vec4(0.0);\n"
        "mainImage(fragColor, gl_FragCoord.xy);\n"
        "gl_FragColor = fragColor;\n"
    "}\n";

static const float QUAD[] = {
    -1.f, -1.f,
     1.f, -1.f,
    -1.f,  1.f,
     1.f, -1.f,
     1.f,  1.f,
    -1.f,  1.f,
};

GLuint CompileShader(GLenum type, const char* src, const char* label) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(sh, sizeof(log), NULL, log);
        fprintf(stderr, "[%s] compile error:\n%s\n", label, log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

bool BuildProgram(Application* app, const char* user_shader) {
    size_t frag_len = strlen(FRAG_PREFIX) + strlen(user_shader) + strlen(FRAG_SUFFIX) + 1;
    char *frag_src = malloc(frag_len);
    if (!frag_src) return false;
    strcpy(frag_src, FRAG_PREFIX);
    strcat(frag_src, user_shader);
    strcat(frag_src, FRAG_SUFFIX);
    GLuint vert = CompileShader(GL_VERTEX_SHADER,   VERT_SRC, "vertex");
    GLuint frag = CompileShader(GL_FRAGMENT_SHADER, frag_src,  "fragment");
    free(frag_src);
    if (!vert || !frag) return false;
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glBindAttribLocation(prog, 0, "a_pos");
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);
    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        fprintf(stderr, "[program] link error:\n%s\n", log);
        glDeleteProgram(prog);
        return false;
    }
    if (app->prog) glDeleteProgram(app->prog);
    app->prog = prog;
    app->u_resolution = glGetUniformLocation(prog, "iResolution");
    app->u_time = glGetUniformLocation(prog, "iTime");
    app->u_timedelta = glGetUniformLocation(prog, "iTimeDelta");
    app->u_frame = glGetUniformLocation(prog, "iFrame");
    app->u_mouse = glGetUniformLocation(prog, "iMouse");
    app->u_date = glGetUniformLocation(prog, "iDate");
    app->u_samplerate = glGetUniformLocation(prog, "iSampleRate");
    return true;
}

void InitGeometry(Application* app) {
    glGenBuffers(1, &app->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, app->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD), QUAD, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
