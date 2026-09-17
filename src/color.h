#ifndef COLOR_H
#define COLOR_H

#include <cstdint>
#include <stddef.h>


template <class T> struct Color3 {
	T r, g, b;
	Color3() {}
	Color3(T _r, T _g, T _b): r(_r), g(_g), b(_b) {}
	// Unsafe, maybe redefine color as a vector and have r(), g() and b() methods?
	T &operator[](int n) { return ((T *)this)[n]; }

	//This is actually worse than the approximate ycc below.
	Color3 RgbToYCbCr() {
		Color3 c;
		c.r =       0.299   * r + 0.587   * g + 0.114   * b;
		c.g = 0.5 - 0.16874 * r - 0.33126 * g + 0.5     * b;
		c.b = 0.5 + 0.50000 * r - 0.41869 * g - 0.08131 * b;
		return c;
	}

	Color3 YCbCrToRgb() {
		T G = g - 0.5f;
		T B = b - 0.5f;

		Color3 c;
		c.r = r +                 1.402    * B;
		c.g = r + -0.344136 * G - 0.714136 * B;
		c.b = r +  1.772    * G;
		return c;
	}

	Color3 toYcc() {
		Color3 yog;
		yog.g = r - b;
		T tmp = b + yog.g/2;
		yog.b = g - tmp;
		yog.r = tmp + yog.b/2;
		return yog;
	}
	Color3 toRgb() {
		Color3 rgb;
		T tmp = r - b/2;
		rgb.g = b + tmp;
		rgb.b = tmp - g/2;
		rgb.r = rgb.b + g;
		return rgb;
	}
	Color3 clip() {
		return Color3(r / 255, g / 255, b / 255);
	}
	void operator+=(const Color3 &c) {
		r += c.r;
		g += c.g;
		b += c.b;
	}
	void operator-=(const Color3 &c) {
		r -= c.r;
		g -= c.g;
		b -= c.b;
	}

	void operator*=(float v) {
		r *= v;
		g *= v;
		b *= v;
	}
	void operator/=(float v) {
		r /= v;
		g /= v;
		b /= v;
	}
	bool operator==(const Color3<uint8_t>& other) const {
		return r == other.r && g == other.g && b == other.b;
	}
	Color3 operator/(float v) {
		return Color3(r / v, g / v, b / v);
	}
	Color3 operator*(float v) {
		return Color3(r * v, g * v, b * v);
	}
	float mean() {
		return (r + g + b)/3;
	}
};
typedef Color3<uint8_t>  Color3b;
typedef Color3<uint16_t>  Color3us;
typedef Color3<float>  Color3f;
typedef Color3<double>  Color3d;


#endif // COLOR_H
