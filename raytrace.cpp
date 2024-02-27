#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <png.h>
#include <stdexcept>
#include <string>
#include <vector>

constexpr int img_width = 512;
constexpr int img_height = 512;

std::string readFile(const std::string &name) {
    std::ifstream infile(name, std::ios::in | std::ios::ate);
    size_t size = infile.tellg();
    std::string inp(size, '\0');
    infile.seekg(0);
    if (!infile.read(inp.data(), size))
        throw std::runtime_error("File reading failed: " + name);
    return inp;
}

void printShaderInfoLog(GLuint shader_index) {
    GLint size;
    glGetShaderiv(shader_index, GL_INFO_LOG_LENGTH, &size);
    std::string info(size, '\0');
    int actual_length;
    glGetShaderInfoLog(shader_index, size, &actual_length, info.data());
    printf("shader info log for GL index %u:\n%s\n", shader_index,
           info.c_str());
}

GLuint loadShader(const std::string &name, GLuint shader_type) {
    auto data = readFile(name);
    auto shader_idx = glCreateShader(shader_type);
    std::array<const char *, 1> dataArr{data.data()};
    glShaderSource(shader_idx, 1, dataArr.data(), NULL);
    glCompileShader(shader_idx);

    int params = -1;
    glGetShaderiv(shader_idx, GL_COMPILE_STATUS, &params);
    if (GL_TRUE != params) {
        fprintf(stderr, "ERROR: GL shader index %i did not compile\n",
                shader_idx);
        printShaderInfoLog(shader_idx);
        throw std::runtime_error("shader compilation failed: " + name);
    }

    return shader_idx;
}

void printProgramInfoLog(GLuint program) {
    GLint size;
    glGetShaderiv(program, GL_INFO_LOG_LENGTH, &size);
    std::string info(size, '\0');
    int actual_length;
    glGetProgramInfoLog(program, size, &actual_length, info.data());
    printf("program info log for GL index %u:\n%s", program, info.c_str());
}

GLuint createProgram(const std::vector<GLuint> &shaders) {
    GLuint program = glCreateProgram();
    for (auto shader : shaders)
        glAttachShader(program, shader);
    glLinkProgram(program);

    int params = -1;
    glGetProgramiv(program, GL_LINK_STATUS, &params);
    if (GL_TRUE != params) {
        fprintf(stderr, "ERROR: could not link shader programme GL index %u\n",
                program);
        printProgramInfoLog(program);
        throw std::runtime_error("program creation failed");
    }

    return program;
}

void writeToPng(const std::string &filename, const std::vector<float> &data) {
    png_image img;
    memset(&img, 0, sizeof(img));
    img.version = PNG_IMAGE_VERSION;
    img.width = img_width;
    img.height = img_height;
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
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_output);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_w, tex_h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, NULL);
    glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA);

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

    auto ray_shader = loadShader("raytrace.glsl", GL_COMPUTE_SHADER);
    std::vector<GLuint> shaders{ray_shader};
    auto program = createProgram(shaders);

    glUseProgram(program);
    glDispatchCompute((GLuint)tex_w, (GLuint)tex_h, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    std::vector<GLubyte> data(3 * tex_w * tex_h);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());

    for (auto b : data)
        std::cout << static_cast<int>(b) << " ";
    std::cout << "\n";

    glfwTerminate();
}
