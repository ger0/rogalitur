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
	this->bytes.resize(size);
	size_t read_size = fread(this->bytes.data(), sizeof(byte), this->bytes.size(), f.get());
	if (read_size != size) return false;
	return true;
}
void Map::iterate() {
    LOG_DBG("Iterating over map's data");
    u32 i = 0;
    for (const byte& item : bytes) {
        printf("%u,", item / 255);
        i++;
        if (i % (width * 3) == 0) printf("\n");
    }
}
