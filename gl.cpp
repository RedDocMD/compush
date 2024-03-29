#include "gl.hpp"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <array>
#include <cstdio>
#include <fcntl.h>
#include <gbm.h>
#include <iostream>
#include <string>
#include <unistd.h>

#if !HEADLESS

int gl_init_glfw() {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1, 1, "", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
    printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));

    glewExperimental = GL_TRUE;
    glewInit();

    return 0;
}

#else

int gl_init_headless(bool es) {
    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(display, nullptr, nullptr)) {
        fprintf(stderr, "failed to egl initialize\n");
        return 1;
    }
    std::string egl_extensions_st = eglQueryString(display, EGL_EXTENSIONS);
    if (egl_extensions_st.find("EGL_KHR_create_context") == std::string::npos) {
        fprintf(stderr, "EGL_KHR_create_context not found\n");
        return 1;
    }
    if (egl_extensions_st.find("EGL_KHR_surfaceless_context") ==
        std::string::npos) {
        fprintf(stderr, "EGL_KHR_surfaceless_context not found\n");
        return 1;
    }

    auto config_bit = es ? EGL_OPENGL_ES_BIT : EGL_OPENGL_BIT;
    static std::array<EGLint, 3> config_attribs = {EGL_RENDERABLE_TYPE,
                                                   config_bit, EGL_NONE};
    EGLConfig cfg;
    EGLint count;

    if (!eglChooseConfig(display, config_attribs.data(), &cfg, 1, &count)) {
        fprintf(stderr, "eglChooseConfig failed\n");
        return 1;
    }

    auto api = es ? EGL_OPENGL_ES_API : EGL_OPENGL_API;
    if (!eglBindAPI(api)) {
        fprintf(stderr, "eglBindAPI failed\n");
        return 1;
    }

    EGLint major = es ? 3 : 4;
    EGLint minor = es ? 2 : 3;
    static std::array<EGLint, 5> attribs = {EGL_CONTEXT_MAJOR_VERSION, major,
                                            EGL_CONTEXT_MINOR_VERSION, minor,
                                            EGL_NONE};
    auto context =
        eglCreateContext(display, cfg, EGL_NO_CONTEXT, attribs.data());
    if (context == EGL_NO_CONTEXT) {
        fprintf(stderr, "failed to create egl context\n");
        return 1;
    }

    if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context)) {
        fprintf(stderr, "failed to make egl context current\n");
        return 1;
    }
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
    printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));

    glewExperimental = GL_TRUE;
    auto err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "failed to glewInit: %s\n", glewGetErrorString(err));
        return 1;
    }

    return 0;
}

#endif
