#include "Pch.h"
#include "Image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

Color Color::Lerp(const Color& color, float amount)
{
	return Color(
		byte(float(color.r) * amount + float(r) * (1.f - amount)),
		byte(float(color.g) * amount + float(g) * (1.f - amount)),
		byte(float(color.b) * amount + float(b) * (1.f - amount)));
}

Image::Image(uint width, uint height, float* image) :
	m_Width(width),
	m_Height(height)
{
	m_Data = new byte[width * height * 3 * sizeof(byte)];
	for (uint y = 0; y < height; y++)
	{
		for (uint x = 0; x < width; x++)
		{
			uint index = x + y * width;
			Color c = Interpolate(image[index]);
			m_Data[index * 3 + 0] = c.r;
			m_Data[index * 3 + 1] = c.g;
			m_Data[index * 3 + 2] = c.b;
		}
	}
}

Image::~Image()
{
	delete m_Data;
}

void Image::WriteToFile(const std::string& path)
{
	stbi_write_bmp(path.c_str(), m_Width, m_Height, 3, m_Data);
}

Color Image::Interpolate(float value)
{
	float temp = value / 10.f;
	float red, green, blue;
	if (temp <= 66) {
		red = 255;
		green = temp;
		green = 99.4708025861f * std::log(green) - 161.1195681661f;
		if (temp <= 19) {
			blue = 0;
		}
		else {
			blue = temp - 10.f;
			blue = 138.5177312231f * std::log(blue) - 305.0447927307f;
		}
	}
	else {
		red = temp - 60.f;
		red = 329.698727446f * std::pow(red, -0.1332047592f);
		green = temp - 60.f;
		green = 288.1221695283f * std::pow(green, -0.0755148492f);
		blue = 255.f;
	}
	return Color(
		byte(std::max(std::min(red, 255.f), 0.f)),
		byte(std::max(std::min(green, 255.f), 0.f)),
		byte(std::max(std::min(blue, 255.f), 0.f)));
}

FloatImage::FloatImage()
{
	data = nullptr;
}

void FloatImage::Init(uint width, uint height)
{
	this->width = width;
	this->height = height;
	data = new float[width * height];
}

FloatImage::~FloatImage()
{
	if (data)
	{
		delete[] data;
		data = nullptr;
	}
}

float* FloatImage::operator[](int index)
{
	return data + index * width;
}

const float* FloatImage::operator[](int index) const
{
	return data + index * width;
}