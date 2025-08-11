//
// Created by Ayaphis on 25-8-7.
//

#define UNICODE
#include <windows.h>
#include <wincodec.h>
#include "ImageHelp.h"


BOOL Ready = FALSE;
UINT width, height;
IWICImagingFactory *pFactory = NULL;
HRESULT hr = S_OK;

HRESULT InitializeWICImagingFactory() {
    hr = CoCreateInstance(
        &CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory,
        (void **)&pFactory
        );
    Ready = TRUE;
    return hr;
}

HBITMAP LoadBitmapFromStream(IWICStream *pStream) {
    if (!Ready) InitializeWICImagingFactory();
    HBITMAP hBitmap = NULL;

    HDC hDC = CreateCompatibleDC(NULL);

    IWICBitmapDecoder *pDecoder = NULL;
    IWICBitmapFrameDecode *pFrame = NULL;
    IWICFormatConverter *pConverter = NULL;

    if (SUCCEEDED(hr)) {
        // create decoder
        hr = pFactory->lpVtbl->CreateDecoderFromStream(
            pFactory,
            (IStream *)pStream,
            NULL,
            WICDecodeMetadataCacheOnLoad,
            &pDecoder
            );
    }

    if (SUCCEEDED(hr)) {
        hr = pFactory->lpVtbl->CreateFormatConverter(pFactory, &pConverter);
    }

    if (SUCCEEDED(hr)) {
        hr = pDecoder->lpVtbl->GetFrame(pDecoder,0,&pFrame);
    }

    if (SUCCEEDED(hr)) {
        hr = pConverter->lpVtbl->Initialize(
            pConverter,
            (IWICBitmapSource*)pFrame,
            &GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.05,
            WICBitmapPaletteTypeCustom
            );
    }

    if (SUCCEEDED(hr)) {
        hr = pFrame->lpVtbl->GetSize(pFrame, &width, &height);
    }

    if (SUCCEEDED(hr)) {
        BITMAPINFO bmi;
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = (LONG)width;
        bmi.bmiHeader.biHeight = (LONG)-height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void *pImageBits = NULL;
        hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pImageBits, NULL, 0);

        if (hBitmap && pImageBits) {
            hr = pConverter->lpVtbl->CopyPixels(
                pConverter,
                NULL,
                width * 4,
                width * height * 4,
                (BYTE *)pImageBits
                );
        }
    }
    if (pConverter) pConverter->lpVtbl->Release(pConverter);
    if (pDecoder) pDecoder->lpVtbl->Release(pDecoder);
    if (pFrame) pFrame->lpVtbl->Release(pFrame);
    DeleteDC(hDC);

    return hBitmap;
}

HBITMAP LoadBitmapFromRC(LPWSTR lpwstr) {
    if (!Ready) InitializeWICImagingFactory();
    HBITMAP hBitmap = NULL;
    HINSTANCE hInstance = GetModuleHandle(NULL);

    IWICStream *pStream = NULL;

    HRSRC imageResHandle = NULL;
    HGLOBAL imageResDataHandle = NULL;
    void *pImageData = NULL;
    DWORD imageSize = 0;

    imageResHandle = FindResource(NULL, lpwstr, RT_RCDATA);
    hr = (imageResHandle != NULL) ? S_OK : E_FAIL;

    if (SUCCEEDED(hr)) {
        imageResDataHandle = LoadResource(hInstance, imageResHandle);
        hr = (imageResDataHandle != NULL) ? S_OK : E_FAIL;
    }
    if (SUCCEEDED(hr)) {
        pImageData = LoadResource(hInstance, imageResHandle);
        hr = (pImageData != NULL) ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr)) {
        imageSize = SizeofResource(hInstance, imageResHandle);
        hr = (imageSize != 0) ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr)) {
        // create stream
        hr = pFactory->lpVtbl->CreateStream(pFactory,&pStream);
    }

    if (SUCCEEDED(hr)) {
        // initialize stream from memory
        hr = pStream->lpVtbl->InitializeFromMemory(pStream, pImageData, imageSize);
    }

    if (SUCCEEDED(hr)) {
        hBitmap = LoadBitmapFromStream(pStream);
    }

    return hBitmap;
}

HBITMAP LoadBitmapFromFile(const TCHAR filename[]) {
    if (!Ready) InitializeWICImagingFactory();
    HBITMAP hBitmap = NULL;

    IWICStream *pStream = NULL;

    TCHAR* extFile;
    TCHAR fullPath[MAX_PATH];

    GetFullPathName(filename, MAX_PATH, fullPath, &extFile);

    hr = pFactory->lpVtbl->CreateStream(pFactory, &pStream);

    if (SUCCEEDED(hr)) {
        hr = pStream->lpVtbl->InitializeFromFilename(pStream, fullPath, GENERIC_READ);
    }

    if (SUCCEEDED(hr)) {
        hBitmap = LoadBitmapFromStream(pStream);
    }

    return hBitmap;
}
