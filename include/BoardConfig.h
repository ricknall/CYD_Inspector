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

// Expected XPT2046-compatible touch wiring on the classic single-USB CYD.
// These pins are used only after the user explicitly selects profile 1.
inline constexpr int TOUCH_MOSI = 32;
inline constexpr int TOUCH_MISO = 39;
inline constexpr int TOUCH_SCLK = 25;
inline constexpr int TOUCH_CS   = 33;
inline constexpr int TOUCH_IRQ  = 36;

// Confirmed active-low RGB LED channels on the tested classic single-USB CYD.
// GPIO 4 = red, GPIO 16 = green, and GPIO 17 = blue.
inline constexpr int RGB_CANDIDATE_1 = 4;
inline constexpr int RGB_CANDIDATE_2 = 16;
inline constexpr int RGB_CANDIDATE_3 = 17;

// Confirmed analog light-sensor connection on the tested classic single-USB
// CYD. GPIO 34 is input-only, so this reader never drives the pin. The divider
// is inverted: darker conditions produce higher raw ADC readings.
inline constexpr int LIGHT_SENSOR_PIN = 34;

}  // namespace cyd
