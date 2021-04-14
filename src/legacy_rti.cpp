#include "legacy_rti.h"
#include "jpeg_decoder.h"
#include "jpeg_encoder.h"

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <assert.h>
#include <math.h>

using namespace std;

//DEBUG
template <class C> void dump(const char *header, std::vector<C> &data) {
	cout << header << " ";
	for(auto &c: data)
		cout << (float)c << " ";
	cout << endl;
}

bool skipComments(FILE *file, char head = '#') {
	char buffer[1024];
	while(1) {
		long pos = ftell(file);
		if(fgets(buffer, 1024, file) == nullptr)
			return false;
		if(buffer[0] != head) {
			fseek(file, pos, SEEK_SET);
			return true;
		}
	}
	return true;
}

bool getLine(FILE *file, string &str) {
	char buffer[1024];
	if(fgets(buffer, 1024, file) == nullptr)
		return false;
	buffer[1023] = 0;
	
	char *t = buffer;
	while(*t != '\n')
		str.push_back(*t++);
	return true;
}

bool getInteger(FILE *file, int &n) {
	char buffer[32];
	if(fgets(buffer, 32, file) == nullptr)
		return false;
	buffer[31] = 0;
	errno = 0;
	n = strtol(buffer, nullptr, 10);
	return errno == 0;
}

bool getFloats(FILE *file, vector<float> &a, unsigned int expected = 0) {
	char buffer[256];
	if(fgets(buffer, 256, file) == nullptr)
		return false;
	buffer[255] = '0';
	char *start = buffer;
	char *end = start;
	while(1) {
		float n = strtod(start, &end);
		if(errno != 0 || start == end)
			break;
		start = end;
		a.push_back(n);
	}
	if(expected != 0)
		return expected == a.size();
	return a.size() > 0;
}

bool getIntegers(FILE *file, vector<int> &a, unsigned int expected = 0) {
	char buffer[256];
	if(fgets(buffer, 256, file) == nullptr)
		return false;
	buffer[255] = '0';
	char *start = buffer;
	char *end = start;
	while(1) {
		int n = strtol(start, &end, 10);
		if(errno != 0 || start == end)
			break;
		start = end;
		a.push_back(n);
	}
	if(expected != 0)
		return expected == a.size();
	return a.size() > 0;
}

bool LRti::load(const char *filename) {
	FILE* file = fopen(filename, "rb");
	if (file == nullptr) {
		cerr << "Could not open file: " << filename << endl;
		return false;
	}
	
	string version;
	getLine(file, version);
	bool status = true;
	if(version.compare(0, 3, "PTM", 3) == 0)
		status = loadPTM(file);
	else  if(version.compare(0, 7, "#HSH1.2", 7) == 0)
		status = loadHSH(file);
	else {
		cerr << "Not a PTM or HSH file." << endl;
		return false;
	}
	
	fclose(file);
	
	return status;
}

bool LRti::loadPTM(FILE* file) {
	rewind(file);
	
	string version, format;
	
	if(!getLine(file, version))
		return false;
	
	if(!getLine(file, format))
		return false;
	
	/*	PTM_FORMAT_RGB
	PTM_FORMAT_LUM
	PTM_FORMAT_LRGB
	PTM_FORMAT_PTM_LUT
	PTM_FORMAT_PTM_C_LUT
	PTM_FORMAT_JPEG_RGB
	PTM_FORMAT_JPEG_LRGB
	PTM_FORMAT_JPEGLS_RGB
	PTM_FORMAT_JPEGLS_LRGB */
	
	bool compressed = false;
	if(format == "PTM_FORMAT_RGB") {
		type = PTM_RGB;
		data.resize(18);
		
	} else if (format == "PTM_FORMAT_LRGB") {
		type = PTM_LRGB;
		data.resize(9);
		
	} else if (format == "PTM_FORMAT_JPEG_RGB") {
		type = PTM_RGB, compressed = true;
		data.resize(18);
		
	} else if (format == "PTM_FORMAT_JPEG_LRGB") {
		type = PTM_LRGB, compressed = true;
		data.resize(9);
		
	} else {
		type = UNKNOWN;
		cerr << "Unsupported format: " << format << endl;
		return false;
	}
	
	if(!getInteger(file, width) ||
			!getInteger(file, height) ||
			!getFloats(file, scale, 6) ||
			!getFloats(file, bias, 6)) {
		cerr << "File format invalid\n";
		return false;
	}
	for(auto &b: bias)
		b /= 255.0;
	
	for(auto &d: data)
		d.resize(width*height);
	
	if(type != PTM_LRGB && type != PTM_RGB) {
		cerr << "Unsupported RGB (for now)" << endl;
		return false;
	}
	if(!compressed)
		decodeRAW(version, file);
	
	else
		decodeJPEG(file);
	
	return true;
}

bool LRti::decodeRAW(const string &version, FILE *file) {
	if(type == PTM_LRGB) {
		//stored as interleaved abcdefRGB before 1.2
		//stored as interleaved abcdef then planes RGB.
		bool ptm12 = (version == "PTM_1.2");
		int multiplexed = ptm12? 6 : 9;
		uint32_t line_size = width*multiplexed;
		
		vector<unsigned char> line(line_size);
		for(int y = 0; y < height; y++) {
			if(fread(line.data(), 1, line_size, file) != line_size)
				return false;
			int c = 0;
			for(int x = 0; x < width; x++)
				for(int k = 0; k < multiplexed; k++)
					data[k][x + y*width] = line[c++];
		}
		
		if(ptm12) {
			for(int y = 0; y < height; y++) {
				if(fread(line.data(), 1, width*3, file) != (uint32_t)width*3)
					return false;
				
				int c = 0;
				for(int x = 0; x < width; x++)
					for(int k = 6; k < 9; k++)
						data[k][x + y*width] = line[c++];
			}
		}
	} else {
		uint32_t line_size = width*6;
		vector<unsigned char> line(line_size);
		
		for(int k = 0; k < 3; k++) {
		
			for(int y = 0; y < height; y++) {
				if(fread(line.data(), 1, line_size, file) != line_size)
					return false;
				int c = 0;
				for(int x = 0; x < width; x++)
					for(int j = 0; j < 6; j++)
						data[j*3 + k][x + y*width] = line[c++];
			}
		}
	}
	return true;
}





bool LRti::loadHSH(FILE* file) {
	rewind(file);
	
	skipComments(file);
	
	int rti_type = 0;
	if(!getInteger(file, rti_type))
		return false;
	
	vector<int> tmp;
	if(!getIntegers(file, tmp, 3))
		return false;
	
	width = tmp[0];
	height = tmp[1];
	int ncomponents = tmp[2];
	if(ncomponents != 3) {
		cerr << "Unsupported components != 3" << endl;
		return false;
	}
	
	vector<int> basis;
	if(!getIntegers(file, basis, 3))
		return false;
	
	int basis_terms = basis[0]; //number of terms in the basis
/*	//ignored
	int basis_type = basis[1];
	int basis_size = basis[2];
*/
	
	if(rti_type != 3) {
		cerr << "Unsupported .rti if not HSH (for the moment)" << endl;
		return false;
	}
	type = HSH;
	
	data.resize(basis_terms*3);
	for(auto &a: data)
		a.resize(width*height);
	//now load HSH
	
	vector<float> gmax(basis_terms);
	vector<float> gmin(basis_terms);
	
	bias.resize(basis_terms);
	scale.resize(basis_terms);
	
	size_t read = fread(scale.data(), sizeof(float), basis_terms, file);  //max
	if(read != basis_terms*sizeof(float)) {
		cerr << "Failed reading basis." << endl;
		return false;
	}
		
	read = fread(bias.data(),  sizeof(float), basis_terms, file); //min
	if(read != basis_terms*sizeof(float)) {
		cerr << "Failed reading basis." << endl;
		return false;
	}

	//in OUR system we want to write (c - bias)*scale
	//in .rti system HSH its c*scale + bias
	//we need to convert the coefficients to the uniform standard.
	
	for(int i = 0; i < basis_terms; i++)
		bias[i] = -bias[i]/scale[i];

	uint32_t line_size = width * basis_terms * 3;
	vector<unsigned char> line(line_size);

	//for each pixel is 9 for red... 9 for green, 9 for blue
	for(int y = 0; y < height; y++)	{
		int Y = height -1 -y;
		if(fread(line.data(), 1, line_size, file) != line_size)
			return false;
		int c = 0; //line position;
		for(int x = 0; x < width; x++)
			for(int k = 0; k < 3; k++)
				for(int j = 0; j < basis_terms; j++)
					data[j*3 + k][(Y*width + x)] = line[c++];
	}
	return true;
}

void LRti::clip(int left, int bottom, int right, int top) {
	assert(left >= 0 && right > left && right  <= width);
	assert(bottom >= 0 && top > bottom && top  <= height);
	
	int w = right - left;
	int h = top - bottom;
	vector<vector<uint8_t>> tmp(9);
	for(auto &d: tmp)
		d.resize(w*h);
	
	for(int k = 0; k < 9; k++) {
		for(int y = bottom, dy = 0; y < top; y++, dy++) {
			memcpy(tmp[k].data() + dy*w, data[k].data() + y*width + left, w);
		}
	}
	for(int k = 0; k < 9; k++)
		swap(tmp[k], data[k]);
	width = w;
	height = h;
}


LRti LRti::clipped(int left, int bottom, int right, int top) {
	//NRO optimization avoid copying this object in return.
	LRti tmp;
	tmp.type = type;
	
	assert(left >= 0 && right > left && right  <= width);
	assert(bottom >= 0 && top > bottom && top  <= height);
	
	int w = tmp.width = right - left;
	int h = tmp.height = top - bottom;
	tmp.bias = bias;
	tmp.scale = scale;
	tmp.data.resize(data.size());
	for(auto &d: tmp.data)
		d.resize(w*h);
	
	for(int k = 0; k < 9; k++)
		for(int y = bottom, dy = 0; y < top; y++, dy++)
			memcpy(tmp.data[k].data() + dy*w, data[k].data() + y*width + left, w);
	return tmp;
}


template <class C> std::ostringstream &join(std::vector<C> &v, std::ostringstream &stream, const char *separator = " ") {
	for(size_t i = 0; i < v.size(); i++) {
		stream << v[i];
		if(i != v.size()-1)
			stream << separator;
	}
	return stream;
}

bool LRti::encode(PTMFormat format, int &size, uint8_t *&buffer, int quality) {
	std::string f;
	switch(format) {
	case RAW:
		if(type == PTM_LRGB) f = "PTM_FORMAT_LRGB";
		else if(type == PTM_RGB) f = "PTM_FORMAT_RGB";
		break;
	case JPEG:
		if(type == PTM_LRGB) f = "PTM_FORMAT_JPEG_LRGB";
		else if(type == PTM_RGB) f = "PTM_FORMAT_JPEG_RGB";
		break;
	default:
		break;
	}
	if(f.empty()) {
		cerr << "Unsupported or incompatible save format" << endl;
		return false;
	}
	
	int bsize = 0;  //binary size
	
	vector<uint8_t *> buffers(9, nullptr);
	vector<int> sizes(9, 0);
	
	switch(format) {
	case RAW:
		//no need to allocate buffer
		for(auto &d: data)
			bsize += d.size();
		break;
	case JPEG:
		if(!encodeJPEG(sizes, buffers, quality)) return false;
		for(auto &s: sizes)
			bsize += s;
		break;
	default:
		break;
	}
	
	std::ostringstream stream;
	stream << "PTM_1.2\n";
	stream << f << "\n";
	stream << width << "\n" << height << "\n";
	join(scale, stream) << "\n";
	for(auto &b: bias)
		b = floor(b*255.0 + 0.5);
	join(bias, stream) << "\n";
	
	switch(format) {
	case RAW:
		break;
	case JPEG:
		stream << quality << "\n";
		stream << "0 0 0 0 0 0 0 0 0\n";
		stream << "0 0 0 0 0 0 0 0 0\n";
		stream << "0 0 0 0 0 0 0 0 0\n";
		stream << "0 1 2 3 4 5 6 7 8\n";
		stream << "-1 -1 -1 -1 -1 -1 -1 -1 -1\n";
		join(sizes, stream) << "\n";
		stream << "0 0 0 0 0 0 0 0 0\n";
		break;
	default:
		break;
	}
	
	int pos = stream.str().size();
	size = bsize + pos;
	buffer = new uint8_t[size];
	
	memcpy(buffer, stream.str().c_str(), pos);
	
	uint8_t *start = buffer + pos;
	switch(format) {
	case RAW:
		if(type == PTM_LRGB) {
			for(int y = 0; y < height; y++) {
				for(int32_t x = 0; x < width; x++) {
					int32_t i = y*width + x;
					for(int k = 0; k < 6; k++)
						start[i*6 + k] = data[k][i];
				}
			}

			start += height*width*6;
			for(int y = 0; y < height; y++) {
				for(int32_t x = 0; x < width; x++) {
					int32_t i = y*width + x;
					for(int k = 0; k < 3; k++)
						start[i*3 + k] = data[6 + k][i];
				}
			}

		} else if(type == PTM_RGB) {
			//data is in groups of 6 coeffs for each component
			//
			for(int p = 0; p < 18; p += 6) {
				int o = p*width*height;
				for(int y = 0; y < height; y++) {
					for(int32_t x = 0; x < width; x++) {
						int32_t i = y*width + x;
						for(int k = 0; k < 6; k++)
							start[o + i*6 + k] = data[p + k][i];
					}
				}
			}

		} else {
			throw "Unimplemented";
		}
		break;
		
	case JPEG:
		for(int k = 0; k < 9; k++) {
			memcpy(start, buffers[k], sizes[k]);
			start += sizes[k];
		}
		break;
		
	default:
		break;
	}
	//cleanup temporary buffers
	for(uint8_t *m: buffers)
		delete []m;
	return true;
}

bool LRti::encode(PTMFormat format, const char *filename, int quality) {
	FILE *file = fopen(filename, "wb");
	if(!file) {
		cerr << "Could not open file: " << filename << endl;
		return false;
	}
	uint8_t *buffer = nullptr;
	int size = 0;
	bool ok  = encode(format, size, buffer, quality);
	if(!ok) return false;
	fwrite(buffer, 1, size, file);
	fclose(file);
	return true;
}

/* JPEG
 * write to file, we need to reverse the order of the Y */

bool LRti::encodeJPEGtoFile(int startplane, int quality, const char *filename) {
	vector<int> order;
	if(type == PTM_LRGB) {
		//lrgb ptm order is: x^2, y^2, xy, x, y, 1, r, g, b
		//while we use r, g, b, 1, x, y, x2 xy y2
		order = {6,7,8, 5,3,4, 0,2,1};
	} else if(type == PTM_RGB) {
		order = {5, 3, 4, 0, 2, 1};
	} else
		order = {0, 1, 2, 3, 4, 5, 6, 7, 8};
	JpegEncoder enc;
	enc.setQuality(quality);

	enc.setColorSpace(JCS_RGB, 3);
	
	//enc.setJpegColorSpace(JCS_YCbCr);
	enc.setJpegColorSpace(JCS_RGB);
	
	//lets avoid make another copy in memory.

	enc.init(filename, width, height);

	vector<uint8_t> line(width*3);
	for(int y = height-1; y >= 0; y--) {
		for(int32_t x = 0; x < width; x++) {
			int32_t p = y*width + x;
			if(type == PTM_LRGB) {
				line[x*3 + 0] = data[order[startplane + 0]][p];
				line[x*3 + 1] = data[order[startplane + 1]][p];
				line[x*3 + 2] = data[order[startplane + 2]][p];
			} else {
				line[x*3 + 0] = data[order[startplane/3]*3 +  0][p];
				line[x*3 + 1] = data[order[startplane/3]*3 +  1][p];
				line[x*3 + 2] = data[order[startplane/3]*3 +  2][p];
			}
		}
		enc.writeRows(line.data(), 1);
	}

	//return size, if needed.
	enc.finish();

	return true;
}

bool LRti::encodeJPEG(vector<int> &sizes, vector<uint8_t *> &buffers, int quality) {
	for(int k = 0; k < 9; k++) {
		JpegEncoder enc;
		enc.setQuality(quality);
		enc.setColorSpace(JCS_GRAYSCALE, 1);
		enc.setJpegColorSpace(JCS_GRAYSCALE);
		
		//needs to reverse the image!
		//
		bool ok = enc.encode(data[k].data(), width, height, buffers[k], sizes[k]);
		if(!ok) {
			for(int i = 0; i < k; i++)
				delete []buffers[i];
			return false;
		}
				std::ostringstream filename;
		 filename << "coeff_" << k << ".jpg";
		 
		FILE *file = fopen(filename.str().c_str(), "wb");
		fwrite(buffers[k], 1, sizes[k], file);
		fclose(file);
	}
	return true;
}

bool LRti::decodeJPEG(FILE *file) {
	int quality;
	
	vector<int> transform;
	vector<int> motionx;
	vector<int> motiony;
	vector<int> order; //order[0] = 1 means it will be the second plane to be decoded
	vector<int> reference; //again integers
	vector<int> sizes;
	vector<int> overflows;
	
	unsigned int ncoeffs = 9;
	switch(type) {
	case PTM_LRGB: ncoeffs = 9; break;
	case PTM_RGB: ncoeffs = 18; break;
	default: cerr << "Not supported!\n"; exit(0);
	}
	
	if(!getInteger(file, quality) ||
			!getIntegers(file, transform, ncoeffs) ||
			!getIntegers(file, motionx, ncoeffs) ||
			!getIntegers(file, motiony, ncoeffs) ||
			!getIntegers(file, order, ncoeffs) ||
			!getIntegers(file, reference, ncoeffs) ||
			!getIntegers(file, sizes, ncoeffs) ||
			!getIntegers(file, overflows, ncoeffs)) {
		cerr << "File format invalid\n";
		return false;
	}
	//check transform and motions are 0
	for(unsigned int k = 0; k < ncoeffs; k++) {
		if(transform[k] != 0 || motionx[k] != 0 || motiony[k] != 0) {
			cerr << "Transform and motion array unsupported." << endl;
			return false;
		}
	}
	//find in which order to decode them
	vector<unsigned int> sequence(order.size());
	for(unsigned int i = 0; i < order.size(); i++)
		sequence[(unsigned int)(order[i])] = i;
	//		sequence[order[i]-1] = i; ???
	
	//precompute start of planes
	vector<int> pos(ncoeffs+1);
	pos[0] = int(ftell(file));
	for(unsigned int k = 1; k < ncoeffs+1; k++) {
		pos[k] = pos[k-1] + sizes[k-1] + overflows[k-1];
	}
	
	//check size
	fseek(file, 0L, SEEK_END);
	unsigned int tot_size = ftell(file);
	if(tot_size < (unsigned int)(pos[ncoeffs])) {
		cerr << "File is truncated." << endl;
		return false;
	}
	
	for(unsigned int k = 0; k < ncoeffs; k++) {
		unsigned int s = sequence[k];
		int r = reference[s];
		
		//read jpeg
		fseek(file, pos[s], SEEK_SET);
		vector<uint8_t> buffer(sizes[s]);
		int readed = fread(buffer.data(), 1, size_t(sizes[s]), file);
		if(readed != sizes[s])
			return false;
		
		bool decoded = decodeJPEG(buffer.size(), buffer.data(), s);

		if(!decoded)
			return false;
		int w = width;
		int h = height;
		if(r != -1) {
			for(int i = 0; i < w*h; i++) {
				data[s][i] = data[s][i] - 128 + data[r][i];
			}
		}
		
		if(overflows[s] > 0) {
			vector<uint8_t> overs(overflows[s]);
			fread(overs.data(), 1, overs.size(), file);
			for(int i = 0; i < overflows[s]; i += 5) {
				//I wonder why the position is stored big endian.
				int p = overs[i]*256*256*256 + overs[i+1]*256*256 + overs[i+2]*256 + overs[i+3];
				assert(p < w*h);
				data[s][p] = overs[i+4];
			}
		}
	}
	
	return true;
}

bool LRti::decodeJPEG(size_t size, unsigned char *buffer, unsigned int plane) {
	uint8_t *img = nullptr;
	int w, h;
	JpegDecoder dec;
	dec.setColorSpace(JCS_GRAYSCALE);
	dec.setJpegColorSpace(JCS_GRAYSCALE);

	if(!dec.decode(buffer, size, img, w, h) || w != width || h != height) {
		cerr << "Failed decoding jpeg or different size." << endl;
		return false;
	}

	chromasubsampled = dec.chromaSubsampled();
	data[plane].resize(int(w*h));
	memcpy(data[plane].data(), img, int(w*h));
	delete []img;
	return true;
}

bool LRti::decodeJPEGfromFile(size_t size, unsigned char *buffer, unsigned int plane0, unsigned int plane1, unsigned int plane2) {
	vector<unsigned int> invorder;
	if(type == PTM_LRGB) {
		invorder = {6,7,8, 5,3,4, 0,2,1};

	} else if(type == PTM_RGB) {
		invorder = { 5,11,17,  3,9,15,  4,10,16,  0,6,12,  2,8,14, 1,7,13 };
	} else {
		invorder = {0, 1, 2, 3, 4, 5, 6, 7, 8};
	}


	uint8_t *img = nullptr;
	int w, h;
	JpegDecoder dec;
	if(!dec.decode(buffer, size, img, w, h) || w != width || h != height) {
		cerr << "Failed decoding jpeg or different size." << endl;
		return false;
	}
	plane0 = invorder[plane0];
	plane1 = invorder[plane1];
	plane2 = invorder[plane2];

	chromasubsampled = dec.chromaSubsampled();
	data[plane0].resize(int(w*h));
	data[plane1].resize(int(w*h));
	data[plane2].resize(int(w*h));


	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			unsigned int i = int(x + y*width);
			unsigned int j = int(x + (height - 1 -y)*width);

			data[plane0][j] = img[i*3+0];
			data[plane1][j] = img[i*3+1];
			data[plane2][j] = img[i*3+2];
		}
	}

	delete []img;
	return true;

}
