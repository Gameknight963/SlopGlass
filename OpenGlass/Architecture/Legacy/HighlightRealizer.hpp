#pragma once
#include "framework.hpp"
#include "cpprt.hpp"

namespace OpenGlass
{
	enum class HighlightStyle : DWORD
	{
		Windows7,
		WindowsVista,
		Msstyles,
		Disabled
	};

	struct HighlightStrip
	{
		UINT width{}, frameHeight{};
		std::vector<BYTE> pixels;
	};
	struct HighlightSettings
	{
		HighlightStyle style{ HighlightStyle::Windows7 };
		std::array<float, 4> cornerOpacity{ .75f, .75f, .75f, .75f };
		std::array<float, 2> sideOpacity{ 1.f, .5f };
		int sizingMargin{ 2 };
		HighlightStrip corner, side;
		UINT generation{};
	};
	struct HighlightContext
	{
		D2D1_MATRIX_4X4_F worldTransform;
		D2D1_MATRIX_3X2_F worldTransform2D;
		const D2D1_RECT_F* viewport;
		const D2D1_RECT_F* viewbox;
		bool sheetOfGlass;
		bool active;
		float opacity;
		float sideOpacity;
	};
	class CHighlightRealizer
	{
		winrt::com_ptr<ID2D1Bitmap1> m_highlightBitmap, m_highlightBitmapInactive;
		winrt::com_ptr<ID2D1Bitmap1> m_sideHighlightBitmap, m_sideHighlightBitmapInactive;
		int m_sizingMargin{ 2 };
		UINT m_generation{};
		HRESULT LoadTextures(ID2D1DeviceContext* context, const HighlightSettings& settings);
	public:
		HRESULT Render(ID2D1DeviceContext* context, const std::span<const D2D1_RECT_F>& rectangles,
			const HighlightContext& highlightContext, const HighlightSettings& settings);
		void Reset()
		{
			m_highlightBitmap = nullptr;
			m_highlightBitmapInactive = nullptr;
			m_sideHighlightBitmap = nullptr;
			m_sideHighlightBitmapInactive = nullptr;
		}
	};
}
