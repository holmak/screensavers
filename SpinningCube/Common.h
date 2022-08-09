#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include "GL.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define DEBUG_GRAPHICS true

#define PI ((float)M_PI)
#define TO_RADIANS (PI / 180.0f)
#define FRAME_TIME (1 / 60.0f)
#define UNUSED(var) (void)(var)
#define COUNTOF(a) (sizeof(a) / sizeof(a[0]))

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

typedef struct Mesh
{
    GLuint vao, vertexBuffer, indexBuffer;
    size_t primitiveCount;
} Mesh;

//=============================================================================================
// Basics
//=============================================================================================

void check(bool condition);

void *xalloc(size_t size);

char *readTextFile(char *path);

//=============================================================================================
// GL
//=============================================================================================

GLuint compileShaderProgram(char *vertexShaderSource, char *fragmentShaderSource);

void createMesh(Mesh *mesh);

void setMeshData(
    Mesh *mesh,
    size_t vertexCount, BasicVertex *vertexData,
    size_t indexCount, uint16_t *indexData);

//=============================================================================================
// Matrices
//=============================================================================================

Matrix4 matrixPixelPerfect();

Matrix4 matrixPerspective(float near, float fov);

Matrix4 matrixMultiply(Matrix4 left, Matrix4 right);

void matrixConcat(Matrix4 *left, Matrix4 right);

Matrix4 matrixTranslation(Vector3 v);

Matrix4 matrixTranslationF(float x, float y, float z);

Matrix4 matrixRotationX(float radians);

Matrix4 matrixRotationY(float radians);

Matrix4 matrixRotationZ(float radians);

Matrix4 matrixScaleF(float x, float y, float z);

Matrix4 matrixScaleUniform(float s);

Vector3 matrixTransformPoint(Matrix4 transform, Vector3 point);

//=============================================================================================
// Modes
//=============================================================================================

void screensaverCube();

void screensaverCheckers();
