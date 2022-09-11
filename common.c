#include "common.h"
#include <stdio.h>
#include <string.h>

extern FILE *GLLog;

//=============================================================================================
// Basics
//=============================================================================================

void check(bool condition, char *message)
{
    if (!condition)
    {
        fprintf(stderr, "error: %s\n", message);
        exit(1);
    }
}

void *xalloc(size_t size)
{
    void *p = calloc(1, size);
    check(p != NULL, "xalloc");
    return p;
}

char *readTextFile(char *path)
{
    FILE *f = fopen(path, "rb");
    if (fseek(f, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "error: cannot read file: %s\n", path);
        exit(1);
    }
    long len = ftell(f);
    check(len >= 0, "ftell");
    check(fseek(f, 0, SEEK_SET) == 0, "fseek");
    char *text = xalloc(len + 1);
    check(fread(text, len, 1, f) == 1, "fread");
    fclose(f);
    text[len] = '\0';
    return text;
}

//=============================================================================================
// Math
//=============================================================================================

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

Vector3 vector3Scale(Vector3 v, float scale)
{
    v.x *= scale;
    v.y *= scale;
    v.z *= scale;
    return v;
}

Vector3 vector3RotateX(Vector3 v, float angle)
{
    return matrixTransformPoint(matrixRotationX(angle), v);
}

Vector3 vector3RotateY(Vector3 v, float angle)
{
    return matrixTransformPoint(matrixRotationY(angle), v);
}

Vector3 vector3RotateZ(Vector3 v, float angle)
{
    return matrixTransformPoint(matrixRotationZ(angle), v);
}

float vector3Dot(Vector3 a, Vector3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
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
    check(success, "compiling shader/program");
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PointVertex), (void*)offsetof(PointVertex, position));

    mesh->primitiveCount = 0;
}

void setMeshData(
    Mesh *mesh,
    size_t vertexCount, PointVertex *vertexData,
    size_t indexCount, uint16_t *indexData)
{
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(vertexData[0]), vertexData, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(indexData[0]), indexData, GL_STATIC_DRAW);
    mesh->primitiveCount = indexCount; // TODO: Divide by 2 or 3?
}

//=============================================================================================
// Matrices (4x4)
//=============================================================================================

Matrix4 matrixIdentity()
{
    Matrix4 m = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    return m;
}

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

void matrixConcat(Matrix4 *left, Matrix4 right)
{
    *left = matrixMultiply(*left, right);
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
    return matrixTranslation((Vector3) { x, y, z });
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

Matrix4 matrixScaleF(float x, float y, float z)
{
    Matrix4 m = {
        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, z, 0,
        0, 0, 0, 1,
    };
    return m;
}

Matrix4 matrixScaleUniform(float s)
{
    return matrixScaleF(s, s, s);
}

Vector3 matrixTransformPoint(Matrix4 transform, Vector3 point)
{
    float *m = transform.e;
    Vector3 p = point;
    Vector3 result;
    result.x = m[0] * p.x + m[1] * p.y + m[2] * p.z + m[3];
    result.y = m[4] * p.x + m[5] * p.y + m[6] * p.z + m[7];
    result.z = m[8] * p.x + m[9] * p.y + m[10] * p.z + m[11];
    return result;
}
