#include "../src/deepzoom.h"
#include <iostream>

#if __cplusplus < 201703L // C++ less than 17
#include <experimental/filesystem>
	namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
	namespace fs = std::filesystem;
#endif


using namespace std;

int main(int argc, char *argv[])
{
	if(argc != 3) {
		cerr << "Usage: " << argv[0] << " <input.jpg> <output folder>" << endl;
		return 0;
	}

	fs::path path(argv[2]);

/*	if(!fs::exists(path.parent_path())) {
		cerr << "Output parent folder: '" << path.parent_path().c_str() << "' does not exists" << endl;
		return -1;
	}*/
	DeepZoom deep;
	if(!deep.build(argv[1], argv[2], 254, 1)) {
		cerr << "Something failed!" << endl;
	}
	return 0;
}
