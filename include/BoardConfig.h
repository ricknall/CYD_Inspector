#pragma once

#include <Arduino.h>

namespace cyd {

// Display SPI bus used by the tested classic and two-USB CYD boards.
inline constexpr int LCD_MOSI = 13;
inline constexpr int LCD_MISO = 12;
inline constexpr int LCD_SCLK = 14;
inline constexpr int LCD_CS   = 15;
inline constexpr int LCD_DC   = 2;
inline constexpr int LCD_BL   = 21;

inline constexpr uint16_t DISPLAY_WIDTH  = 320;
inline constexpr uint16_t DISPLAY_HEIGHT = 240;
inline constexpr uint32_t SPI_FREQUENCY  = 40'000'000;

// MicroSD SPI bus.
inline constexpr int SD_MOSI = 23;
inline constexpr int SD_MISO = 19;
inline constexpr int SD_SCLK = 18;
inline constexpr int SD_CS   = 5;

inline constexpr uint32_t SD_SPI_FREQUENCY = 10'000'000;

}  // namespace cyd
