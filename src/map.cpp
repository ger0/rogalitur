#include "map.hpp"
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
    for (const u32& item : data) {
        printf("%lu,", item);
    }
}

// todo: remove 
void Map::load_from_sdl(SDL_Surface &surf) {
	SDL_LockSurface(&surf);
    defer {
        SDL_UnlockSurface(&surf);
    };
    byte size = surf.format->BytesPerPixel;
    data.resize(surf.w * surf.h);
    this->width = surf.w;
    this->height = surf.h;
	memcpy(data.data(), surf.pixels, width * height * size);
}
