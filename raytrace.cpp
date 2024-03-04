#include "util.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <png.h>
#include <stdexcept>

constexpr int img_width = 512;
constexpr int img_height = 512;

static void writeToPng(const std::string &filename,
                       const std::vector<GLubyte> &data) {
    png_image img;
    memset(&img, 0, sizeof(img));
    img.version = PNG_IMAGE_VERSION;
    img.width = img_width;
    img.height = img_height;
    img.format = PNG_FORMAT_RGBA;
    img.colormap_entries = 0;

    png_image_write_to_file(&img, filename.c_str(), false, data.data(),
                            img_width * 4, nullptr);
    auto err_mask = img.warning_or_error & 0x3;
    if (err_mask != 0)
        throw std::runtime_error(img.message);
}

int main(int argc, char **argv) {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    // glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    GLFWwindow *window = glfwCreateWindow(640, 480, "Raytracer", NULL, NULL);
    if (!window) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    int tex_w = img_width, tex_h = img_height;
    GLuint tex_output;
    glGenTextures(1, &tex_output);
    glBindTexture(GL_TEXTURE_2D, tex_output);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureStorage2D(tex_output, 1, GL_RGBA8UI, tex_w, tex_h);
    glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_READ_WRITE,
                       GL_RGBA8UI);

    int work_grp_cnt[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
    printf("max global (total) work group counts x:%i y:%i z:%i\n",
           work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

    int work_grp_size[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
    printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n",
           work_grp_size[0], work_grp_size[1], work_grp_size[2]);

    int work_grp_inv;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
    printf("max local work group invocations %i\n", work_grp_inv);

    auto ray_shader = loadShader("../raytrace.glsl", GL_COMPUTE_SHADER);
    std::vector<GLuint> shaders{ray_shader};
    auto program = createProgram(shaders);

    glUseProgram(program);
    glDispatchCompute(static_cast<GLuint>(tex_w), static_cast<GLuint>(tex_h),
                      1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    std::vector<GLubyte> data(4 * tex_w * tex_h, 125);
    glGetTextureImage(tex_output, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE,
                      data.size(), data.data());
    printGlError();

    std::string outPng("out.png");
    writeToPng("out.png", data);
    std::cout << "Wrote out to " << outPng << "\n";

    glfwTerminate();
}
