#include "graphics.hpp"

namespace Graphics {

// Implementacja metod RGB
uint8_t RGB::toColorDepth(ColorDepth depth) const {
    switch(depth) {
        case ColorDepth::RGB444: {
            // 4 bity na kanał (0-15)
            uint8_t r4 = r >> 4;    // Konwersja 8 bitów do 4 bitów
            uint8_t g4 = g >> 4;
            uint8_t b4 = b >> 4;
            return (r4 << 4) | b4;  // Zapisujemy R i B, G w osobnym bajcie
        }
        case ColorDepth::RGB565: {
            // 5 bitów R (0-31), 6 bitów G (0-63), 5 bitów B (0-31)
            uint8_t r5 = r >> 3;    // Konwersja 8 bitów do 5 bitów
            uint8_t g6 = g >> 2;    // Konwersja 8 bitów do 6 bitów
            uint8_t b5 = b >> 3;    // Konwersja 8 bitów do 5 bitów
            return (r5 << 3) | b5;  // Zapisujemy R i B, G w osobnym bajcie
        }
        case ColorDepth::RGB666: {
            // 6 bitów na kanał (0-63)
            uint8_t r6 = r >> 2;    // Konwersja 8 bitów do 6 bitów
            uint8_t g6 = g >> 2;
            uint8_t b6 = b >> 2;
            return (r6 << 2) | b6;  // Zapisujemy R i B, G w osobnym bajcie
        }
        default: {
            return 0;
        }
    }
}

RGB RGB::fromColorDepth(uint8_t value, ColorDepth depth) {
    switch(depth) {
        case ColorDepth::RGB444: {
            uint8_t r4 = (value >> 4) & 0x0F;
            uint8_t b4 = value & 0x0F;
            // Rozszerzenie do 8 bitów przez powielenie
            uint8_t r = (r4 << 4) | r4;
            uint8_t b = (b4 << 4) | b4;
            return RGB(r, 0, b); // G musi być ustawione z drugiego bajtu
        }
        
        case ColorDepth::RGB565: {
            uint8_t r5 = (value >> 3) & 0x1F;
            uint8_t b5 = value & 0x07;
            // Rozszerzenie do 8 bitów
            uint8_t r = (r5 << 3) | (r5 >> 2);
            uint8_t b = (b5 << 3) | (b5 >> 2);
            return RGB(r, 0, b); // G musi być ustawione z drugiego bajtu
        }
        
        case ColorDepth::RGB666: {
            uint8_t r6 = (value >> 2) & 0x3F;
            uint8_t b6 = value & 0x03;
            // Rozszerzenie do 8 bitów
            uint8_t r = (r6 << 2) | (r6 >> 4);
            uint8_t b = (b6 << 2) | (b6 >> 4);
            return RGB(r, 0, b); // G musi być ustawione z drugiego bajtu
        }
        
        default:
            return RGB();
    }
}

Canvas::Canvas(uint16_t w, uint16_t h, ColorDepth depth)
    : width(w), height(h), colorDepth(depth) {
    buffer.resize(calculateBufferSize(), 0);
}

Canvas::~Canvas() = default;

uint8_t Canvas::getBytesPerPixel() const {
    switch(colorDepth) {
        case ColorDepth::RGB444: return 2;  // 12 bitów = 2 bajty
        case ColorDepth::RGB565: return 2;  // 16 bitów = 2 bajty
        case ColorDepth::RGB666: return 3;  // 18 bitów = 3 bajty
        default: return 0;
    }
}

size_t Canvas::calculateBufferSize() const {
    return static_cast<size_t>(width) * height * getBytesPerPixel();
}

void Canvas::setPixelInBuffer(uint16_t x, uint16_t y, uint8_t color) {
    if (x >= width || y >= height) return;
    
    size_t pixelOffset = (y * width + x) * getBytesPerPixel();
    buffer[pixelOffset] = color;
}

RGB Canvas::getPixelFromBuffer(uint16_t x, uint16_t y) const {
    if (x >= width || y >= height) return RGB();
    
    size_t pixelOffset = (y * width + x) * getBytesPerPixel();
    return RGB::fromColorDepth(buffer[pixelOffset], colorDepth);
}

void Canvas::clear(const RGB& color) {
    uint8_t colorValue = color.toColorDepth(colorDepth);
    for (uint16_t y = 0; y < height; ++y) {
        for (uint16_t x = 0; x < width; ++x) {
            setPixelInBuffer(x, y, colorValue);
        }
    }
}

void Canvas::drawPixel(uint16_t x, uint16_t y, const RGB& color) {
    if (x >= width || y >= height) return;
    setPixelInBuffer(x, y, color.toColorDepth(colorDepth));
}

RGB Canvas::getPixel(uint16_t x, uint16_t y) const {
    if (x >= width || y >= height) return RGB();
    return getPixelFromBuffer(x, y);
}

void Canvas::drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const RGB& color, uint8_t thickness) {
    // Implementacja algorytmu Bresenhama z obsługą grubości
    int16_t dx = abs(static_cast<int16_t>(x1) - x0);
    int16_t dy = abs(static_cast<int16_t>(y1) - y0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx - dy;
    
    // Oblicz offset dla grubości
    int16_t thicknessOffset = thickness / 2;

    while (true) {
        // Rysuj piksel z uwzględnieniem grubości
        for (int16_t tx = -thicknessOffset; tx <= thicknessOffset; ++tx) {
            for (int16_t ty = -thicknessOffset; ty <= thicknessOffset; ++ty) {
                int16_t px = x0 + tx;
                int16_t py = y0 + ty;
                if (px >= 0 && px < width && py >= 0 && py < height) {
                    drawPixel(px, py, color);
                }
            }
        }

        if (x0 == x1 && y0 == y1) break;
        
        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

} // namespace Graphics