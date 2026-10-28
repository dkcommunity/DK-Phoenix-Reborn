#include "MapEditorPngWriter.h"

#include <windows.h>

namespace phoenix::tools::map_editor
{
    bool WritePngImage(const std::filesystem::path& path, const std::uint32_t width,
        const std::uint32_t height, const PngPixelWriter& writePixels)
    {
        IWICImagingFactory* factory = nullptr;
        IWICStream* stream = nullptr;
        IWICBitmapEncoder* encoder = nullptr;
        IWICBitmapFrameEncode* frame = nullptr;
        IPropertyBag2* properties = nullptr;
        bool succeeded = false;
        if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory))) && SUCCEEDED(factory->CreateStream(&stream)) &&
            SUCCEEDED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE)) &&
            SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) &&
            SUCCEEDED(encoder->Initialize(stream, WICBitmapEncoderNoCache)) &&
            SUCCEEDED(encoder->CreateNewFrame(&frame, &properties)) &&
            SUCCEEDED(frame->Initialize(properties)) && SUCCEEDED(frame->SetSize(width, height)))
        {
            auto format = GUID_WICPixelFormat32bppBGRA;
            if (SUCCEEDED(frame->SetPixelFormat(&format)) && format == GUID_WICPixelFormat32bppBGRA &&
                writePixels(frame) && SUCCEEDED(frame->Commit()) && SUCCEEDED(encoder->Commit()))
                succeeded = true;
        }
        if (properties != nullptr) properties->Release();
        if (frame != nullptr) frame->Release();
        if (encoder != nullptr) encoder->Release();
        if (stream != nullptr) stream->Release();
        if (factory != nullptr) factory->Release();
        return succeeded;
    }
}
