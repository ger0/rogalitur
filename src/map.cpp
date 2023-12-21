#include "map.hpp"
#include <cstdint>
#include <memory>
#include <cstdlib>

static auto file_destroy = [](FILE* file) {
	fclose(file);
};

template<typename T>
bool save_file(const char* filename, std::vector<T>& buff) {
	std::unique_ptr<FILE, decltype(file_destroy)> f(fopen(filename, "wb"), file_destroy);
	if (f.get() == nullptr) return false;
	size_t write_size = fwrite(buff.data(), sizeof(T), buff.size(), f.get());
	if (write_size != buff.size()) return false;
	return true;
}

bool Map::load_data(std::string filename) {
	std::unique_ptr<FILE, decltype(file_destroy)> f(
		fopen(filename.c_str(), "rb"), file_destroy
	);
	if (f.get() == nullptr) return false;
	if (fseek(f.get(), 0, SEEK_END) != 0) return false;
	const size_t size = ftell(f.get());
	fseek(f.get(), 0, SEEK_SET);
	this->data.resize(size);
	size_t read_size = fread(this->data.data(), sizeof(byte), this->data.size(), f.get());
	if (read_size != size) return false;
	return true;
}
void Map::iterate() {
    LOG_DBG("Iterating over map's data");
    u64 i = 0;
    /* for (const u32& item : data) {
        printf("%1lu,", item / 0xFFFFFFFFFFFFFFFF);
    	i++;
        if (i % this->width == 0) printf("\n");
    } */
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
        	printf("%1lu,", at(Vec2i{x, y}) / 0xFFFFFFFFFFFFFFFF);
		}
        printf("\n");
	}
}

u32 Map::at(const Vec2i pos) const {
	if (pos.x <= 0 || pos.y <= 0
			|| pos.x >= width || pos.y >= height) return UINT32_MAX;
	return this->data[pos.x + pos.y * width];
}

// todo: remove 
Map::Map(SDL_Surface &surf): width(surf.w), height(surf.h) {
	SDL_LockSurface(&surf);
    defer {
        SDL_UnlockSurface(&surf);
    };
    byte size = surf.format->BytesPerPixel;
    data.resize(surf.w * surf.h);
	memcpy(data.data(), surf.pixels, width * height * size);
}
