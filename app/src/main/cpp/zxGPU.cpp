//
// Created by Sergey on 06.01.2020.
//

#include "zxCommon.h"
#include "zxGPU.h"

static GLuint createShader(GLenum type, const char* text) {
    GLint compileStatus;
    auto shaderId = glCreateShader(type);
    if (shaderId == 0) return 0;
    glShaderSource(shaderId, 1, &text, nullptr);
    glCompileShader(shaderId);
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);
    if(compileStatus == 0) {
        glDeleteShader(shaderId);
        return 0;
    }
    return shaderId;
}

static GLuint createProgram(GLuint vShader, GLuint fShader) {
    GLint linkStatus;
    auto programId = glCreateProgram();
    if (programId == 0) return 0;
    glAttachShader(programId, vShader);
    glAttachShader(programId, fShader);
    glLinkProgram(programId);
    glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);
    if(linkStatus == 0) {
        glDeleteProgram(programId);
        return 0;
    }
    return programId;
}

zxGPU::zxGPU() : frameHeight(0), frameWidth(0), texture(0) {
    frameBuffer = new uint32_t[320 * 256];
}

zxGPU::~zxGPU() {
    SAFE_A_DELETE(frameBuffer);
}

void zxGPU::initGL() {
    static float vertices[] = {
            -1.0f,  1.0f, 1.0f,   0.0f, 0.0f,
            -1.0f, -1.0f, 1.0f,   0.0f, 1.0f,
            1.0f,   1.0f, 1.0f,   1.0f, 0.0f,
            1.0f,  -1.0f, 1.0f,   1.0f, 1.0f
    };
    static const char* vertShader = "attribute vec4 a_Position;\n"
                                    "attribute vec2 a_Texture;\n"
                                    "varying   vec2 v_Texture;\n\n"
                                    "void main() {\n"
                                    "    gl_Position = a_Position;\n"
                                    "    v_Texture = a_Texture;\n}";
    ;
    static const char* fragShader = "precision mediump float;\n\n"
                                    "uniform sampler2D u_TextureUnit;\n"
                                    "varying vec2 v_Texture;\n\n"
                                    "void main() {\n"
                                    "    gl_FragColor = texture2D(u_TextureUnit, v_Texture);\n}";

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // шейдеры
    auto vShader  = createShader(GL_VERTEX_SHADER, vertShader);
    auto fShader  = createShader(GL_FRAGMENT_SHADER, fragShader);
    auto pid      = createProgram(vShader, fShader);
    glUseProgram(pid);
    // параметры шейдеров
    auto aPositionLocation    = (GLuint)glGetAttribLocation(pid, "a_Position");
    auto aTextureLocation     = (GLuint)glGetAttribLocation(pid, "a_Texture");
    auto uTextureUnitLocation = (GLuint)glGetUniformLocation(pid, "u_TextureUnit");
    // вершины
    glVertexAttribPointer(aPositionLocation, 3, GL_FLOAT, 0, 20, &vertices[0]);
    glEnableVertexAttribArray(aPositionLocation);
    glVertexAttribPointer(aTextureLocation, 2, GL_FLOAT, 0, 20, &vertices[3]);
    glEnableVertexAttribArray(aTextureLocation);
    // текстура
    glGenTextures(1, &texture);
    if(texture) {
        // юнит текстуры
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(uTextureUnitLocation, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void zxGPU::updateProps(uint32_t sizeBorder, int filter) {
    frameWidth = sizeBorder * 2 + 256;
    frameHeight = sizeBorder * 2 + 192;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    // настройка объекта текстуры
    filter = filter != 0 ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frameWidth, frameHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

void zxGPU::updateFrame() {
    if(frameBuffer) {
//        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frameWidth, frameHeight, GL_RGBA, GL_UNSIGNED_BYTE, frameBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frameWidth, frameHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, frameBuffer);
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void zxGPU::uninitGL() {

}
