#include "gl.hpp"
#include "util.hpp"
#include <cstring>
#include <fstream>

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

int main() {
    if (gl_init(true))
        return 1;

    std::string data_file = "../data.npy";
    std::string query_file = "../queries.npy";

    auto data = parse_vectors(data_file);
    auto query = parse_vectors(query_file);
    if (data.dim != query.dim)
        throw std::runtime_error("Data and query vecs don't match dimensions");

    split_double(data.vec);
    split_double(query.vec);

    auto shader = loadShader("../estest.glsl", GL_COMPUTE_SHADER);
    std::vector<GLuint> shaders{shader};
    auto program = createProgram(shaders);
    glUseProgram(program);

    join_double(data.vec);
    join_double(query.vec);
    return 0;
}
