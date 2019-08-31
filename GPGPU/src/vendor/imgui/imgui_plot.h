#pragma once

#include "imgui.h"

namespace ImGui {
	// These 2 have a completely different implementation:
	// Posted by @JaapSuter and @maxint (please see: https://github.com/ocornut/imgui/issues/632)
	IMGUI_API void PlotMultiLines(
		const char* label,
		int num_datas,
		const char** names,
		const ImColor* colors,
		float(*getter)(const void* data, int idx),
		const void * const * datas,
		int values_count,
		float scale_min,
		float scale_max,
		ImVec2 graph_size);

	// Posted by @JaapSuter and @maxint (please see: https://github.com/ocornut/imgui/issues/632)
	IMGUI_API void PlotMultiHistograms(
		const char* label,
		int num_hists,
		const char** names,
		const ImColor* colors,
		float(*getter)(const void* data, int idx),
		const void * const * datas,
		int values_count,
		float scale_min,
		float scale_max,
		ImVec2 graph_size);
}