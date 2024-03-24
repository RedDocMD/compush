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

int gl_init_headless() {
    // auto fd = open("/dev/dri/renderD128", O_RDWR);
    // if (fd < 0) {
    //     perror("Opening /dev/dri/renderD128");
    //     return 1;
    // }
    // auto *gbm = gbm_create_device(fd);
    // if (!gbm) {
    //     fprintf(stderr, "Failed to create gdm device\n");
    //     return 1;
    // }
    // auto egl_dpy = eglGetPlatformDisplay(EGL_PLATFORM_GBM_MESA, gbm,
    // nullptr); if (!egl_dpy) {
    //     fprintf(stderr, "Failed to get egl platform display\n");
    //     return 1;
    // }
    auto egl_dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(egl_dpy, nullptr, nullptr)) {
        fprintf(stderr, "failed to egl initialize\n");
        return 1;
    }
    std::string egl_extensions_st = eglQueryString(egl_dpy, EGL_EXTENSIONS);
    if (egl_extensions_st.find("EGL_KHR_create_context") == std::string::npos) {
        fprintf(stderr, "EGL_KHR_create_context not found\n");
        return 1;
    }
    if (egl_extensions_st.find("EGL_KHR_surfaceless_context") ==
        std::string::npos) {
        fprintf(stderr, "EGL_KHR_surfaceless_context not found\n");
        return 1;
    }

    static std::array<EGLint, 3> config_attribs = {EGL_RENDERABLE_TYPE,
                                                   EGL_OPENGL_BIT, EGL_NONE};
    EGLConfig cfg;
    EGLint count;

    if (!eglChooseConfig(egl_dpy, config_attribs.data(), &cfg, 1, &count)) {
        fprintf(stderr, "eglChooseConfig failed\n");
        return 1;
    }
    if (!eglBindAPI(EGL_OPENGL_API)) {
        fprintf(stderr, "eglBindAPI failed\n");
        return 1;
    }

    static std::array<EGLint, 5> attribs = {
        EGL_CONTEXT_MAJOR_VERSION, 4, EGL_CONTEXT_MINOR_VERSION, 3, EGL_NONE};
    auto core_ctx =
        eglCreateContext(egl_dpy, cfg, EGL_NO_CONTEXT, attribs.data());
    if (core_ctx == EGL_NO_CONTEXT) {
        fprintf(stderr, "failed to create egl context\n");
        return 1;
    }

    if (!eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, core_ctx)) {
        fprintf(stderr, "failed to make egl context current\n");
        return 1;
    }
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
    printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));

    glewExperimental = GL_TRUE;
    glewInit();

    return 0;
}

#endif
