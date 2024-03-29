#pragma once

// #include <GL/glew.h>
#define HEADLESS 1

#if HEADLESS

#include <GLES3/gl32.h>

int gl_init_headless(bool es);
static inline __attribute__((always_inline)) int gl_init(bool es = false) {
    return gl_init_headless(es);
}

#else

#include <GLFW/glfw3.h>

int gl_init_glfw();
static inline __attribute__((always_inline)) int gl_init() {
    return gl_init_glfw();
}

#endif
