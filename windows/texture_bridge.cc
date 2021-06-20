#include <algorithm>
#include <atomic>
#include <cassert>
#include <iostream>

#include "texture_bridge_fallback.h"
#include "util/direct3d11.interop.h"

using namespace winrt::Windows::Graphics::Capture;

namespace {
const int kNumBuffers = 1;
}  // namespace

TextureBridge::TextureBridge(GraphicsContext* graphics_context,
                             winrt::Windows::UI::Composition::Visual visual)
    : graphics_context_(graphics_context) {
  capture_item_ =
      winrt::Windows::Graphics::Capture::GraphicsCaptureItem::CreateFromVisual(
          visual);
  if (!capture_item_) {
    return;
  }

  frame_pool_ = Direct3D11CaptureFramePool::CreateFreeThreaded(
      graphics_context->device(), kPixelFormat, kNumBuffers,
      capture_item_.Size());

  if (!frame_pool_) {
    return;
  }

  capture_session_ = frame_pool_.CreateCaptureSession(capture_item_);
  frame_arrived_ = frame_pool_.FrameArrived(
      winrt::auto_revoke, {this, &TextureBridge::OnFrameArrived});

  is_valid_ = true;
}

TextureBridge::~TextureBridge() { Stop(); }

bool TextureBridge::Start() {
  if (!is_valid_) {
    return false;
  }
  if (!is_running_) {
    is_running_ = true;
    capture_session_.StartCapture();
  }
  return true;
}

void TextureBridge::Stop() {
  if (is_running_) {
    is_running_ = false;
    capture_session_.Close();
  }
}

void TextureBridge::OnFrameArrived(
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
    winrt::Windows::Foundation::IInspectable const&) {
  {
    if (!is_running_) {
      return;
    }

    if (needs_update_) {
      frame_pool_.Recreate(graphics_context_->device(), kPixelFormat,
                           kNumBuffers, capture_item_.Size());
      needs_update_ = false;
    }
  }

  if (frame_available_) {
    frame_available_();
  }
}

void TextureBridge::NotifySurfaceSizeChanged() { needs_update_ = true; }
