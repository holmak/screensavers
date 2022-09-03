#include "Common.h"
#include <string.h>

#define BOX_SIZE (WINDOW_HEIGHT - 80)
#define BOX_BORDER 4
#define STATION_FACETS 16
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

    Mesh station;

    float segmentTime;
} g;

static struct meshBuilder
{
    uint16_t vertexCount;
    int indexCount;
    BasicVertex vertices[1000];
    uint16_t indices[1000];
} meshBuilder;

static void appendVertex(BasicVertex v)
{
    struct meshBuilder *mb = &meshBuilder;
    check(mb->vertexCount < COUNTOF(mb->vertices), "mesh vertex overflow");
    check(mb->indexCount < COUNTOF(mb->indices), "mesh index overflow");
    mb->indices[mb->indexCount++] = mb->vertexCount;
    mb->vertices[mb->vertexCount++] = v;
}

static void appendLine(Vector3 a, Vector3 b)
{
    BasicVertex v = { a, 0, { 0, 0, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } };
    appendVertex(v);
    v.position = b;
    appendVertex(v);
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

    for (uint16_t i = 0; i < STATION_FACETS; i++)
    {
        float theta = 2 * PI * ((float)i / STATION_FACETS);
        Matrix4 rotateY = matrixRotationY(theta);
        Vector3 spoke = matrixTransformPoint(rotateY, (Vector3) { 1, 0, 0 });
        appendLine(spoke, vector3Scale(spoke, 0.95f));
    }

    finishMesh(&g.station);

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
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(3.0f);
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
    glBindVertexArray(g.station.vao);
    Matrix4 modelTransform = matrixScaleUniform(1.0f);
    glUniformMatrix4fv(g.uniformModelTransform, 1, GL_TRUE, modelTransform.e);
    glDrawElements(GL_LINES, (GLsizei)g.station.primitiveCount, GL_UNSIGNED_SHORT, 0);
}
