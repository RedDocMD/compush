#include "util.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

struct vectors {
    std::vector<double> vec;
    size_t dim;
    size_t cnt;

    vectors(std::vector<double> vec, size_t dim, size_t cnt)
        : vec(std::move(vec)), dim(dim), cnt(cnt) {}
};

static vectors parse_vectors(const std::string &filename) {
    std::ifstream inp(filename);
    if (inp.fail())
        throw std::runtime_error("Failed to open " + filename);

    char magic[6] = {0};
    char exp_magic[] = "\x93NUMPY";
    if (inp.read(magic, sizeof(magic)).fail())
        throw std::runtime_error("Failed to read magic from " + filename);
    if (memcmp(magic, exp_magic, sizeof(magic)) != 0)
        throw std::runtime_error("Magic bytes not matching");

    uint8_t version[2];
    if (inp.read(reinterpret_cast<char *>(version), sizeof(version)).fail())
        throw std::runtime_error("Failed to read version from " + filename);

    size_t size_len = version[0] == 1 ? 2 : 4;
    union {
        uint32_t v2;
        uint16_t v1;
    } read_size = {0};
    if (inp.read(reinterpret_cast<char *>(&read_size), size_len).fail())
        throw std::runtime_error("Failed to read size from " + filename);
    auto size = *reinterpret_cast<uint32_t *>(&read_size);

    std::string header(size, '\0');
    if (inp.read(header.data(), size).fail())
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
            .fail()) {
        throw std::runtime_error("Failed to read vector from " + filename);
    }

    return vectors(std::move(vecs), vector_dim, vector_cnt);
}

static GLuint make_texture(GLuint width, GLuint height) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTextureStorage2D(tex, 1, GL_RG32F, width, height);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

static GLuint vectors_to_texture(const std::vector<double> &vec, size_t vec_dim,
                                 size_t vec_cnt, GLuint loc) {
    GLuint tex_width = vec_dim;
    GLuint tex_height = vec_cnt;
    auto tex = make_texture(vec_dim, vec_cnt);
    glTextureSubImage2D(tex, 0, 0, 0, tex_width, tex_height, GL_RG, GL_FLOAT,
                        vec.data());
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

static inline __attribute__((always_inline)) GLint
get_uniform_location(GLint program, const std::string &name) {
    auto loc = glGetUniformLocation(program, name.c_str());
    if (loc == -1)
        throw std::runtime_error("failed to find location of uniform: " + name);
    handleGlError();
    return loc;
}

int main(int argc, char **argv) {
    std::string data_file = "../data.npy";
    std::string query_file = "../queries.npy";

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

    auto data = parse_vectors(data_file);
    auto query = parse_vectors(query_file);
    if (data.dim != query.dim)
        throw std::runtime_error("Data and query vecs don't match dimensions");

    split_double(data.vec);
    split_double(query.vec);

    auto knn_shader = loadShader("../knn.glsl", GL_COMPUTE_SHADER);
    std::vector<GLuint> shaders{knn_shader};
    auto program = createProgram(shaders);

    glUseProgram(program);
    auto data_loc = get_uniform_location(program, "data");
    auto query_loc = get_uniform_location(program, "queries");
    auto dist_loc = get_uniform_location(program, "dist");

    auto data_tex = vectors_to_texture(data.vec, data.dim, data.cnt, data_loc);
    auto query_tex =
        vectors_to_texture(query.vec, query.dim, query.cnt, query_loc);
    auto dist_tex = make_texture(query.cnt, data.cnt);
    glBindImageTexture(dist_loc, dist_tex, 0, GL_FALSE, 0, GL_READ_WRITE,
                       GL_RG32F);

    glDispatchCompute(query.cnt, data.cnt, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    std::vector<double> dist(data.cnt * query.cnt, -1.0f);
    glGetTextureImage(dist_tex, 0, GL_RG, GL_FLOAT,
                      dist.size() * sizeof(double), dist.data());
    handleGlError();
    join_double(dist);
    for (auto d : dist)
        std::cout << d << " ";
    std::cout << "\n";

    return 0;
}
