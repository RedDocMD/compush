#pragma once

// #include <GL/glew.h>
#include "gl.hpp"
#include <string>
#include <vector>

std::string readFile(const std::string &name);
void printShaderInfoLog(GLuint shader_index);
GLuint loadShader(const std::string &name, GLuint shader_type);
void printProgramInfoLog(GLuint program);
GLuint createProgram(const std::vector<GLuint> &shaders);
void handleGlError();
