#include "../src/deepzoom.h"
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
	if(argc != 3) {
		cerr << "Usage: " << argv[0] << " <input.jpg> <output folder>" << endl;
		return 0;
	}

	DeepZoom deep;
	deep.build(argv[1], argv[2]);
	return 0;
}
