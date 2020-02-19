//
// Created by Sergey on 06.01.2020.
//

#pragma once

class zxGPU {
    friend class zxSpeccy;
public:
    zxGPU() : frameHeight(0), frameWidth(0), texture(0) { }
    ~zxGPU();

    void initGL();

    void updateProps(uint32_t sizeBorder, int filter);

    void updateFrame();

    void uninitGL();

    // буфер кадра
    static uint32_t* frameBuffer;
protected:

    // габариты текстуры
    GLsizei frameWidth, frameHeight;

    // ИД текстуры
    GLuint texture;
};
