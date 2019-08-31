#pragma once

#define _CRTDBG_MAP_ALLOC 
#define _CRTDBG_MAP_ALLOC_NEW

#include <stdlib.h> 
#include <crtdbg.h> 

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#pragma comment(lib, "OpenCL.lib")

#include <algorithm>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <unordered_map>
#include <functional>

#include <cstdlib>
#include <cstring>
#include <cmath>

#include <iostream>
#include <fstream>

typedef unsigned int uint;
typedef uint8_t byte;

typedef uint32_t uint32;