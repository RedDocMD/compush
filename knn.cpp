#include "util.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

class DVector {
public:
    DVector(size_t len) : ptr_(std::make_unique<double[]>(len)), len_(len) {}

    const double *data() const { return ptr_.get(); }
    double *data() { return ptr_.get(); }
    size_t byte_len() const { return len_ * sizeof(double); }
    size_t len() const { return len_; }

private:
    std::unique_ptr<double[]> ptr_;
    size_t len_;
};

static std::vector<DVector> parse_vectors(const std::string &filename) {
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

    int vector_cnt = std::stoi(shape_val.substr(0, comma_pos - 1));
    int vector_dim = std::stoi(shape_val.substr(comma_pos + 2));

    std::vector<DVector> vecs;
    for (int i = 0; i < vector_cnt; i++) {
        DVector dv(vector_dim);
        if (inp.read(reinterpret_cast<char *>(dv.data()), dv.byte_len()).bad())
            throw std::runtime_error("Failed to read vector from " + filename);
        vecs.push_back(std::move(dv));
    }

    return vecs;
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
    return 0;
}
