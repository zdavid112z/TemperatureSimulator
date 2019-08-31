#pragma once

#include "Pch.h"

struct Color {
	Color(byte v = 0)
		: r(v), g(v), b(v) {}
	Color(byte r, byte g, byte b)
		: r(r), g(g), b(b) {}

	byte r, g, b;

	Color Lerp(const Color& color, float amount);
};

class Image {

public:
	Image(uint width, uint height, float* image);
	~Image();

	void WriteToFile(const std::string& path);
private:
	Color Interpolate(float value);
private:
	byte* m_Data;
	uint m_Width, m_Height;
};

struct FloatImage
{
	FloatImage();
	~FloatImage();
	void Init(uint width, uint height);
	float* operator[](int index);
	const float* operator[](int index) const;

	uint width, height;
	float* data;
};