//
// Created by Sergey on 06.01.2020.
//

#pragma once

class zxGPU {
    friend class zxALU;
public:
    zxGPU();
    ~zxGPU();

    void initGL();

    void updateProps(uint32_t sizeBorder, int filter);

    void updateFrame();

    void uninitGL();

protected:

    // габариты текстуры
    GLsizei frameWidth, frameHeight;

    // ИД текстуры
    GLuint texture;

    // буфер кадра
    uint32_t* frameBuffer;

};
