#include "util.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

static std::vector<double> parse_vectors(const std::string &filename) {
    std::ifstream inp(filename);

    char magic[6];
    char exp_magic[] = "\x93NUMPY";
    if (inp.read(magic, sizeof(magic)).bad())
        throw std::runtime_error("Failed to read magic from " + filename);
    if (memcmp(magic, exp_magic, sizeof(magic)) != 0)
        throw std::runtime_error("Magic bytes not matching");

    uint8_t version[2];
    if (inp.read(reinterpret_cast<char *>(version), sizeof(version)).bad())
        throw std::runtime_error("Failed to read version from " + filename);

    size_t size_len = version[0] == 1 ? 2 : 4;
    union {
        uint32_t v2;
        uint16_t v1;
    } read_size = {0};
    if (inp.read(reinterpret_cast<char *>(&read_size), size_len).bad())
        throw std::runtime_error("Failed to read size from " + filename);
    auto size = *reinterpret_cast<uint32_t *>(&read_size);

    std::string header(size, '\0');
    if (inp.read(header.data(), size).bad())
        throw std::runtime_error("Failed to read header from " + filename);

    auto shape_pos = header.find("shape");
    auto shape_val_pos = shape_pos + 8;
    auto shape_val_end = header.find_first_of(")", shape_val_pos);
    auto shape_val =
        header.substr(shape_val_pos + 1, shape_val_end - shape_val_pos - 1);
    auto comma_pos = shape_val.find(",");

    int vector_cnt = std::stoi(shape_val.substr(0, comma_pos));
    int vector_dim = std::stoi(shape_val.substr(comma_pos + 2));

    std::vector<double> vecs(vector_cnt * vector_dim);
    if (inp.read(reinterpret_cast<char *>(vecs.data()),
                 vecs.size() * sizeof(double))
            .bad()) {
        throw std::runtime_error("Failed to read vector from " + filename);
    }

    return vecs;
}

static GLuint vectors_to_texture(const std::vector<double> &vec, size_t vec_cnt,
                                 GLuint loc) {
    GLuint tex_width = vec.size();
    GLuint tex_height = vec_cnt;
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTextureStorage2D(tex, 1, GL_RG32F, tex_width, tex_height);
    glTexSubImage2D(tex, 0, 0, 0, tex_width, tex_height, GL_RG, GL_FLOAT,
                    vec.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindImageTexture(loc, tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F);
    return tex;
}

static void split_double(std::vector<double> &vec) {
    constexpr double splitter = (1 << 29) + 1;
    for (double &a : vec) {
        double t = a * splitter;
        float t_hi = t - (t - a);
        float t_lo = a - t_hi;
        memcpy(&a, &t_lo, sizeof(t_lo));
        memcpy(reinterpret_cast<char *>(&a) + sizeof(float), &t_hi,
               sizeof(t_hi));
    }
}

static void join_double(std::vector<double> &vec) {
    for (double &a : vec) {
        float t_lo, t_hi;
        memcpy(&t_lo, &a, sizeof(t_lo));
        memcpy(&t_hi, reinterpret_cast<char *>(&a) + sizeof(float),
               sizeof(t_hi));
        a = static_cast<double>(t_lo) + static_cast<double>(t_hi);
    }
}

int main(int argc, char **argv) {
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

    glewExperimental = GL_TRUE;
    glewInit();

    auto vecs = parse_vectors("../arr.npy");
    split_double(vecs);
    return 0;
}
