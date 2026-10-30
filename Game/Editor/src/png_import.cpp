#include "Epidemic/Game/Editor/png_import.h"
#include "Epidemic/Game/Editor/image_resource.h"
#include <array>
#include <fstream>
#include <limits>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <wincodec.h>
#include <windows.h>
#include <wrl/client.h>
#endif
namespace epidemic::game::editor {
namespace {
template <class T> foundation::Result<T> Error(std::string_view code) {
  return foundation::Result<T>::Failure(
      foundation::Error::Create(code, "PNG import rejected"));
}
constexpr std::uint64_t kMaxPixels = 4096ULL * 4096ULL;
bool Signature(const std::filesystem::path &path) {
  std::ifstream stream(path, std::ios::binary);
  std::array<unsigned char, 8> bytes{};
  stream.read(reinterpret_cast<char *>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
  return stream.gcount() == static_cast<std::streamsize>(bytes.size()) &&
         bytes == std::array<unsigned char, 8>{137, 80, 78, 71, 13, 10, 26, 10};
}
} // namespace
foundation::Result<DecodedPng> DecodePng(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  input.seekg(0, std::ios::end);
  const std::streamoff bytes = input.tellg();
  if (!input || bytes < 0 || bytes > 128LL * 1024LL * 1024LL)
    return Error<DecodedPng>("editor.png_source_limit");
  if (!Signature(path))
    return Error<DecodedPng>("editor.png_signature");
#ifdef _WIN32
  using Microsoft::WRL::ComPtr;
  // Caller controls COM apartment; do not uninitialize an apartment we do
  // not own. WIC is available on supported Windows desktop environments.
  const auto initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE)
    return Error<DecodedPng>("editor.png_com");
  struct ComGuard {
    HRESULT code;
    ~ComGuard() {
      if (SUCCEEDED(code))
        CoUninitialize();
    }
  } guard{initialized};
  ComPtr<IWICImagingFactory> factory;
  if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                              CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
    return Error<DecodedPng>("editor.png_wic");
  ComPtr<IWICBitmapDecoder> decoder;
  if (FAILED(factory->CreateDecoderFromFilename(
          path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand,
          &decoder)))
    return Error<DecodedPng>("editor.png_decode");
  GUID format{};
  if (FAILED(decoder->GetContainerFormat(&format)) ||
      format != GUID_ContainerFormatPng)
    return Error<DecodedPng>("editor.png_format");
  ComPtr<IWICBitmapFrameDecode> frame;
  if (FAILED(decoder->GetFrame(0, &frame)))
    return Error<DecodedPng>("editor.png_frame");
  UINT w = 0, h = 0;
  if (FAILED(frame->GetSize(&w, &h)) || !w || !h || w > 4096 || h > 4096 ||
      static_cast<std::uint64_t>(w) * h > kMaxPixels)
    return Error<DecodedPng>("editor.png_dimensions");
  ComPtr<IWICFormatConverter> convert;
  if (FAILED(factory->CreateFormatConverter(&convert)) ||
      FAILED(convert->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
                                 WICBitmapDitherTypeNone, nullptr, 0,
                                 WICBitmapPaletteTypeCustom)))
    return Error<DecodedPng>("editor.png_conversion");
  DecodedPng decoded;
  decoded.width = w;
  decoded.height = h;
  decoded.pixels.resize(static_cast<std::size_t>(w) * h * 4);
  if (FAILED(convert->CopyPixels(
          nullptr, w * 4, static_cast<UINT>(decoded.pixels.size()),
          reinterpret_cast<BYTE *>(decoded.pixels.data()))))
    return Error<DecodedPng>("editor.png_pixels");
  return foundation::Result<DecodedPng>::Success(std::move(decoded));
#else
  (void)path;
  return Error<DecodedPng>("editor.png_windows_only");
#endif
}
foundation::Result<runtime::AssetId>
ImportProjectPng(ProjectFolder &project, std::string_view relative) {
  return CookProjectPng(project, relative);
}
foundation::Result<runtime::AssetId>
ImportExternalPng(ProjectFolder &project, const std::filesystem::path &external,
                  std::string_view content_relative) {
  { // Release the validation buffer before decoding/cooking the copied file.
    const auto decoded = DecodePng(external);
    if (!decoded)
      return foundation::Result<runtime::AssetId>::Failure(decoded.GetError());
  }
  const auto target = project.ResolveMount("Content", content_relative);
  if (!target)
    return Error<runtime::AssetId>("editor.png_path");
  std::error_code ec;
  if (std::filesystem::exists(target.Value(), ec) || ec ||
      !std::filesystem::is_directory(target.Value().parent_path(), ec) || ec)
    return Error<runtime::AssetId>("editor.png_destination");
  // Copying to a unique target may still race with another process. The
  // project folder's writer lock only synchronizes cooperating editors.
  if (!std::filesystem::copy_file(external, target.Value(),
                                  std::filesystem::copy_options::none, ec) ||
      ec)
    return Error<runtime::AssetId>("editor.png_copy");
  const auto result = ImportProjectPng(project, content_relative);
  if (!result) {
    std::filesystem::remove(target.Value(), ec);
  }
  return result;
}
} // namespace epidemic::game::editor
