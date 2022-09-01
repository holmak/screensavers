#include "Common.h"

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

    Mesh cube, station;

    float segmentTime;
} g;

void clear(float* color)
{
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void start()
{
    //=============================================================================================
    // Data
    //=============================================================================================

    BasicVertex cubeVertices[] =
    {
        { { -1, -1, -1 }, 0, { 0, 0, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } },
        { { +1, -1, -1 }, 0, { 0, 0, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } },
        { { -1, +1, -1 }, 0, { 0, 0, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } },
        { { +1, +1, -1 }, 0, { 0, 0, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } },
        { { -1, -1, +1 }, 0, { 0, 0, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } },
        { { +1, -1, +1 }, 0, { 0, 0, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } },
        { { -1, +1, +1 }, 0, { 0, 0, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } },
        { { +1, +1, +1 }, 0, { 0, 0, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } },
    };

    uint16_t cubeIndices[] =
    {
        0, 1, 3, 0, 3, 2, // front
        1, 5, 7, 1, 7, 3, // right
        5, 4, 6, 5, 6, 7, // back
        4, 0, 2, 4, 2, 6, // left
        1, 0, 4, 1, 4, 5, // bottom
        2, 3, 7, 2, 7, 6, // top
    };

    BasicVertex stationVertices[1000];
    uint16_t stationIndices[1000];
    int stationIndexCount;
    {
        uint16_t vert = 0;
        int indx = 0;

        for (uint16_t i = 0; i < STATION_FACETS; i++)
        {
            float theta = 2 * PI * ((float)i / STATION_FACETS);
            Matrix4 rotateY = matrixRotationY(theta);
            Vector3 spoke = matrixTransformPoint(rotateY, (Vector3) { 1, 0, 0 });
            Vector3 normal = matrixTransformPoint(rotateY, (Vector3) { 0, 0, 1 });
            BasicVertex vertex = { spoke, 0, normal, { 0xFF, 0xFF, 0xFF, 0xFF } };
            stationIndices[indx++] = vert;
            stationVertices[vert++] = vertex;
            vertex.position = vector3Scale(vertex.position, 0.95f);
            stationIndices[indx++] = vert;
            stationVertices[vert++] = vertex;
        }

        check(vert <= COUNTOF(stationVertices), "overflow");
        check(indx <= COUNTOF(stationIndices), "overflow");
        stationIndexCount = indx;
    }

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

    createMesh(&g.station);
    setMeshData(&g.station, COUNTOF(stationVertices), stationVertices, stationIndexCount, stationIndices);
    createMesh(&g.cube);
    setMeshData(&g.cube, COUNTOF(cubeVertices), cubeVertices, COUNTOF(cubeIndices), cubeIndices);

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

void screensaverTrench()
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
