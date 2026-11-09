#pragma once
#include "HighlightRealizer.hpp"
#include "uDwmProjection.hpp"
#include "GlassEngine.hpp"

namespace OpenGlass::FrameHighlights
{
	inline HighlightSettings g_settings{};

	// OpenGlass-owned image-brush payload, carried through native render commands.
	// Like the existing glass alpha marker, this remains inside the opacity range.
	inline float Encode(bool active, bool maximized, bool sheetOfGlass)
	{
		return std::bit_cast<float>(0x3F000002u | (active << 8) | (maximized << 9) | (sheetOfGlass << 10));
	}
	inline bool IsMarker(float opacity)
	{
		return (std::bit_cast<DWORD>(opacity) & 0xFFFFF8FFu) == 0x3F000002u;
	}
	inline bool IsActive(float opacity)
	{
		return (std::bit_cast<DWORD>(opacity) & (1u << 8)) != 0;
	}
	inline bool IsMaximized(float opacity)
	{
		return (std::bit_cast<DWORD>(opacity) & (1u << 9)) != 0;
	}
	inline bool IsSheetOfGlass(float opacity)
	{
		return (std::bit_cast<DWORD>(opacity) & (1u << 10)) != 0;
	}

	HRESULT Append(uDWM::CTopLevelWindow* window, uDWM::CRenderDataVisual* visual,
		uDWM::CBaseGeometryProxy* geometry);
	HRESULT UpdateWindow(uDWM::CTopLevelWindow* window);
	void Update(GlassEngine::UpdateType type);
	void Remove(uDWM::CTopLevelWindow* window);
	void Cleanup();
}
