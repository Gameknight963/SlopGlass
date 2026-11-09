// Frame highlight rendering adapted from arceuss/OpenGlass, commit d8a826e.
#include "pch.h"
#include "HighlightRealizer.hpp"
#include "resource.h"
#include "PngAssetValidation.hpp"
#include "RectF.hpp"

using namespace OpenGlass;

namespace
{
	HRESULT LoadStrip(ID2D1DeviceContext* context, IWICImagingFactory* factory, int resourceId,
		const HighlightStrip& strip, winrt::com_ptr<ID2D1Bitmap1>& active, winrt::com_ptr<ID2D1Bitmap1>& inactive)
	{
		const auto properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
		if (!strip.pixels.empty())
		{
			const UINT pitch = strip.width * 4;
			RETURN_IF_FAILED(context->CreateBitmap(D2D1::SizeU(strip.width, strip.frameHeight),
				strip.pixels.data(), pitch, properties, active.put()));
			return context->CreateBitmap(D2D1::SizeU(strip.width, strip.frameHeight),
				strip.pixels.data() + static_cast<size_t>(pitch) * strip.frameHeight, pitch, properties, inactive.put());
		}

		const auto resource = FindResourceW(wil::GetModuleInstanceHandle(), MAKEINTRESOURCEW(resourceId), L"PNG");
		RETURN_LAST_ERROR_IF_NULL(resource);
		const auto loaded = LoadResource(wil::GetModuleInstanceHandle(), resource);
		RETURN_LAST_ERROR_IF_NULL(loaded);
		winrt::com_ptr<IStream> stream{ SHCreateMemStream(static_cast<const BYTE*>(LockResource(loaded)),
			SizeofResource(wil::GetModuleInstanceHandle(), resource)), winrt::take_ownership_from_abi };
		RETURN_HR_IF_NULL(E_OUTOFMEMORY, stream);
		winrt::com_ptr<IWICFormatConverter> source;
		RETURN_IF_FAILED(PngAssetValidation::CreateValidatedWicSource(factory, stream.get(), nullptr, source.put()));
		UINT width{}, height{};
		RETURN_IF_FAILED(source->GetSize(&width, &height));
		for (int state = 0; state < 2; ++state)
		{
			winrt::com_ptr<IWICBitmapClipper> clipper;
			RETURN_IF_FAILED(factory->CreateBitmapClipper(clipper.put()));
			const WICRect rect{ 0, state * static_cast<int>(height / 2), static_cast<int>(width), static_cast<int>(height / 2) };
			RETURN_IF_FAILED(clipper->Initialize(source.get(), &rect));
			RETURN_IF_FAILED(context->CreateBitmapFromWicBitmap(clipper.get(), properties,
				state ? inactive.put() : active.put()));
		}
		return S_OK;
	}
}

HRESULT CHighlightRealizer::LoadTextures(ID2D1DeviceContext* context, const HighlightSettings& settings)
{
	winrt::com_ptr<IWICImagingFactory> factory;
	RETURN_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.put())));
	m_sizingMargin = settings.sizingMargin;
	// Publish a complete pair only after both corner and side textures load.
	winrt::com_ptr<ID2D1Bitmap1> corner, cornerInactive, side, sideInactive;
	RETURN_IF_FAILED(LoadStrip(context, factory.get(),
		settings.style == HighlightStyle::WindowsVista ? IDB_HIGHLIGHT_VISTA : IDB_HIGHLIGHT,
		settings.corner, corner, cornerInactive));
	RETURN_IF_FAILED(LoadStrip(context, factory.get(), IDB_SIDEHIGHLIGHT, settings.side, side, sideInactive));
	m_highlightBitmap = std::move(corner);
	m_highlightBitmapInactive = std::move(cornerInactive);
	m_sideHighlightBitmap = std::move(side);
	m_sideHighlightBitmapInactive = std::move(sideInactive);
	return S_OK;
}

HRESULT CHighlightRealizer::Render(
	ID2D1DeviceContext* context,
	const std::span<const D2D1_RECT_F>& rectangles,
	const HighlightContext& highlightContext,
	const HighlightSettings& settings
)
{
	const auto generation = settings.generation;
	if (generation != m_generation)
	{
		Reset();
		m_generation = generation;
	}

	if (!m_highlightBitmap)
	{
		RETURN_IF_FAILED(LoadTextures(context, settings));
	}

	const auto worldTransform3D = highlightContext.worldTransform;
	auto worldTransform2DInversed = highlightContext.worldTransform2D;
	D2D1InvertMatrix(&worldTransform2DInversed);

	const auto highlightBitmapSize = highlightContext.active ? m_highlightBitmap->GetSize() : m_highlightBitmapInactive->GetSize();
	const auto sideHighlightBitmapSize = highlightContext.active ? m_sideHighlightBitmap->GetSize() : m_sideHighlightBitmapInactive->GetSize();

	const D2D1_SIZE_F viewportSize
	{
		wil::rect_width(*highlightContext.viewport),
		wil::rect_height(*highlightContext.viewport)
	};

	const D2D1_RECT_F frameMargins
	{
		highlightContext.viewbox->left,
		highlightContext.viewbox->top,
		highlightContext.viewbox->right,
		highlightContext.viewbox->bottom,
	};

	const auto clip = highlightContext.viewport->left + (viewportSize.width / 2.0f);

	const D2D1_RECT_F leftDestRectangle
	{
		highlightContext.viewport->left,
		highlightContext.viewport->top,
		std::min(highlightContext.viewport->left + highlightBitmapSize.width, clip),
		highlightContext.viewport->top + highlightBitmapSize.height
	};

	const D2D1_RECT_F rightDestRectangle
	{
		std::max(highlightContext.viewport->right - highlightBitmapSize.width, clip),
		highlightContext.viewport->top,
		highlightContext.viewport->right,
		highlightContext.viewport->top + highlightBitmapSize.height
	};

	const auto cx = 2.0f * (highlightContext.viewport->right - highlightBitmapSize.width + (highlightBitmapSize.width / 2.0f));

	const D2D1_MATRIX_4X4_F flipWorldTransform3D = D2D1::Matrix4x4F(
		-worldTransform3D._11, -worldTransform3D._12, -worldTransform3D._13, -worldTransform3D._14,
		worldTransform3D._21, worldTransform3D._22, worldTransform3D._23, worldTransform3D._24,
		worldTransform3D._31, worldTransform3D._32, worldTransform3D._33, worldTransform3D._34,
		cx * worldTransform3D._11 + worldTransform3D._41,
		cx * worldTransform3D._12 + worldTransform3D._42,
		cx * worldTransform3D._13 + worldTransform3D._43,
		cx * worldTransform3D._14 + worldTransform3D._44
	);

	const auto leftSideWidth = frameMargins.left - 1;
	const auto rightSideWidth = frameMargins.right - 1;
	const auto cxSide = 2.0f * (highlightContext.viewport->right - rightSideWidth + (rightSideWidth / 2.0f));

	const D2D1_MATRIX_4X4_F flipSideWorldTransform3D = D2D1::Matrix4x4F(
		-worldTransform3D._11, -worldTransform3D._12, -worldTransform3D._13, -worldTransform3D._14,
		worldTransform3D._21, worldTransform3D._22, worldTransform3D._23, worldTransform3D._24,
		worldTransform3D._31, worldTransform3D._32, worldTransform3D._33, worldTransform3D._34,
		cxSide * worldTransform3D._11 + worldTransform3D._41,
		cxSide * worldTransform3D._12 + worldTransform3D._42,
		cxSide * worldTransform3D._13 + worldTransform3D._43,
		cxSide * worldTransform3D._14 + worldTransform3D._44
	);

	// Preserve the source fork's approximate side-highlight placement and stretch.
	const auto leftScaleX = leftSideWidth > 0.f ? sideHighlightBitmapSize.width / leftSideWidth : 0.f;
	const auto rightScaleX = rightSideWidth > 0.f ? sideHighlightBitmapSize.width / rightSideWidth : 0.f;

	const auto topOffset = frameMargins.top;
	const auto height = std::max(0.f, viewportSize.height - frameMargins.top - frameMargins.bottom) * .25f;

	const auto destinationHeight = std::max(0.0f, height - std::min(height, static_cast<float>(m_sizingMargin)));
	const auto sourceHeight = std::max(0.0f, sideHighlightBitmapSize.height - static_cast<float>(m_sizingMargin));
	const auto scaleY = destinationHeight > 0.f ? sourceHeight / destinationHeight : 0.f;

	const D2D1_RECT_F leftSideDestRect
	{
		highlightContext.viewport->left,
		highlightContext.viewport->top + topOffset,
		highlightContext.viewport->left + leftSideWidth,
		(highlightContext.viewport->top + topOffset) + destinationHeight
	};
	const D2D1_RECT_F leftSideDestRectBottom
	{
		highlightContext.viewport->left,
		(highlightContext.viewport->top + topOffset) + destinationHeight,
		highlightContext.viewport->left + leftSideWidth,
		(highlightContext.viewport->top + topOffset) + height
	};

	const D2D1_RECT_F rightSideDestRect
	{
		highlightContext.viewport->right - rightSideWidth,
		highlightContext.viewport->top + topOffset,
		highlightContext.viewport->right,
		(highlightContext.viewport->top + topOffset) + destinationHeight
	};
	const D2D1_RECT_F rightSideDestRectBottom
	{
		highlightContext.viewport->right - rightSideWidth,
		(highlightContext.viewport->top + topOffset) + destinationHeight,
		highlightContext.viewport->right,
		(highlightContext.viewport->top + topOffset) + height
	};

	for (auto subRectangle : rectangles)
	{
		subRectangle = RectF::TransformRect(subRectangle, worldTransform2DInversed);

		auto destRectangle = subRectangle;
		if (RectF::IntersectUnsafe(destRectangle, leftDestRectangle))
		{
			D2D1_RECT_F sourceRectangle
			{
				destRectangle.left - highlightContext.viewport->left,
				destRectangle.top - highlightContext.viewport->top,
				destRectangle.right - highlightContext.viewport->left,
				destRectangle.bottom - highlightContext.viewport->top
			};
			context->DrawBitmap(
				highlightContext.active ? m_highlightBitmap.get() : m_highlightBitmapInactive.get(),
				destRectangle,
				highlightContext.opacity,
				D2D1_INTERPOLATION_MODE_LINEAR,
				&sourceRectangle,
				&worldTransform3D
			);
		}

		destRectangle = subRectangle;
		if (RectF::IntersectUnsafe(destRectangle, rightDestRectangle))
		{
			D2D1_RECT_F flipDestRectangle = destRectangle;
			flipDestRectangle.left = cx - destRectangle.right;
			flipDestRectangle.right = cx - destRectangle.left;
			D2D1_RECT_F sourceRectangle
			{
				flipDestRectangle.left - (highlightContext.viewport->right - highlightBitmapSize.width),
				flipDestRectangle.top - highlightContext.viewport->top,
				flipDestRectangle.right - (highlightContext.viewport->right - highlightBitmapSize.width),
				flipDestRectangle.bottom - highlightContext.viewport->top
			};
			context->DrawBitmap(
				highlightContext.active ? m_highlightBitmap.get() : m_highlightBitmapInactive.get(),
				flipDestRectangle,
				highlightContext.opacity,
				D2D1_INTERPOLATION_MODE_LINEAR,
				&sourceRectangle,
				&flipWorldTransform3D
			);
		}

		if (!highlightContext.sheetOfGlass)
		{
			destRectangle = subRectangle;
			if (RectF::IntersectUnsafe(destRectangle, leftSideDestRect))
			{
				D2D1_RECT_F sourceRectangle
				{
					(destRectangle.left - leftSideDestRect.left) * leftScaleX,
					(destRectangle.top - leftSideDestRect.top) * scaleY,
					(destRectangle.right - leftSideDestRect.left) * leftScaleX,
					(destRectangle.bottom - leftSideDestRect.top) * scaleY
				};
				context->DrawBitmap(
					highlightContext.active ? m_sideHighlightBitmap.get() : m_sideHighlightBitmapInactive.get(),
					destRectangle,
					highlightContext.sideOpacity,
					D2D1_INTERPOLATION_MODE_LINEAR,
					&sourceRectangle,
					&worldTransform3D
				);
			}
			destRectangle = subRectangle;
			if (RectF::IntersectUnsafe(destRectangle, leftSideDestRectBottom))
			{
				D2D1_RECT_F sourceRectangle
				{
					(destRectangle.left - leftSideDestRectBottom.left) * leftScaleX,
					sourceHeight + (destRectangle.top - leftSideDestRectBottom.top),
					(destRectangle.right - leftSideDestRectBottom.left) * leftScaleX,
					sourceHeight + (destRectangle.bottom - leftSideDestRectBottom.top)
				};
				context->DrawBitmap(
					highlightContext.active ? m_sideHighlightBitmap.get() : m_sideHighlightBitmapInactive.get(),
					destRectangle,
					highlightContext.sideOpacity,
					D2D1_INTERPOLATION_MODE_LINEAR,
					&sourceRectangle,
					&worldTransform3D
				);
			}

			destRectangle = subRectangle;
			if (RectF::IntersectUnsafe(destRectangle, rightSideDestRect))
			{
				D2D1_RECT_F flipDestRectangle = destRectangle;
				flipDestRectangle.left = cxSide - destRectangle.right;
				flipDestRectangle.right = cxSide - destRectangle.left;

				D2D1_RECT_F sourceRectangle
				{
					(flipDestRectangle.left - rightSideDestRect.left) * rightScaleX,
					(flipDestRectangle.top - rightSideDestRect.top) * scaleY,
					(flipDestRectangle.right - rightSideDestRect.left) * rightScaleX,
					(flipDestRectangle.bottom - rightSideDestRect.top) * scaleY
				};
				context->DrawBitmap(
					highlightContext.active ? m_sideHighlightBitmap.get() : m_sideHighlightBitmapInactive.get(),
					flipDestRectangle,
					highlightContext.sideOpacity,
					D2D1_INTERPOLATION_MODE_LINEAR,
					&sourceRectangle,
					&flipSideWorldTransform3D
				);
			}
			destRectangle = subRectangle;
			if (RectF::IntersectUnsafe(destRectangle, rightSideDestRectBottom))
			{
				D2D1_RECT_F flipDestRectangle = destRectangle;
				flipDestRectangle.left = cxSide - destRectangle.right;
				flipDestRectangle.right = cxSide - destRectangle.left;

				D2D1_RECT_F sourceRectangle
				{
					(flipDestRectangle.left - rightSideDestRectBottom.left) * rightScaleX,
					sourceHeight + (flipDestRectangle.top - rightSideDestRectBottom.top),
					(flipDestRectangle.right - rightSideDestRectBottom.left) * rightScaleX,
					sourceHeight + (flipDestRectangle.bottom - rightSideDestRectBottom.top)
				};
				context->DrawBitmap(
					highlightContext.active ? m_sideHighlightBitmap.get() : m_sideHighlightBitmapInactive.get(),
					flipDestRectangle,
					highlightContext.sideOpacity,
					D2D1_INTERPOLATION_MODE_LINEAR,
					&sourceRectangle,
					&flipSideWorldTransform3D
				);
			}
		}
	}

	return S_OK;
}
