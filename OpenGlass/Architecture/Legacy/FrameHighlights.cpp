// Frame highlights adapted from arceuss/OpenGlass, commit d8a826e.
#include "pch.h"
#include "FrameHighlights.hpp"
#include "GlassKernel.hpp"
#include "CustomThemeAtlasLoader.hpp"
#include "PngAssetValidation.hpp"

using namespace OpenGlass;

namespace OpenGlass::FrameHighlights
{
	std::unordered_map<uDWM::CTopLevelWindow*, winrt::com_ptr<uDWM::CImageLegacyMilBrushProxy>> g_brushes{};

	HRESULT LoadThemeStrip(IWICBitmapSource* atlas, HTHEME theme, int part, HighlightStrip& strip)
	{
		RECT rect{};
		RETURN_IF_FAILED(CustomThemeAtlasLoader::MyGetThemeRect(theme, part, 0, TMT_ATLASRECT, &rect));
		UINT width{}, height{};
		RETURN_IF_FAILED(atlas->GetSize(&width, &height));
		const auto stripWidth = wil::rect_width(rect);
		const auto stripHeight = wil::rect_height(rect);
		// Theme/layout coordinates cross into the decoded bitmap here.
		RETURN_HR_IF(E_INVALIDARG, rect.left < 0 || rect.top < 0 || stripWidth <= 0 || stripHeight < 2 ||
			stripHeight % 2 || static_cast<UINT>(rect.right) > width || static_cast<UINT>(rect.bottom) > height);
		HighlightStrip result;
		result.width = stripWidth;
		result.frameHeight = stripHeight / 2;
		result.pixels.resize(static_cast<size_t>(stripWidth) * stripHeight * 4);
		const WICRect copy{ rect.left, rect.top, stripWidth, stripHeight };
		RETURN_IF_FAILED(atlas->CopyPixels(&copy, result.width * 4, static_cast<UINT>(result.pixels.size()), result.pixels.data()));
		strip = std::move(result);
		return S_OK;
	}

	HRESULT LoadThemeHighlights()
	{
		const auto theme = CustomThemeAtlasLoader::GetThemeHandle();
		if (!theme)
		{
			return S_OK;
		}
		void* bytes{};
		DWORD size{};
		RETURN_IF_FAILED(CustomThemeAtlasLoader::MyGetThemeStream(theme, 0, 0, TMT_DISKSTREAM, &bytes, &size, nullptr));
		winrt::com_ptr<IStream> stream{ SHCreateMemStream(static_cast<const BYTE*>(bytes), size), winrt::take_ownership_from_abi };
		RETURN_HR_IF_NULL(E_OUTOFMEMORY, stream);
		winrt::com_ptr<IWICImagingFactory> factory;
		RETURN_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.put())));
		winrt::com_ptr<IWICFormatConverter> atlas;
		RETURN_IF_FAILED(PngAssetValidation::CreateValidatedWicSource(factory.get(), stream.get(), nullptr, atlas.put()));
		const auto cornerId = static_cast<int>(GlassEngine::GetDwordFromRegistry(L"LEFTHIGHLIGHTCORNERid", 95));
		const auto sideId = static_cast<int>(GlassEngine::GetDwordFromRegistry(L"SIDEHIGHLIGHTid", 96));
		if (SUCCEEDED(LoadThemeStrip(atlas.get(), theme, cornerId, g_settings.corner)))
		{
			for (int state = 1; state <= 4; ++state)
			{
				int opacity{};
				if (SUCCEEDED(CustomThemeAtlasLoader::MyGetThemeInt(theme, cornerId, state, TMT_OPACITY, &opacity)))
				{
					g_settings.cornerOpacity[state - 1] = std::clamp(opacity / 100.f, 0.f, 1.f);
				}
			}
		}
		if (SUCCEEDED(LoadThemeStrip(atlas.get(), theme, sideId, g_settings.side)))
		{
			for (int state = 1; state <= 2; ++state)
			{
				int opacity{};
				if (SUCCEEDED(CustomThemeAtlasLoader::MyGetThemeInt(theme, sideId, state, TMT_OPACITY, &opacity)))
				{
					g_settings.sideOpacity[state - 1] = std::clamp(opacity / 100.f, 0.f, 1.f);
				}
			}
			MARGINS margins{};
			if (SUCCEEDED(CustomThemeAtlasLoader::MyGetThemeMargins(theme, nullptr, sideId, 0, TMT_SIZINGMARGINS, nullptr, &margins)))
			{
				g_settings.sizingMargin = std::clamp(margins.cyBottomHeight, 0, static_cast<int>(g_settings.side.frameHeight));
			}
		}
		return S_OK;
	}
}

HRESULT FrameHighlights::UpdateWindow(uDWM::CTopLevelWindow* window)
{
	const auto it = g_brushes.find(window);
	if (it == g_brushes.end() || window->IsOffscreen())
	{
		return S_OK;
	}
	const auto status = GlassKernel::GetWindowStatus(window);
	const auto visual = window->GetLegacyVisual();
	if (!visual)
	{
		return S_OK;
	}
	const auto& inside = window->GetFrameInsideMargins();
	const auto nativeMaximized = window->IsWindowMaximized();
	const auto& outside = window->GetFrameOutsideMargins(nativeMaximized);
	RECT rect{};
	window->GetActualWindowRect(&rect, true, false, true);
	const auto& scale = visual->GetScale();
	const D2D1_RECT_F viewport{
		static_cast<float>((nativeMaximized ? 0 : rect.left) / scale.width),
		static_cast<float>((nativeMaximized ? 0 : rect.top) / scale.height),
		static_cast<float>((nativeMaximized ? visual->GetWidth() : rect.right) / scale.width),
		static_cast<float>((nativeMaximized ? visual->GetHeight() : rect.bottom) / scale.height) };
	const D2D1_RECT_F margins{
		static_cast<float>(inside.cxLeftWidth - outside.cxLeftWidth),
		static_cast<float>(inside.cyTopHeight - outside.cyTopHeight),
		static_cast<float>(inside.cxRightWidth - outside.cxRightWidth),
		static_cast<float>(inside.cyBottomHeight - outside.cyBottomHeight) };
	return it->second->Update(g_settings.style == HighlightStyle::Disabled ? 0. :
		Encode(status.test(0), status.test(1), window->GetData()->IsSheetOfGlass()),
		viewport, margins, nullptr, DWM::MilBrushMappingMode::Absolute, DWM::MilBrushMappingMode::Absolute,
		nullptr, nullptr, DWM::MilStretch::None, DWM::MilTileMode::Extend,
		DWM::MilHorizontalAlignment::Left, DWM::MilVerticalAlignment::Top, nullptr);
}

HRESULT FrameHighlights::Append(uDWM::CTopLevelWindow* window, uDWM::CRenderDataVisual* visual,
	uDWM::CBaseGeometryProxy* geometry)
{
	auto& brush = g_brushes[window];
	if (!brush)
	{
		RETURN_IF_FAILED(uDWM::CDesktopManager::GetInstance()->GetCompositor()->CreateImageLegacyMilBrushProxy(brush.put()));
	}
	RETURN_IF_FAILED(UpdateWindow(window));
	winrt::com_ptr<uDWM::CDrawGeometryInstruction> instruction;
	RETURN_IF_FAILED(uDWM::CDrawGeometryInstruction::Create(brush.get(), geometry, instruction.put()));
	return visual->AddInstruction(instruction.get());
}

void FrameHighlights::Update(GlassEngine::UpdateType type)
{
	if (!(type & GlassEngine::UpdateType::Theme))
	{
		return;
	}
	const auto generation = g_settings.generation + 1;
	g_settings = {};
	g_settings.generation = generation;
	g_settings.style = static_cast<HighlightStyle>(std::clamp(GlassEngine::GetDwordFromRegistry(L"HighlightStyle", 0), 0ul, 3ul));
	if (g_settings.style == HighlightStyle::WindowsVista)
	{
		g_settings.cornerOpacity = { .75f, .75f, 0.f, 0.f };
	}
	else if (g_settings.style == HighlightStyle::Msstyles)
	{
		LOG_IF_FAILED(LoadThemeHighlights());
	}
}

void FrameHighlights::Remove(uDWM::CTopLevelWindow* window)
{
	g_brushes.erase(window);
}

void FrameHighlights::Cleanup()
{
	g_brushes.clear();
	g_settings.corner = {};
	g_settings.side = {};
}
