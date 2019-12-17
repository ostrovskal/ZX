# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.4.1)

add_library( # Sets the name of the library.
        zx-lib

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        zx-lib.cpp
        zxCommon.cpp
        zxALU.cpp
        zxCPU.cpp
        zxFile.cpp
        zxDA.cpp
        zxDebugger.cpp
        zxAssembler.cpp)

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

find_library( # Sets the name of the path variable.
        bmp-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        jnigraphics)

find_library( # Sets the name of the path variable.
        a-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        android)

target_link_libraries( # Specifies the target library.
        zx-lib

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib}
        ${bmp-lib}
        ${a-lib})