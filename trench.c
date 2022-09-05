#include "Common.h"
#include <string.h>

#define BOX_SIZE (WINDOW_HEIGHT - 80)
#define BOX_BORDER 4
#define STATION_FACETS 30
#define CRATER_FACETS 16
#define CRATER_ANGLE_OUTER 23
#define CRATER_ANGLE_INNER 15
#define ANIMATION_RATE 0.04f

static float LIGHT[] = { 0.75f, 0.9f, 0.9f, 1.0f };
static float DARK[] = { 0.0f, 0.0f, 0.0f, 0.0f };

static struct trenchGlobals
{
    bool started;

    GLuint program;
    GLuint uProjection;
    GLuint uModelTransform;
    GLuint uColor;

    Mesh points, lines;

    float segmentTime;
} g;

static struct meshBuilder
{
    uint16_t vertexCount;
    int indexCount;
    PointVertex vertices[4000];
    uint16_t indices[4000];
} meshBuilder;

static void appendVertex(PointVertex v)
{
    struct meshBuilder *mb = &meshBuilder;
    check(mb->vertexCount < COUNTOF(mb->vertices), "mesh vertex overflow");
    check(mb->indexCount < COUNTOF(mb->indices), "mesh index overflow");
    mb->indices[mb->indexCount++] = mb->vertexCount;
    mb->vertices[mb->vertexCount++] = v;
}

static void appendPoint(Vector3 p)
{
    PointVertex v = { p, 1 };
    appendVertex(v);
}

static void appendLine(Vector3 a, Vector3 b)
{
    appendPoint(a);
    appendPoint(b);
}

static void finishMesh(Mesh *mesh)
{
    struct meshBuilder *mb = &meshBuilder;
    createMesh(mesh);
    setMeshData(mesh, mb->vertexCount, mb->vertices, mb->indexCount, mb->indices);
    memset(mb, 0, sizeof(*mb));
}

Vector3 spherePoint(float longitude, float latitude, float radius)
{
    Matrix4 transform = matrixRotationZ(latitude);
    matrixConcat(&transform, matrixRotationY(longitude));
    return matrixTransformPoint(transform, (Vector3){ radius, 0, 0 });
}

void clear(float* color)
{
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void start()
{
    //=============================================================================================
    // Construct meshes
    //=============================================================================================

    for (int j = 1; j < STATION_FACETS / 3; j++)
    {
        float latitude = ((float)j / (STATION_FACETS / 3)) * PI / 2;

        for (int i = 0; i < STATION_FACETS; i++)
        {
            float offset = ((j % 2) == 1) ? 0.5f : 0.0f;
            float longitude = ((float)(i + offset) / STATION_FACETS) * 2 * PI;
            appendPoint(spherePoint(longitude, latitude, 1.0f));
            appendPoint(spherePoint(longitude, -latitude, 1.0f));
        }
    }

    finishMesh(&g.points);

    for (int i = 0; i < STATION_FACETS; i++)
    {
        float longitudeA = ((float)i / STATION_FACETS) * 2 * PI;
        float longitudeB = ((float)(i + 1) / STATION_FACETS) * 2 * PI;
        appendLine(spherePoint(longitudeA, 0, 1.0f), spherePoint(longitudeB, 0, 1.0f));
    }

    for (int i = 0; i < CRATER_FACETS; i++)
    {
        Vector3 p0 = { 1, 0, 0 };
        p0 = vector3RotateY(p0, CRATER_ANGLE_OUTER * TO_RADIANS);
        p0 = vector3RotateX(p0, ((i + 0.5f) / CRATER_FACETS) * 2 * PI);
        Vector3 p1 = { 0.85f, 0, 0 };
        p1 = vector3RotateY(p1, CRATER_ANGLE_INNER * TO_RADIANS);
        p1 = vector3RotateX(p1, ((i + 0.5f) / CRATER_FACETS) * 2 * PI);
        appendLine(p0, p1);
    }

    finishMesh(&g.lines);

    //=============================================================================================
    // GL resources
    //=============================================================================================

    char *vertexShaderSource = readTextFile("assets/shaders/point.v.glsl");
    char *fragmentShaderSource = readTextFile("assets/shaders/point.f.glsl");
    g.program = compileShaderProgram(vertexShaderSource, fragmentShaderSource);
    g.uProjection = glGetUniformLocation(g.program, "uProjection");
    g.uModelTransform = glGetUniformLocation(g.program, "uModelTransform");
    g.uColor = glGetUniformLocation(g.program, "uColor");

    //=============================================================================================
    // Program state
    //=============================================================================================

    g.segmentTime = 0;

    //=============================================================================================
    // GL state
    //=============================================================================================

    glDisable(GL_DEPTH_TEST);
    glLineWidth(3.0f);
    glPointSize(3.0f);
}

void screensaver()
{
    if (!g.started)
    {
        start();
        g.started = true;
    }

    g.segmentTime += FRAME_TIME * ANIMATION_RATE;
    while (g.segmentTime > 1) g.segmentTime -= 1.0f;

    // Draw box and constrain scene to it:
    {
        int bx = (WINDOW_WIDTH - BOX_SIZE) / 2;
        int by = (WINDOW_HEIGHT - BOX_SIZE) / 2;

        glDisable(GL_SCISSOR_TEST);
        clear(DARK);

        glEnable(GL_SCISSOR_TEST);
        glScissor(bx - BOX_BORDER, by - BOX_BORDER, BOX_SIZE + 2 * BOX_BORDER, BOX_SIZE + 2 * BOX_BORDER);
        clear(LIGHT);

        glScissor(bx, by, BOX_SIZE, BOX_SIZE);
        clear(DARK);
    }

    glUniform4fv(g.uColor, 1, LIGHT);

    // Set up projection:
    glUseProgram(g.program);
    float angleX = lerp(0, PI, g.segmentTime);
    float angleY = 3 * PI / 2;
    Matrix4 projectionAndView = matrixRotationY(angleY);
    matrixConcat(&projectionAndView, matrixRotationX(angleX));
    matrixConcat(&projectionAndView, matrixTranslationF(0, 0, -11));
    matrixConcat(&projectionAndView, matrixPerspective(0.1f, 30.0f * TO_RADIANS));
    glUniformMatrix4fv(g.uProjection, 1, GL_TRUE, projectionAndView.e);

    // Draw sphere:
    Matrix4 modelTransform = matrixScaleUniform(1.0f);
    glUniformMatrix4fv(g.uModelTransform, 1, GL_TRUE, modelTransform.e);
    glBindVertexArray(g.points.vao);
    glDrawElements(GL_POINTS, (GLsizei)g.points.primitiveCount, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(g.lines.vao);
    glDrawElements(GL_LINES, (GLsizei)g.lines.primitiveCount, GL_UNSIGNED_SHORT, 0);
}
