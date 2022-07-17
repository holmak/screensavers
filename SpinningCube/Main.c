#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "SDL/SDL.h"
#include "GL.h"

#pragma comment(lib, "SDL2main")
#pragma comment(lib, "SDL2")
#pragma comment(lib, "opengl32")

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900
#define DEBUG_GRAPHICS true

#define TO_RADIANS (float)(M_PI / 180.0)
#define FRAME_TIME (1 / 60.0f)
#define UNUSED(var) (void)(var)

typedef struct Vector3
{
    float x, y, z;
} Vector3;

typedef struct Matrix4
{
    float e[16];
} Matrix4;

typedef struct Color
{
    float r, g, b, a;
} Color;

typedef struct PackedColor
{
    uint8_t r, g, b, a;
} PackedColor;

typedef struct BasicVertex
{
    Vector3 position;
    PackedColor color;
} BasicVertex;

static FILE *GLLog;

void check(bool condition)
{
    if (!condition)
    {
        exit(1);
    }
}

void onGLDebugMessage(GLenum source, GLenum type, unsigned id, GLenum severity,
    GLsizei length, const char *message, const void *userParam)
{
    UNUSED(length);
    UNUSED(userParam);

    char *sourceName = "unknown";
    char *typeName = "unknown";
    char *severityName = "unknown";

    switch (source)
    {
    case GL_DEBUG_SOURCE_API: sourceName = "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceName = "window system"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceName = "shader compiler"; break;
    case GL_DEBUG_SOURCE_OTHER: sourceName = "other"; break;
    }

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR: typeName = "error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeName = "deprecated behavior"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: typeName = "undefined behavior"; break;
    case GL_DEBUG_TYPE_PORTABILITY: typeName = "portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE: typeName = "performance"; break;
    case GL_DEBUG_TYPE_OTHER: typeName = "other"; break;
    }

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH: severityName = "high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM: severityName = "medium"; break;
    case GL_DEBUG_SEVERITY_LOW: severityName = "low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: severityName = "notice"; break;
    }

    fprintf(GLLog, "[%s | %s | %s | %u] %s\n", severityName, typeName, sourceName, id, message);
    fflush(GLLog);

    // Break when a serious error occurs:
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
    {
    }
}

void printShaderLog(
    GLuint object, char *label,
    GLPROC_glGetShaderiv get, GLenum status,
    GLPROC_glGetShaderInfoLog getLog)
{
    GLint success;
    get(object, status, &success);
    if (!success)
    {
        fprintf(GLLog, "compiled/linked %s: FAILED\n", label);
    }

    static char errors[4096];
    GLsizei length;
    getLog(object, sizeof(errors), &length, errors);
    if (length > 0)
    {
        fprintf(GLLog, "%s: %s\n\n", label, errors);
    }

    fflush(GLLog);
    check(success);
}

GLuint compileShader(GLenum type, char *label, const char *source)
{
    GLuint shader = oglCreateShader(type);
    oglShaderSource(shader, 1, &source, NULL);
    oglCompileShader(shader);
    printShaderLog(shader, label, oglGetShaderiv, GL_COMPILE_STATUS, oglGetShaderInfoLog);
    return shader;
}

GLuint linkShaderProgram(GLuint vertexShader, GLuint fragmentShader)
{
    GLuint program = oglCreateProgram();
    oglAttachShader(program, vertexShader);
    oglAttachShader(program, fragmentShader);
    oglLinkProgram(program);
    printShaderLog(program, "program", oglGetProgramiv, GL_LINK_STATUS, oglGetProgramInfoLog);

    oglValidateProgram(program);
    printShaderLog(program, "program validation", oglGetProgramiv, GL_VALIDATE_STATUS, oglGetProgramInfoLog);

    oglDetachShader(program, vertexShader);
    oglDeleteShader(vertexShader);
    oglDetachShader(program, fragmentShader);
    oglDeleteShader(fragmentShader);

    return program;
}

Matrix4 matrixPixelPerfect()
{
    Matrix4 m = {
        2.0f / WINDOW_WIDTH, 0, 0, -1,
        0, 2.0f / WINDOW_HEIGHT, 0, -1,
        0, 0, -0.001f, 0,
        0, 0, 0, 1,
    };
    return m;
}

Matrix4 matrixPerspective(float near, float fov)
{
    float e = 1 / (float)tan(fov / 2);
    float a = (float)WINDOW_HEIGHT / WINDOW_WIDTH;
    Matrix4 m = {
        e, 0, 0, 0,
        0, e / a, 0, 0,
        0, 0, -1, -2 * near,
        0, 0, -1, 0,
    };
    return m;
}

Matrix4 matrixRotationZ(float radians)
{
    float sinx = (float)sin(radians);
    float cosx = (float)cos(radians);
    Matrix4 m = {
        cosx, -sinx, 0, 0,
        sinx, cosx, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    return m;
}

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    check(SDL_Init(SDL_INIT_EVERYTHING) == 0);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    if (DEBUG_GRAPHICS)
    {
        GLLog = fopen("gl.log", "w");
        check(GLLog);

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    }

    SDL_Window *window = SDL_CreateWindow(
        "Screensaver",
        SDL_WINDOWPOS_CENTERED_DISPLAY(1), SDL_WINDOWPOS_CENTERED_DISPLAY(1),
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    check(window != NULL);

    SDL_GLContext context = SDL_GL_CreateContext(window);
    check(context != 0);
    LoadGL();
    check(SDL_GL_SetSwapInterval(-1) == 0);

    if (DEBUG_GRAPHICS)
    {
        oglEnable(GL_DEBUG_OUTPUT);
        oglEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        oglDebugMessageCallback(onGLDebugMessage, NULL);
        oglDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    }

    oglDisable(GL_CULL_FACE);

    //=============================================================================================
    // Content
    //=============================================================================================

    BasicVertex vertexData[] =
    {
        { -0.5f, -0.5f, -5.0f, 0xFF, 0x00, 0x00, 0xFF, },
        { 0.5f, -0.5f, -5.0f, 0x00, 0xFF, 0x00, 0xFF, },
        { 0.5f, 0.5f, -5.0f, 0x00, 0x00, 0xFF, 0xFF, },
        { -0.5f, 0.5f, -5.0f, 0xFF, 0xFF, 0x00, 0xFF, },
    };

    uint16_t indexData[] =
    {
        0, 1, 2,
        0, 2, 3,
    };

    const char *vertexShaderSource =
        "#version 330\n"
        "\n"
        "uniform mat4 uniProjection;\n"
        "\n"
        "layout(location = 0) in vec3 inPosition;\n"
        "layout(location = 1) in vec4 inColor;\n"
        "\n"
        "out vec4 vertColor;\n"
        "\n"
        "void main() {\n"
        "    gl_Position = uniProjection * vec4(inPosition, 1.0f);\n"
        "    vertColor = inColor;\n"
        "}\n";

    const char *fragmentShaderSource =
        "#version 330\n"
        "\n"
        "in vec4 vertColor;\n"
        "\n"
        "out vec4 fragColor;\n"
        "\n"
        "void main() {\n"
        "    fragColor = vertColor;\n"
        "}\n";

    //=============================================================================================
    // Create GL resources
    //=============================================================================================

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, "vertex shader", vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, "fragment shader", fragmentShaderSource);
    GLuint program = linkShaderProgram(vertexShader, fragmentShader);
    GLuint uniformProjection = oglGetUniformLocation(program, "uniProjection");

    GLuint vao;
    oglGenVertexArrays(1, &vao);
    oglBindVertexArray(vao);
    oglEnableVertexAttribArray(0);
    oglEnableVertexAttribArray(1);

    GLuint vertexBuffer;
    oglGenBuffers(1, &vertexBuffer);
    oglBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    oglBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

    GLuint indexBuffer;
    oglGenBuffers(1, &indexBuffer);
    oglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    oglBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexData), indexData, GL_STATIC_DRAW);
    
    // Specify vertex layout:
    oglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BasicVertex), (void*)offsetof(BasicVertex, position));
    oglVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(BasicVertex), (void*)offsetof(BasicVertex, color));

    //=============================================================================================
    // Main loop
    //=============================================================================================

    float angle = 0;

    for (;;)
    {
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
            {
                exit(0);
            }
        }

        angle += FRAME_TIME;

        oglClearColor(0.5f, 0.5f, 1.0f, 0.0f);
        oglClear(GL_COLOR_BUFFER_BIT);

        oglUseProgram(program);
        
        Matrix4 projection = matrixRotationZ(angle);
        projection = matrixPerspective(0.1f, 60.0f * TO_RADIANS);

        oglUniformMatrix4fv(uniformProjection, 1, GL_TRUE, projection.e);
        oglDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

        SDL_GL_SwapWindow(window);
    }
}
