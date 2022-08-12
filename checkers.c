#include "Common.h"

#define BOARD_SIZE 8
#define CYLINDER_FACETS 12

static struct checkersGlobals
{
    bool started;

    GLuint program;
    GLuint uniformProjection;
    GLuint uniformModelTransform;
    GLuint uniformModelColor;
    GLuint uniformAmbientLight;

    Mesh cube, plane, cylinder;

    float angle;
} g;

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

    BasicVertex planeVertices[] =
    {
        { { -1, 0, -1 }, 0, { 0, 1, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } },
        { { +1, 0, -1 }, 0, { 0, 1, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } },
        { { -1, 0, +1 }, 0, { 0, 1, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } },
        { { +1, 0, +1 }, 0, { 0, 1, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } },
    };

    uint16_t planeIndices[] =
    {
        0, 1, 3, 0, 3, 2,
    };

    BasicVertex cylinderVertices[3 * CYLINDER_FACETS];
    uint16_t cylinderIndices[9 * CYLINDER_FACETS];
    for (uint16_t i = 0; i < CYLINDER_FACETS; i++)
    {
        uint16_t v = 3 * i;
        int tri = 9 * i;

        float theta = 2 * PI * ((float)i / CYLINDER_FACETS);
        Matrix4 rotateY = matrixRotationY(theta);
        Vector3 spoke = matrixTransformPoint(rotateY, (Vector3) { 1, 0, 0 });
        Vector3 normal = matrixTransformPoint(rotateY, (Vector3) { 0, 0, 1 });
        BasicVertex bottom = { spoke, 0, normal, { 0xFF, 0xFF, 0xFF, 0xFF } };
        BasicVertex top = bottom;
        top.position.y = 0.5f;
        BasicVertex topFlat = top;
        topFlat.normal = (Vector3){ 0, 1, 0 };
        cylinderVertices[v + 0] = bottom;
        cylinderVertices[v + 1] = top;
        cylinderVertices[v + 2] = topFlat;

        // Side triangles:
        uint16_t end = COUNTOF(cylinderVertices);
        cylinderIndices[tri + 0] = (v) % end;
        cylinderIndices[tri + 1] = (v + 1) % end;
        cylinderIndices[tri + 2] = (v + 4) % end;
        cylinderIndices[tri + 3] = (v) % end;
        cylinderIndices[tri + 4] = (v + 4) % end;
        cylinderIndices[tri + 5] = (v + 3) % end;

        // Top triangles:
        cylinderIndices[tri + 6] = 2;
        cylinderIndices[tri + 7] = (v + 5) % end;
        cylinderIndices[tri + 8] = v + 2;
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

    createMesh(&g.cube);
    setMeshData(&g.cube, COUNTOF(cubeVertices), cubeVertices, COUNTOF(cubeIndices), cubeIndices);
    createMesh(&g.plane);
    setMeshData(&g.plane, COUNTOF(planeVertices), planeVertices, COUNTOF(planeIndices), planeIndices);
    createMesh(&g.cylinder);
    setMeshData(&g.cylinder, COUNTOF(cylinderVertices), cylinderVertices, COUNTOF(cylinderIndices), cylinderIndices);

    //=============================================================================================
    // Program state
    //=============================================================================================

    g.angle = 0;

    //=============================================================================================
    // GL state
    //=============================================================================================

    glDisable(GL_STENCIL_TEST);
}

void screensaverCheckers()
{
    if (!g.started)
    {
        start();
        g.started = true;
    }

    g.angle += FRAME_TIME * 0.1f;
    g.angle = fmodf(g.angle, 2 * PI);

    glClearColor(0.7f, 0.7f, 0.7f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniform4f(g.uniformModelColor, 1, 1, 1, 1);
    glUniform1f(g.uniformAmbientLight, 0.5f);

    // Set up projection:
    glUseProgram(g.program);
    Matrix4 projectionAndView = matrixRotationY(g.angle);
    matrixConcat(&projectionAndView, matrixRotationX(45 * TO_RADIANS));
    matrixConcat(&projectionAndView, matrixTranslationF(0, -1, -5));
    matrixConcat(&projectionAndView, matrixPerspective(0.1f, 90.0f * TO_RADIANS));
    glUniformMatrix4fv(g.uniformProjection, 1, GL_TRUE, projectionAndView.e);

    // Draw piece:
    Matrix4 cubeTransform = matrixScaleUniform(0.45f);
    matrixConcat(&cubeTransform, matrixTranslationF(0, 0.05f, 0));
    glUniformMatrix4fv(g.uniformModelTransform, 1, GL_TRUE, cubeTransform.e);
    glBindVertexArray(g.cylinder.vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)g.cylinder.primitiveCount, GL_UNSIGNED_SHORT, 0);

    // Draw board:
    glBindVertexArray(g.plane.vao);
    for (int gy = 0; gy < BOARD_SIZE; gy++)
    {
        for (int gx = 0; gx < BOARD_SIZE; gx++)
        {
            bool isRed = (gx ^ gy) & 1;
            float w = 0.1f;
            float red[] = { 1, w, w, 1 };
            float black[] = { w, w, w, 1 };

            Matrix4 modelTransform = matrixScaleUniform(0.5f);
            matrixConcat(&modelTransform, matrixTranslationF(gx - BOARD_SIZE / 2 + 0.5f, 0, gy - BOARD_SIZE / 2 + 0.5f));
            glUniformMatrix4fv(g.uniformModelTransform, 1, GL_TRUE, modelTransform.e);
            glUniform4fv(g.uniformModelColor, 1, isRed ? red : black);
            glDrawElements(GL_TRIANGLES, (GLsizei)g.plane.primitiveCount, GL_UNSIGNED_SHORT, 0);
        }
    }
}
