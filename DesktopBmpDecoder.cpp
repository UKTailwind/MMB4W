// Independent Windows BMP adapter for the MMB4W desktop port.
// Distributed under the MMB4W redistribution terms in Readme.
// This uses Windows GDI and contains no code from the embedded BMP decoder.
#define _CRT_DECLARE_NONSTDC_NAMES 0
#include "MainThread.h"
#include "MMBasic_Includes.h"
#include <io.h>
#include <stdio.h>
#include <stdint.h>

namespace {
constexpr int MaximumDimension = 16384;
constexpr uint64_t MaximumPixels = 16ULL * 1024 * 1024;
constexpr int64_t MaximumFileBytes = 128LL * 1024 * 1024;

// No interpreter calls are made here: every GDI handle is released before the
// caller can report an error or CheckAbort can leave through BASIC's longjmp.
bool ReadWindowsBitmap(FILE* file, uint32_t* pixels, int width, int height) {
    const intptr_t native = _get_osfhandle(_fileno(file));
    if (native == -1) return false;
    wchar_t filename[32768];
    const DWORD count = GetFinalPathNameByHandleW(reinterpret_cast<HANDLE>(native),
        filename, static_cast<DWORD>(sizeof(filename) / sizeof(filename[0])),
        FILE_NAME_OPENED | VOLUME_NAME_DOS);
    if (!count || count >= sizeof(filename) / sizeof(filename[0])) return false;

    HBITMAP bitmap = static_cast<HBITMAP>(LoadImageW(nullptr, filename,
        IMAGE_BITMAP, width, height, LR_LOADFROMFILE | LR_CREATEDIBSECTION));
    if (!bitmap) return false;
    BITMAP object = {};
    bool success = GetObjectW(bitmap, sizeof(object), &object) == sizeof(object) &&
        object.bmWidth == width && object.bmHeight == height;
    HDC dc = success ? CreateCompatibleDC(nullptr) : nullptr;
    if (!dc) success = false;
    if (success) {
        BITMAPINFO output = {};
        output.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        output.bmiHeader.biWidth = width;
        output.bmiHeader.biHeight = -height; // Request the top row first.
        output.bmiHeader.biPlanes = 1;
        output.bmiHeader.biBitCount = 32;
        output.bmiHeader.biCompression = BI_RGB;
        success = GetDIBits(dc, bitmap, 0, static_cast<UINT>(height), pixels,
            &output, DIB_RGB_COLORS) == height;
    }
    if (dc) DeleteDC(dc);
    DeleteObject(bitmap);
    return success;
}
}

extern "C" uint8_t BMP_bDecode(int x, int y, int fnbr) {
    if (fnbr < 1 || fnbr > MAXOPENFILES || !FileTable[fnbr].fptr)
        return 1;
    FILE* file = FileTable[fnbr].fptr;
    BITMAPFILEHEADER fileHeader = {};
    BITMAPINFOHEADER imageHeader = {};
    const int64_t fileBytes = _filelengthi64(_fileno(file));
    const int64_t savedPosition = _ftelli64(file);
    bool valid = fileBytes >= sizeof(fileHeader) + sizeof(imageHeader) &&
        fileBytes <= MaximumFileBytes && savedPosition >= 0 &&
        _fseeki64(file, 0, SEEK_SET) == 0 &&
        fread(&fileHeader, sizeof(fileHeader), 1, file) == 1 &&
        fread(&imageHeader, sizeof(imageHeader), 1, file) == 1;
    if (savedPosition >= 0 && _fseeki64(file, savedPosition, SEEK_SET) != 0) valid = false;
    const int64_t imageHeight = imageHeader.biHeight < 0 ?
        -static_cast<int64_t>(imageHeader.biHeight) : imageHeader.biHeight;
    valid = valid && fileHeader.bfType == 0x4D42 &&
        imageHeader.biSize >= sizeof(BITMAPINFOHEADER) && imageHeader.biSize <= 124 &&
        imageHeader.biWidth > 0 && imageHeader.biWidth <= MaximumDimension &&
        imageHeight > 0 && imageHeight <= MaximumDimension && imageHeader.biPlanes == 1 &&
        static_cast<uint64_t>(imageHeader.biWidth) * imageHeight <= MaximumPixels &&
        fileHeader.bfOffBits >= sizeof(BITMAPFILEHEADER) + imageHeader.biSize &&
        fileHeader.bfOffBits < fileBytes;
    const WORD depth = imageHeader.biBitCount;
    valid = valid && (depth == 1 || depth == 4 || depth == 8 ||
        depth == 16 || depth == 24 || depth == 32);
    const DWORD compression = imageHeader.biCompression;
    valid = valid && (compression == BI_RGB ||
        (compression == BI_BITFIELDS && (depth == 16 || depth == 32)) ||
        (compression == BI_RLE4 && depth == 4 && imageHeader.biHeight > 0) ||
        (compression == BI_RLE8 && depth == 8 && imageHeader.biHeight > 0));
    if (valid && (compression == BI_RGB || compression == BI_BITFIELDS)) {
        const uint64_t rowBytes = ((static_cast<uint64_t>(imageHeader.biWidth) * depth + 31) / 32) * 4;
        valid = rowBytes * imageHeight <= static_cast<uint64_t>(fileBytes - fileHeader.bfOffBits);
    }
    if (!valid) return 1;

    const int width = imageHeader.biWidth;
    const int height = static_cast<int>(imageHeight);
    // Interpreter-owned memory is reclaimed even when CheckAbort/error longjmps.
    // Allocation happens before acquiring any GDI resource.
    uint32_t* pixels = static_cast<uint32_t*>(GetTempMemory(width * height * 4));
    if (!ReadWindowsBitmap(file, pixels, width, height))
        return 2;

    const int64_t pageWidth = PageTable[WritePage].xmax;
    const int64_t pageHeight = PageTable[WritePage].ymax;
    const int64_t firstColumn = x < 0 ? -static_cast<int64_t>(x) : 0;
    const int64_t lastColumn = pageWidth - x < width ? pageWidth - x : width;
    const int64_t firstRow = y < 0 ? -static_cast<int64_t>(y) : 0;
    const int64_t lastRow = pageHeight - y < height ? pageHeight - y : height;
    for (int64_t row = firstRow; row < lastRow; ++row) {
        CheckAbort();
        for (int64_t column = firstColumn; column < lastColumn; ++column) {
            const uint32_t dib = pixels[row * width + column];
            // GDI returns B,G,R,unused bytes; MMBasic uses Windows RGB order.
            const uint32_t colour = 0xFF000000U | RGB((dib >> 16) & 255, (dib >> 8) & 255, dib & 255);
            DrawPixel(static_cast<int>(x + column), static_cast<int>(y + row), colour);
        }
    }
    return 0;
}
