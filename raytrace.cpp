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
    if (infile.fail())
        throw std::runtime_error("File opening failed: " + name);
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
        fprintf(stderr, "ERROR: could not link shader programm GL index %u\n",
                program);
        printProgramInfoLog(program);
        throw std::runtime_error("program creation failed");
    }

    return program;
}

void writeToPng(const std::string &filename, const std::vector<GLubyte> &data) {
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

static void printGlError() {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        switch (err) {
        case GL_INVALID_ENUM:
            std::cerr << "GL_INVALID_ENUM: An unacceptable value is specified "
                         "for an enumerated "
                         "argument. The offending command is ignored and has "
                         "no other side effect than to set the error flag\n";
            break;
        case GL_INVALID_VALUE:
            std::cerr << "GL_INVALID_VALUE: A numeric argument is out of "
                         "range. The offending command is ignored and has no "
                         "other side effect than to set the error flag\n";
            break;
        case GL_INVALID_OPERATION:
            std::cerr << "GL_INVALID_OPERATION: The specified operation is not "
                         "allowed in the current state. The offending command "
                         "is ignored and has no other side effect than to set "
                         "the error flag\n";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            std::cerr
                << "GL_INVALID_FRAMEBUFFER_OPERATION: The framebuffer object "
                   "is not complete. The offending command is ignored and has "
                   "no other side effect than to set the error flag\n";
            break;
        case GL_OUT_OF_MEMORY:
            std::cerr << "GL_OUT_OF_MEMORY: The framebuffer object is not "
                         "complete. The offending command is ignored and has "
                         "no other side effect than to set the error flag\n";
            break;
        case GL_STACK_UNDERFLOW:
            std::cerr << "GL_STACK_UNDERFLOW: An attempt has been made to "
                         "perform an operation that would cause an internal "
                         "stack to underflow\n";
            break;
        case GL_STACK_OVERFLOW:
            std::cerr
                << "GL_STACK_OVERFLOW: An attempt has been made to perform an "
                   "operation that would cause an internal stack to overflow\n";
            break;
        default:
            std::cerr << "Unknown error\n";
        }
        std::flush(std::cerr);
    }
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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
