#include "Common.h"

#define BOARD_SIZE 8

static struct checkersGlobals
{
    bool started;

    GLuint program;
    GLuint uniformProjection;
    GLuint uniformModelTransform;
    GLuint uniformModelColor;

    Mesh cube, plane;

    float angle;
} g;

static void start()
{
    //=============================================================================================
    // Data
    //=============================================================================================

    BasicVertex cubeVertices[] =
    {
        { -1, -1, -1, 0xFF, 0xFF, 0xFF, 0xFF, },
        { +1, -1, -1, 0xFF, 0xFF, 0xFF, 0xFF, },
        { -1, +1, -1, 0xFF, 0xFF, 0xFF, 0xFF, },
        { +1, +1, -1, 0xFF, 0xFF, 0xFF, 0xFF, },
        { -1, -1, +1, 0xFF, 0xFF, 0xFF, 0xFF, },
        { +1, -1, +1, 0xFF, 0xFF, 0xFF, 0xFF, },
        { -1, +1, +1, 0xFF, 0xFF, 0xFF, 0xFF, },
        { +1, +1, +1, 0xFF, 0xFF, 0xFF, 0xFF, },
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
        { -1, 0, -1, 0xFF, 0xFF, 0xFF, 0xFF, },
        { +1, 0, -1, 0xFF, 0xFF, 0xFF, 0xFF, },
        { -1, 0, +1, 0xFF, 0xFF, 0xFF, 0xFF, },
        { +1, 0, +1, 0xFF, 0xFF, 0xFF, 0xFF, },
    };

    uint16_t planeIndices[] =
    {
        0, 1, 3, 0, 3, 2,
    };

    char* vertexShaderSource = readTextFile("assets/shaders/cube.v.glsl");
    char* fragmentShaderSource = readTextFile("assets/shaders/cube.f.glsl");

    //=============================================================================================
    // GL resources
    //=============================================================================================

    g.program = compileShaderProgram(vertexShaderSource, fragmentShaderSource);
    g.uniformProjection = glGetUniformLocation(g.program, "uniProjection");
    g.uniformModelTransform = glGetUniformLocation(g.program, "uniModelTransform");
    g.uniformModelColor = glGetUniformLocation(g.program, "uniModelColor");

    createMesh(&g.cube);
    setMeshData(&g.cube, COUNTOF(cubeVertices), cubeVertices, COUNTOF(cubeIndices), cubeIndices);
    createMesh(&g.plane);
    setMeshData(&g.plane, COUNTOF(planeVertices), planeVertices, COUNTOF(planeIndices), planeIndices);

    //=============================================================================================
    // Program state
    //=============================================================================================

    g.angle = 0;
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

    glClearColor(0.5f, 0.5f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0x00, 0x00);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);
    glUniform4f(g.uniformModelColor, 1, 1, 1, 1);

    // Set up projection:
    glUseProgram(g.program);
    Matrix4 projectionAndView = matrixMultiply(
        matrixMultiply(
            matrixMultiply(
                matrixRotationY(g.angle),
                matrixRotationX(45 * TO_RADIANS)),
            matrixTranslationF(0, -2, -15)),
        matrixPerspective(0.1f, 90.0f * TO_RADIANS));
    glUniformMatrix4fv(g.uniformProjection, 1, GL_TRUE, projectionAndView.e);

    // Draw cube:
    Matrix4 cubeTransform = matrixMultiply(
        matrixScaleF(0.4f, 0.05f, 0.4f),
        matrixTranslationF(0, 0.05f, 0));
    glUniformMatrix4fv(g.uniformModelTransform, 1, GL_TRUE, cubeTransform.e);
    glBindVertexArray(g.cube.vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)g.cube.primitiveCount, GL_UNSIGNED_SHORT, 0);

    // Draw board:
    glBindVertexArray(g.plane.vao);
    glStencilFunc(GL_ALWAYS, 0xFF, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glDepthMask(GL_FALSE);
    for (int gy = 0; gy < BOARD_SIZE; gy++)
    {
        for (int gx = 0; gx < BOARD_SIZE; gx++)
        {
            bool isRed = (gx ^ gy) & 1;
            float w = 0.1f;
            float red[] = { 1, w, w, 1 };
            float black[] = { w, w, w, 1 };

            Matrix4 modelTransform = matrixMultiply(
                matrixScaleUniform(0.5f),
                matrixTranslationF(gx - BOARD_SIZE / 2 + 0.5f, 0, gy - BOARD_SIZE / 2 + 0.5f));
            glUniformMatrix4fv(g.uniformModelTransform, 1, GL_TRUE, modelTransform.e);
            glUniform4fv(g.uniformModelColor, 1, isRed ? red : black);
            glDrawElements(GL_TRIANGLES, (GLsizei)g.plane.primitiveCount, GL_UNSIGNED_SHORT, 0);
        }
    }
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glDepthMask(GL_TRUE);

    // Draw reflected cube:
    glStencilFunc(GL_NOTEQUAL, 0x00, 0xFF);
    cubeTransform = matrixMultiply(
        cubeTransform,
        matrixScaleF(1, -1, 1));
    glUniformMatrix4fv(g.uniformModelTransform, 1, GL_TRUE, cubeTransform.e);
    glUniform4f(g.uniformModelColor, 0.3f, 0.3f, 0.3f, 1.0f);
    glBindVertexArray(g.cube.vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)g.cube.primitiveCount, GL_UNSIGNED_SHORT, 0);
}
