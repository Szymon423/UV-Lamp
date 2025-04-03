#pragma once
#include <stdint.h>
#include <algorithm>
#include <vector>

namespace Graphics {

    /// @brief Definicja głębi kolorów
    enum class ColorDepth : uint8_t {
        RGB444, // (12 bitów)
        RGB565, // (16 bitów), 
        RGB666  // (18 bitów)
    };
    
    /// @brief Struktura reprezentująca kolor RGB
    struct RGB {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    
        RGB() : r(0), g(0), b(0) {}
        RGB(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
    
        // Konwersja do wartości odpowiadającej głębi kolorów
        uint8_t toColorDepth(ColorDepth depth) const;

        // Konwersja z wartości odpowiadającej głębi kolorów
        static RGB fromColorDepth(uint8_t value, ColorDepth depth);
    };
   
    /// @brief Klasa obsługująca płótno
    class Canvas {
    private:
        uint16_t width;                 // Canvas width
        uint16_t height;                // Canvas Height
        ColorDepth colorDepth;          // Canvas color depth
        std::vector<uint8_t> buffer;    // vector of bytes with pixels
        
        /// @brief Function to calculate bytes per pixel based on color depth
        /// @return number of bytes per pixel
        uint8_t getBytesPerPixel() const;
        
        /// @brief Function to calculate required buffer size
        /// @return buffer size in bytes
        size_t calculateBufferSize() const;

        /// @brief Function to set pixel color in buffer
        /// @param x X coordinate of pixel
        /// @param y Y coordinate of pixel
        /// @param color color to set
        void setPixelInBuffer(uint16_t x, uint16_t y, uint8_t color);

        /// @brief Function to read color of pixel in buffer
        /// @param x X coordinate of pixel
        /// @param y Y coordinate of pixel
        /// @return color of pixel
        RGB getPixelFromBuffer(uint16_t x, uint16_t y) const;
    
    public:
        /// @brief Contructor of Canvas
        /// @param w Width of canvas
        /// @param h Height of canvas
        /// @param depth color depth to use
        Canvas(uint16_t w, uint16_t h, ColorDepth depth);
        
        /// @brief Destructor
        ~Canvas();
    
        // Remove unwanted stuff
        Canvas(const Canvas&) = delete;
        Canvas& operator=(const Canvas&) = delete;
    
        /// @brief Function to get Width of canvas
        /// @return width of canvas
        uint16_t getWidth() const { return width; }
        
        /// @brief Function to get Height of canvas
        /// @return height of canvas
        uint16_t getHeight() const { return height; }
        
        /// @brief Function to get color depth of canvas
        /// @return color depth of canvas
        ColorDepth getColorDepth() const { return colorDepth; }
        
        /// @brief Function to get pointer to raw buffer of canvas
        /// @return raw buffer of canvas
        const uint8_t* getBuffer() const { return buffer.data(); }
        
        /// @brief Function to get size in bytes 
        /// @return 
        uint16_t getBufferSize() const { return buffer.size(); }
    
        /// @brief Function to clear canvas and set it to one color
        /// @param color color to set 
        void clear(const RGB& color = RGB());
        
        /// @brief Function to draw single pixel of given color
        /// @param x X coordinate of pixel 
        /// @param y Y coordinate of pixel
        /// @param color color of pixel
        void drawPixel(uint16_t x, uint16_t y, const RGB& color);

        /// @brief Function to get color from pixel
        /// @param x X coordinate of pixel 
        /// @param y Y coordinate of pixel
        /// @return color of pixel
        RGB getPixel(uint16_t x, uint16_t y) const;
    
        /// @brief Function to draw line from point (x0, y0) to (x1, y1) of given thickness and color
        /// @param x0 X coordinate of starting point
        /// @param y0 Y coordinate of starting point
        /// @param x1 X coordinate of finish point
        /// @param y1 y coordinate of finish point
        /// @param color color of line
        /// @param thickness thickness of line in pixels
        void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const RGB& color, uint8_t thickness = 1);

        
    };
} // namespace Graphics