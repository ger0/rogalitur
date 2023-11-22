#ifndef RGL_MAP_HPP
#define RGL_MAP_HPP

#include "types_utils.hpp"
#include <vector>

struct Map {
    u32 width;
    u32 height;

    std::vector<byte> bytes;

    bool load_data(std::string filename);
    void iterate();
};

#endif // RGL_MAP_HPP
