#pragma once

#include <Arduino.h>
#include <SPI.h>

enum class DisplayController : uint8_t {
  St7789V,
  Ili9341,
  UnknownReadable,
  ReadbackUnavailable
};

struct DisplayProbe {
  DisplayController controller = DisplayController::ReadbackUnavailable;

  uint8_t id04[4] = {};
  uint8_t idD3[4] = {};
  uint8_t idDA[2] = {};
  uint8_t idDB[2] = {};
  uint8_t idDC[2] = {};
  uint8_t madctl[2] = {};
  uint8_t pixelFormat[2] = {};
};

const char* displayControllerName(DisplayController controller);

class CydDisplay {
 public:
  CydDisplay();

  void begin();
  DisplayProbe probeController();

  void fill(uint16_t color);
  void text(int x, int y, const String& value, uint16_t color, int scale = 2);

 private:
  SPIClass spi_;

  void command(uint8_t value);
  void data8(uint8_t value);
  void data(const uint8_t* values, size_t length);
  void readCommand(uint8_t commandValue, uint8_t* buffer, size_t length);

  void initializeCompatibleProfile();
  void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
  void pixelRect(int x, int y, int width, int height, uint16_t color);

  static const uint8_t* glyph(char character);
};
