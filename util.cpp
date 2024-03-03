#include "util.hpp"
#include <array>
#include <fstream>
#include <iostream>

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

void printGlError() {
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
