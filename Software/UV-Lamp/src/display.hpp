#pragma once

#include "graphics.hpp"

/// @brief Namespace for interaction with display
namespace Display {
    /// @brief Function to initialize Display
    void Initialize();

    /// @brief Function to send canvas to display
    /// @param canvas canvas with content to display
    void SetFrame(const Graphics::Canvas& canvas);
}