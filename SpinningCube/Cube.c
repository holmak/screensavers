#include "Common.h"

static struct cubeGlobals
{
	bool started;
    
    GLuint program;
    GLuint uniformProjection;
    GLuint uniformModelTransform;

    float angle;
} g;

static void start()
{
    //=============================================================================================
    // Data
    //=============================================================================================

    BasicVertex vertexData[] =
    {
        { -1, -1, -1, 0x00, 0x00, 0x00, 0xFF, },
        { +1, -1, -1, 0xFF, 0x00, 0x00, 0xFF, },
        { -1, +1, -1, 0x00, 0xFF, 0x00, 0xFF, },
        { +1, +1, -1, 0xFF, 0xFF, 0x00, 0xFF, },
        { -1, -1, +1, 0x00, 0x00, 0xFF, 0xFF, },
        { +1, -1, +1, 0xFF, 0x00, 0xFF, 0xFF, },
        { -1, +1, +1, 0x00, 0xFF, 0xFF, 0xFF, },
        { +1, +1, +1, 0xFF, 0xFF, 0xFF, 0xFF, },
    };

    uint16_t indexData[] =
    {
        0, 1, 3, 0, 3, 2, // front
        1, 5, 7, 1, 7, 3, // right
        5, 4, 6, 5, 6, 7, // back
        4, 0, 2, 4, 2, 6, // left
        1, 0, 4, 1, 4, 5, // bottom
        2, 3, 7, 2, 7, 6, // top
    };

    char *vertexShaderSource = readTextFile("assets/shaders/cube.v.glsl");
    char *fragmentShaderSource = readTextFile("assets/shaders/cube.f.glsl");

    //=============================================================================================
    // GL resources
    //=============================================================================================

    g.program = compileShaderProgram(vertexShaderSource, fragmentShaderSource);
    g.uniformProjection = oglGetUniformLocation(g.program, "uniProjection");
    g.uniformModelTransform = oglGetUniformLocation(g.program, "uniModelTransform");

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

    g.angle = 0;
}

void screensaverCube()
{
	if (!g.started)
	{
		start();
		g.started = true;
	}

    g.angle += FRAME_TIME;
    g.angle = fmodf(g.angle, 2 * PI);

    oglClearColor(0.5f, 0.5f, 1.0f, 0.0f);
    oglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    oglUseProgram(g.program);

    Matrix4 projection = matrixPerspective(0.1f, 60.0f * TO_RADIANS);
    Matrix4 modelTransform = matrixMultiply(
        matrixMultiply(
            matrixRotationX(g.angle),
            matrixRotationY(2 * g.angle)),
        matrixTranslationF(0, 0, -6));

    oglUniformMatrix4fv(g.uniformProjection, 1, GL_TRUE, projection.e);
    oglUniformMatrix4fv(g.uniformModelTransform, 1, GL_TRUE, modelTransform.e);
    oglDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
}
