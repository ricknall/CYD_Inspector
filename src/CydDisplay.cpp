#include "CydDisplay.h"

#include <pgmspace.h>

#include "BoardConfig.h"

namespace {

struct Glyph {
  char character;
  uint8_t rows[7];
};

// Compact 5x7 glyphs: digits, uppercase letters, and punctuation used by this
// utility.
const Glyph FONT[] PROGMEM = {
    {' ', {0, 0, 0, 0, 0, 0, 0}},
    {'-', {0, 0, 0, 31, 0, 0, 0}},
    {'.', {0, 0, 0, 0, 0, 12, 12}},
    {':', {0, 12, 12, 0, 12, 12, 0}},
    {'/', {1, 2, 4, 8, 16, 0, 0}},
    {'(', {2, 4, 8, 8, 8, 4, 2}},
    {')', {8, 4, 2, 2, 2, 4, 8}},
    {'0', {14, 17, 19, 21, 25, 17, 14}},
    {'1', {4, 12, 4, 4, 4, 4, 14}},
    {'2', {14, 17, 1, 2, 4, 8, 31}},
    {'3', {30, 1, 1, 14, 1, 1, 30}},
    {'4', {2, 6, 10, 18, 31, 2, 2}},
    {'5', {31, 16, 16, 30, 1, 1, 30}},
    {'6', {14, 16, 16, 30, 17, 17, 14}},
    {'7', {31, 1, 2, 4, 8, 8, 8}},
    {'8', {14, 17, 17, 14, 17, 17, 14}},
    {'9', {14, 17, 17, 15, 1, 1, 14}},
    {'A', {14, 17, 17, 31, 17, 17, 17}},
    {'B', {30, 17, 17, 30, 17, 17, 30}},
    {'C', {14, 17, 16, 16, 16, 17, 14}},
    {'D', {30, 17, 17, 17, 17, 17, 30}},
    {'E', {31, 16, 16, 30, 16, 16, 31}},
    {'F', {31, 16, 16, 30, 16, 16, 16}},
    {'G', {14, 17, 16, 23, 17, 17, 15}},
    {'H', {17, 17, 17, 31, 17, 17, 17}},
    {'I', {14, 4, 4, 4, 4, 4, 14}},
    {'J', {7, 2, 2, 2, 2, 18, 12}},
    {'K', {17, 18, 20, 24, 20, 18, 17}},
    {'L', {16, 16, 16, 16, 16, 16, 31}},
    {'M', {17, 27, 21, 21, 17, 17, 17}},
    {'N', {17, 25, 21, 19, 17, 17, 17}},
    {'O', {14, 17, 17, 17, 17, 17, 14}},
    {'P', {30, 17, 17, 30, 16, 16, 16}},
    {'Q', {14, 17, 17, 17, 21, 18, 13}},
    {'R', {30, 17, 17, 30, 20, 18, 17}},
    {'S', {15, 16, 16, 14, 1, 1, 30}},
    {'T', {31, 4, 4, 4, 4, 4, 4}},
    {'U', {17, 17, 17, 17, 17, 17, 14}},
    {'V', {17, 17, 17, 17, 17, 10, 4}},
    {'W', {17, 17, 17, 21, 21, 21, 10}},
    {'X', {17, 17, 10, 4, 10, 17, 17}},
    {'Y', {17, 17, 10, 4, 4, 4, 4}},
    {'Z', {31, 1, 2, 4, 8, 16, 31}},
};

constexpr size_t FONT_SIZE = sizeof(FONT) / sizeof(FONT[0]);
constexpr uint32_t DISPLAY_READ_FREQUENCY = 2'000'000;

bool allBytesEqual(const uint8_t* values,
                   const size_t length,
                   const uint8_t expected) {
  for (size_t index = 0; index < length; ++index) {
    if (values[index] != expected) {
      return false;
    }
  }

  return true;
}

bool usefulReadback(const uint8_t* values, const size_t length) {
  return !allBytesEqual(values, length, 0x00) &&
         !allBytesEqual(values, length, 0xFF);
}

bool containsSequence(const uint8_t* values,
                      const size_t length,
                      const uint8_t* sequence,
                      const size_t sequenceLength) {
  if (sequenceLength == 0 || sequenceLength > length) {
    return false;
  }

  for (size_t start = 0; start <= length - sequenceLength; ++start) {
    bool match = true;

    for (size_t index = 0; index < sequenceLength; ++index) {
      if (values[start + index] != sequence[index]) {
        match = false;
        break;
      }
    }

    if (match) {
      return true;
    }
  }

  return false;
}

bool registerContains(const uint8_t* values,
                      const size_t length,
                      const uint8_t expected) {
  for (size_t index = 0; index < length; ++index) {
    if (values[index] == expected) {
      return true;
    }
  }

  return false;
}

}  // namespace

const char* displayControllerName(const DisplayController controller) {
  switch (controller) {
    case DisplayController::St7789V:
      return "ST7789V";

    case DisplayController::Ili9341:
      return "ILI9341";

    case DisplayController::UnknownReadable:
      return "UNKNOWN";

    case DisplayController::ReadbackUnavailable:
    default:
      return "NO READBACK";
  }
}

CydDisplay::CydDisplay() : spi_(VSPI) {}

void CydDisplay::begin() {
  pinMode(cyd::LCD_CS, OUTPUT);
  pinMode(cyd::LCD_DC, OUTPUT);
  pinMode(cyd::LCD_BL, OUTPUT);
  pinMode(cyd::LCD_MISO, INPUT_PULLUP);

  digitalWrite(cyd::LCD_CS, HIGH);
  digitalWrite(cyd::LCD_BL, HIGH);

  spi_.begin(cyd::LCD_SCLK, cyd::LCD_MISO, cyd::LCD_MOSI, cyd::LCD_CS);
  spi_.beginTransaction(
      SPISettings(cyd::SPI_FREQUENCY, MSBFIRST, SPI_MODE0));

  initializeCompatibleProfile();
}

void CydDisplay::command(const uint8_t value) {
  digitalWrite(cyd::LCD_DC, LOW);
  digitalWrite(cyd::LCD_CS, LOW);
  spi_.transfer(value);
  digitalWrite(cyd::LCD_CS, HIGH);
}

void CydDisplay::data8(const uint8_t value) {
  digitalWrite(cyd::LCD_DC, HIGH);
  digitalWrite(cyd::LCD_CS, LOW);
  spi_.transfer(value);
  digitalWrite(cyd::LCD_CS, HIGH);
}

void CydDisplay::data(const uint8_t* values, const size_t length) {
  digitalWrite(cyd::LCD_DC, HIGH);
  digitalWrite(cyd::LCD_CS, LOW);
  spi_.writeBytes(values, length);
  digitalWrite(cyd::LCD_CS, HIGH);
}

void CydDisplay::readCommand(const uint8_t commandValue,
                             uint8_t* buffer,
                             const size_t length) {
  // The normal display-writing transaction remains open between calls. Close
  // it temporarily, read at a conservative clock rate, then restore it.
  spi_.endTransaction();

  spi_.beginTransaction(
      SPISettings(DISPLAY_READ_FREQUENCY, MSBFIRST, SPI_MODE0));

  digitalWrite(cyd::LCD_CS, LOW);

  digitalWrite(cyd::LCD_DC, LOW);
  spi_.transfer(commandValue);

  digitalWrite(cyd::LCD_DC, HIGH);
  delayMicroseconds(1);

  for (size_t index = 0; index < length; ++index) {
    buffer[index] = spi_.transfer(0x00);
  }

  digitalWrite(cyd::LCD_CS, HIGH);
  spi_.endTransaction();

  spi_.beginTransaction(
      SPISettings(cyd::SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
}

DisplayProbe CydDisplay::probeController() {
  DisplayProbe result;

  // 0x04: Read Display Identification Information.
  readCommand(0x04, result.id04, sizeof(result.id04));

  // 0xD3: ILI9341 extended device-code read.
  readCommand(0xD3, result.idD3, sizeof(result.idD3));

  // 0xDA/0xDB/0xDC: one-byte ID registers. ST7789V commonly reports
  // 0x85, 0x85, 0x52. Depending on the controller and serial timing, the
  // useful byte can appear in either position of our two-byte capture.
  readCommand(0xDA, result.idDA, sizeof(result.idDA));
  readCommand(0xDB, result.idDB, sizeof(result.idDB));
  readCommand(0xDC, result.idDC, sizeof(result.idDC));

  // Standard status registers prove whether the serial read path works even
  // when the controller declines to provide a useful identity value.
  readCommand(0x0B, result.madctl, sizeof(result.madctl));
  readCommand(0x0C, result.pixelFormat, sizeof(result.pixelFormat));

  static constexpr uint8_t ST7789_ID04[] = {0x85, 0x85, 0x52};
  static constexpr uint8_t ILI9341_D3[] = {0x93, 0x41};

  const bool st7789Signature =
      containsSequence(result.id04,
                       sizeof(result.id04),
                       ST7789_ID04,
                       sizeof(ST7789_ID04)) ||
      (registerContains(result.idDA, sizeof(result.idDA), 0x85) &&
       registerContains(result.idDB, sizeof(result.idDB), 0x85) &&
       registerContains(result.idDC, sizeof(result.idDC), 0x52));

  const bool ili9341Signature =
      containsSequence(result.idD3,
                       sizeof(result.idD3),
                       ILI9341_D3,
                       sizeof(ILI9341_D3));

  if (ili9341Signature) {
    result.controller = DisplayController::Ili9341;
  } else if (st7789Signature) {
    result.controller = DisplayController::St7789V;
  } else {
    const bool readable =
        usefulReadback(result.id04, sizeof(result.id04)) ||
        usefulReadback(result.idD3, sizeof(result.idD3)) ||
        usefulReadback(result.idDA, sizeof(result.idDA)) ||
        usefulReadback(result.idDB, sizeof(result.idDB)) ||
        usefulReadback(result.idDC, sizeof(result.idDC)) ||
        usefulReadback(result.madctl, sizeof(result.madctl)) ||
        usefulReadback(result.pixelFormat, sizeof(result.pixelFormat));

    result.controller = readable
                            ? DisplayController::UnknownReadable
                            : DisplayController::ReadbackUnavailable;
  }

  return result;
}

void CydDisplay::initializeCompatibleProfile() {
  command(0x01);  // Software reset
  delay(150);
  command(0x11);  // Sleep out
  delay(120);
  command(0x3A);  // Pixel format
  data8(0x55);    // RGB565
  command(0x36);  // Memory access control
  data8(0x20);    // Landscape orientation used by these tested CYDs
  command(0x21);  // Display inversion on
  command(0x13);  // Normal display mode
  command(0x29);  // Display on
  delay(20);
}

void CydDisplay::setWindow(const uint16_t x0,
                           const uint16_t y0,
                           const uint16_t x1,
                           const uint16_t y1) {
  command(0x2A);
  const uint8_t columns[] = {
      static_cast<uint8_t>(x0 >> 8),
      static_cast<uint8_t>(x0),
      static_cast<uint8_t>(x1 >> 8),
      static_cast<uint8_t>(x1)};
  data(columns, sizeof(columns));

  command(0x2B);
  const uint8_t rows[] = {
      static_cast<uint8_t>(y0 >> 8),
      static_cast<uint8_t>(y0),
      static_cast<uint8_t>(y1 >> 8),
      static_cast<uint8_t>(y1)};
  data(rows, sizeof(rows));

  command(0x2C);
}

void CydDisplay::fill(const uint16_t color) {
  setWindow(0, 0, cyd::DISPLAY_WIDTH - 1, cyd::DISPLAY_HEIGHT - 1);

  digitalWrite(cyd::LCD_DC, HIGH);
  digitalWrite(cyd::LCD_CS, LOW);

  const uint8_t high = static_cast<uint8_t>(color >> 8);
  const uint8_t low = static_cast<uint8_t>(color);
  const uint32_t pixelCount =
      static_cast<uint32_t>(cyd::DISPLAY_WIDTH) * cyd::DISPLAY_HEIGHT;

  for (uint32_t index = 0; index < pixelCount; ++index) {
    spi_.transfer(high);
    spi_.transfer(low);
  }

  digitalWrite(cyd::LCD_CS, HIGH);
}

const uint8_t* CydDisplay::glyph(char character) {
  if (character >= 'a' && character <= 'z') {
    character -= 32;
  }

  for (size_t index = 0; index < FONT_SIZE; ++index) {
    if (static_cast<char>(pgm_read_byte(&FONT[index].character)) ==
        character) {
      return FONT[index].rows;
    }
  }

  return FONT[0].rows;
}

void CydDisplay::pixelRect(const int x,
                           const int y,
                           const int width,
                           const int height,
                           const uint16_t color) {
  if (x < 0 || y < 0 || width <= 0 || height <= 0 ||
      x + width > cyd::DISPLAY_WIDTH ||
      y + height > cyd::DISPLAY_HEIGHT) {
    return;
  }

  setWindow(x, y, x + width - 1, y + height - 1);
  digitalWrite(cyd::LCD_DC, HIGH);
  digitalWrite(cyd::LCD_CS, LOW);

  const uint8_t high = static_cast<uint8_t>(color >> 8);
  const uint8_t low = static_cast<uint8_t>(color);

  for (int index = 0; index < width * height; ++index) {
    spi_.transfer(high);
    spi_.transfer(low);
  }

  digitalWrite(cyd::LCD_CS, HIGH);
}

void CydDisplay::text(int x,
                      const int y,
                      const String& value,
                      const uint16_t color,
                      const int scale) {
  if (scale <= 0) {
    return;
  }

  for (const char character : value) {
    const uint8_t* characterGlyph = glyph(character);

    for (int row = 0; row < 7; ++row) {
      const uint8_t bits = pgm_read_byte(characterGlyph + row);

      for (int column = 0; column < 5; ++column) {
        if ((bits & (1U << (4 - column))) != 0) {
          pixelRect(x + column * scale,
                    y + row * scale,
                    scale,
                    scale,
                    color);
        }
      }
    }

    x += 6 * scale;
  }
}
