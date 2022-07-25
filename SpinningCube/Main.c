#include "Common.h"
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "SDL2main")
#pragma comment(lib, "SDL2")

static FILE *GLLog;

//=============================================================================================
// Basics
//=============================================================================================

void check(bool condition)
{
    if (!condition)
    {
        exit(1);
    }
}

void *xalloc(size_t size)
{
    void *p = calloc(1, size);
    check(p != NULL);
    return p;
}

char *readTextFile(char *path)
{
    FILE *f = fopen(path, "rb");
    check(fseek(f, 0, SEEK_END) == 0);
    long len = ftell(f);
    check(len >= 0);
    check(fseek(f, 0, SEEK_SET) == 0);
    char *text = xalloc(len + 1);
    check(fread(text, len, 1, f) == 1);
    text[len] = '\0';
    return text;
}

//=============================================================================================
// GL
//=============================================================================================

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
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    printShaderLog(shader, label, glGetShaderiv, GL_COMPILE_STATUS, glGetShaderInfoLog);
    return shader;
}

GLuint linkShaderProgram(GLuint vertexShader, GLuint fragmentShader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    printShaderLog(program, "program", glGetProgramiv, GL_LINK_STATUS, glGetProgramInfoLog);

    glValidateProgram(program);
    printShaderLog(program, "program validation", glGetProgramiv, GL_VALIDATE_STATUS, glGetProgramInfoLog);

    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}

GLuint compileShaderProgram(char *vertexShaderSource, char *fragmentShaderSource)
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, "vertex shader", vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, "fragment shader", fragmentShaderSource);
    return linkShaderProgram(vs, fs);
}

void createMesh(Mesh *mesh)
{
    memset(mesh, 0, sizeof(*mesh));

    glGenVertexArrays(1, &mesh->vao);
    glBindVertexArray(mesh->vao);

    glGenBuffers(1, &mesh->vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexBuffer);

    glGenBuffers(1, &mesh->indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->indexBuffer);

    // Vertex layout:
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BasicVertex), (void*)offsetof(BasicVertex, position));
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(BasicVertex), (void*)offsetof(BasicVertex, color));

    mesh->primitiveCount = 0;
}

void setMeshData(
    Mesh *mesh,
    size_t vertexCount, BasicVertex *vertexData,
    size_t indexCount, uint16_t *indexData)
{
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(vertexData[0]), vertexData, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(indexData[0]), indexData, GL_STATIC_DRAW);
    mesh->primitiveCount = indexCount;
}

//=============================================================================================
// Matrices (4x4)
//=============================================================================================

float *matrixElement(Matrix4 *matrix, int row, int column)
{
    return matrix->e + column * 4 + row;
}

Matrix4 matrixMultiply(Matrix4 left, Matrix4 right)
{
    Matrix4 m;
    for (int col = 0; col < 4; col++)
    {
        for (int row = 0; row < 4; row++)
        {
            float elem = 0;
            for (int i = 0; i < 4; i++)
            {
                elem += *matrixElement(&left, row, i) * *matrixElement(&right, i, col);
            }
            *matrixElement(&m, row, col) = elem;
        }
    }
    return m;
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

Matrix4 matrixTranslation(Vector3 v)
{
    Matrix4 m = {
        1, 0, 0, v.x,
        0, 1, 0, v.y,
        0, 0, 1, v.z,
        0, 0, 0, 1,
    };
    return m;
}

Matrix4 matrixTranslationF(float x, float y, float z)
{
    return matrixTranslation((Vector3){ x, y, z });
}

Matrix4 matrixRotationX(float radians)
{
    float sinx = (float)sin(radians);
    float cosx = (float)cos(radians);
    Matrix4 m = {
        1, 0, 0, 0,
        0, cosx, -sinx, 0,
        0, sinx, cosx, 0,
        0, 0, 0, 1,
    };
    return m;
}

Matrix4 matrixRotationY(float radians)
{
    float sinx = (float)sin(radians);
    float cosx = (float)cos(radians);
    Matrix4 m = {
        cosx, 0, sinx, 0,
        0, 1, 0, 0,
        -sinx, 0, cosx, 0,
        0, 0, 0, 1,
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

Matrix4 matrixScaleUniform(float s)
{
    Matrix4 m = {
        s, 0, 0, 0,
        0, s, 0, 0,
        0, 0, s, 0,
        0, 0, 0, 1,
    };
    return m;
}

//=============================================================================================
// Main program
//=============================================================================================

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

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    check(SDL_Init(SDL_INIT_EVERYTHING) == 0);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);

    if (DEBUG_GRAPHICS)
    {
        GLLog = fopen("gl.log", "w");
        check(GLLog);

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    }

    SDL_Window *window = SDL_CreateWindow(
        "Screensaver",
        SDL_WINDOWPOS_CENTERED_DISPLAY(1), SDL_WINDOWPOS_CENTERED_DISPLAY(1),
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);
    check(window != NULL);

    SDL_GLContext context = SDL_GL_CreateContext(window);
    check(context != 0);
    LoadGL();
    check(SDL_GL_SetSwapInterval(-1) == 0);

    if (DEBUG_GRAPHICS)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(onGLDebugMessage, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

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

        screensaverCube();

        SDL_GL_SwapWindow(window);
    }
}
