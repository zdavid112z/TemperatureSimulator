#pragma once

#include "Pch.h"
#include "CL/cl.hpp"

namespace Utils {

	std::string LoadFile(const std::string& path);
	const char* GetCLErrorCode(cl_int errorCode);
	void AssertCLError(cl_int errorCode);

};