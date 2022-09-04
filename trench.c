#include "Common.h"
#include <string.h>

#define BOX_SIZE (WINDOW_HEIGHT - 80)
#define BOX_BORDER 4
#define STATION_FACETS 30
#define ANIMATION_RATE 0.02f

static float LIGHT[] = { 0.75f, 0.9f, 0.9f, 1.0f };
static float DARK[] = { 0.0f, 0.0f, 0.0f, 0.0f };

static struct trenchGlobals
{
    bool started;

    GLuint program;
    GLuint uniformProjection;
    GLuint uniformModelTransform;
    GLuint uniformModelColor;
    GLuint uniformAmbientLight;

    Mesh points, lines;

    float segmentTime;
} g;

static struct meshBuilder
{
    uint16_t vertexCount;
    int indexCount;
    BasicVertex vertices[4000];
    uint16_t indices[4000];
} meshBuilder;

static void appendVertex(BasicVertex v)
{
    struct meshBuilder *mb = &meshBuilder;
    check(mb->vertexCount < COUNTOF(mb->vertices), "mesh vertex overflow");
    check(mb->indexCount < COUNTOF(mb->indices), "mesh index overflow");
    mb->indices[mb->indexCount++] = mb->vertexCount;
    mb->vertices[mb->vertexCount++] = v;
}

static void appendPoint(Vector3 p)
{
    BasicVertex v = { p, 0, { 0, 0, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } };
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
        Matrix4 rotateNorth = matrixRotationZ(latitude);
        Matrix4 rotateSouth = matrixRotationZ(-latitude);

        for (int i = 0; i < STATION_FACETS; i++)
        {
            float offset = ((j % 2) == 1) ? 0.5f : 0.0f;
            float longitude = ((float)(i + offset) / STATION_FACETS) * 2 * PI;
            Matrix4 rotateY = matrixRotationY(longitude);
            appendPoint(matrixTransformPoint(matrixMultiply(rotateNorth, rotateY), (Vector3) { 1, 0, 0 }));
            appendPoint(matrixTransformPoint(matrixMultiply(rotateSouth, rotateY), (Vector3) { 1, 0, 0 }));
        }
    }

    finishMesh(&g.points);

    for (int i = 0; i < STATION_FACETS; i++)
    {
        float longitudeA = ((float)i / STATION_FACETS) * 2 * PI;
        float longitudeB = ((float)(i + 1) / STATION_FACETS) * 2 * PI;
        Vector3 a = matrixTransformPoint(matrixRotationY(longitudeA), (Vector3) { 1, 0, 0 });
        Vector3 b = matrixTransformPoint(matrixRotationY(longitudeB), (Vector3) { 1, 0, 0 });
        appendLine(a, b);
    }

    finishMesh(&g.lines);

    char* vertexShaderSource = readTextFile("assets/shaders/cube.v.glsl");
    char* fragmentShaderSource = readTextFile("assets/shaders/cube.f.glsl");

    //=============================================================================================
    // GL resources
    //=============================================================================================

    g.program = compileShaderProgram(vertexShaderSource, fragmentShaderSource);
    g.uniformProjection = glGetUniformLocation(g.program, "uniProjection");
    g.uniformModelTransform = glGetUniformLocation(g.program, "uniModelTransform");
    g.uniformModelColor = glGetUniformLocation(g.program, "uniModelColor");
    g.uniformAmbientLight = glGetUniformLocation(g.program, "uniAmbientLight");

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

    glUniform4fv(g.uniformModelColor, 1, LIGHT);
    glUniform1f(g.uniformAmbientLight, 1.0f);

    // Set up projection:
    glUseProgram(g.program);
    float angleX = lerp(0, PI, g.segmentTime);
    float angleY = 0;
    Matrix4 projectionAndView = matrixRotationY(angleY);
    matrixConcat(&projectionAndView, matrixRotationX(angleX));
    matrixConcat(&projectionAndView, matrixTranslationF(0, 0, -3));
    matrixConcat(&projectionAndView, matrixPerspective(0.1f, 90.0f * TO_RADIANS));
    glUniformMatrix4fv(g.uniformProjection, 1, GL_TRUE, projectionAndView.e);

    // Draw sphere:
    Matrix4 modelTransform = matrixScaleUniform(1.0f);
    glUniformMatrix4fv(g.uniformModelTransform, 1, GL_TRUE, modelTransform.e);
    glBindVertexArray(g.points.vao);
    glDrawElements(GL_POINTS, (GLsizei)g.points.primitiveCount, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(g.lines.vao);
    glDrawElements(GL_LINES, (GLsizei)g.lines.primitiveCount, GL_UNSIGNED_SHORT, 0);
}
