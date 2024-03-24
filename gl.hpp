#pragma once

#include <GL/glew.h>
#define HEADLESS 1

#if HEADLESS

#include <GL/gl.h>

int gl_init_headless();
static inline __attribute__((always_inline)) int gl_init() {
    return gl_init_headless();
}

#else

#include <GLFW/glfw3.h>

int gl_init_glfw();
static inline __attribute__((always_inline)) int gl_init() {
    return gl_init_glfw();
}

#endif
