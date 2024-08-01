#pragma once

#include <TFT_eSPI.h>
#include <pixels_dice_interface.h>  // https://github.com/axlan/arduino-pixels-dice
#include "wled.h"

#include "roll_info.h"

#ifndef USER_SETUP_LOADED
  #ifndef TFT_WIDTH
    #error Please define TFT_WIDTH
  #endif
  #ifndef TFT_HEIGHT
    #error Please define TFT_HEIGHT
  #endif
  #ifndef TFT_DC
    #error Please define TFT_DC
  #endif
  #ifndef TFT_RST
    #error Please define TFT_RST
  #endif
  #ifndef LOAD_GLCD
    #error Please define LOAD_GLCD
  #endif
#endif
#ifndef TFT_BL
  #define TFT_BL -1
#endif
// Set this parameter to rotate the display. 1-3 rotate by 90,180,270 degrees.
#ifndef USERMOD_PIXELS_DICE_TRAY_ROTATION
  #define USERMOD_PIXELS_DICE_TRAY_ROTATION 0
#endif
// Ideally different sized displays would have their own layouts.
// This mod is only tested with 240x240 and 128x128 displays, so this
// simple rescaling is sufficient.
#ifndef USERMOD_PIXELS_DICE_TRAY_SCALE
  #if TFT_WIDTH < 200
    #define USERMOD_PIXELS_DICE_TRAY_SCALE 1
  #else
    #define USERMOD_PIXELS_DICE_TRAY_SCALE 2
  #endif
#endif

// How often we are redrawing screen
#ifndef USERMOD_PIXELS_DICE_TRAY_REFRESH_RATE_MS
  #define USERMOD_PIXELS_DICE_TRAY_REFRESH_RATE_MS 200
#endif

// Time with no updates before screen turns off (-1 to disable)
#ifndef USERMOD_PIXELS_DICE_TRAY_TIMEOUT_MS
  //#define USERMOD_PIXELS_DICE_TRAY_TIMEOUT_MS 2 * 60 * 1000
  #define USERMOD_PIXELS_DICE_TRAY_TIMEOUT_MS -1
#endif

#define WLED_DEBOUNCE_THRESHOLD \
  50  // only consider button input of at least 50ms as valid (debouncing)
#define WLED_LONG_PRESS \
  600  // long press if button is released after held for at least 600ms
#define WLED_DOUBLE_PRESS \
  350  // double press if another press within 350ms after a short press

extern int getSignalQuality(int rssi);

static constexpr size_t BLE_SCAN_DURATION_SEC = 4;
static constexpr size_t BLE_TIME_BETWEEN_SCANS_SEC = 5;

const uint8_t LIGHTNING_ICON_8X8[] PROGMEM = {
    0b00001111, 0b00010010, 0b00100100, 0b01001111,
    0b10000001, 0b11110010, 0b00010100, 0b00011000,
};

const uint8_t BATTERY_ICON_8X8[] PROGMEM = {
    0b00000000, 0b00000000, 0b11111110, 0b10101011,
    0b10101011, 0b11111110, 0b00000000, 0b00000000,
};

static constexpr uint8_t INVALID_ROLL = 0xFF;
static constexpr size_t NUM_DICE = 2;
// NOTE: The ordering is taken into account when referencing die 1 vs die 2.
static std::array<std::string, NUM_DICE> configured_die_names = {
    "Aurora",
    "Midnight",
};
// The ordering here matches configured_die_names.
static std::array<pixels::PixelsDieID, NUM_DICE> connected_die_ids = {0, 0};
static std::array<uint8_t, NUM_DICE> last_die_values = {INVALID_ROLL,
                                                        INVALID_ROLL};
static uint8_t last_die_value = INVALID_ROLL;

// The vectors to hold results queried from the library
// Since vectors allocate data, it's more efficient to keep reusing objects
// instead of declaring them on the stack
std::vector<pixels::PixelsDieID> dice_list;
pixels::RollUpdates roll_updates;
pixels::BatteryUpdates battery_updates;

TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);  // Invoke custom library

template <typename C, typename T>
static bool Contains(const C& container, T value) {
  return std::find(container.begin(), container.end(), value) !=
         container.end();
}

static void PrintLnInBox(const char* txt, uint32_t color) {
  int16_t sx = tft.getCursorX();
  int16_t sy = tft.getCursorY();
  tft.setCursor(sx + 2, sy);
  tft.print(txt);
  int16_t w = tft.getCursorX() - sx + 1;
  tft.println();
  int16_t h = tft.getCursorY() - sy - 1;
  tft.drawRect(sx, sy, w, h, color);
}

// These are updated in the main loop, but accessed by the effect functions as
// well. My understand is that both of these accesses should be running on the
// same "thread/task" since WLED doesn't directly create additional threads. The
// exception would be network callbacks and interrupts, but I don't beleive
// these accesses are triggered by those. If synchronization was needed, I could
// look at the example in `requestJSONBufferLock()`.
static pixels::RollUpdates dice_effect_state;

extern uint16_t mode_breath();
extern uint16_t mode_blends();
extern uint16_t running(uint32_t color1, uint32_t color2, bool theatre = false);
extern uint16_t mode_glitter();
extern uint16_t mode_gravcenter();

static uint16_t simple_roll() {
  if (!dice_effect_state.empty()) {
    // Only keep last state.
    if (dice_effect_state.size() > 1) {
      dice_effect_state[0] = dice_effect_state.back();
      dice_effect_state.resize(1);
    }

    auto roll = dice_effect_state.end()->second;

    if (roll.state != pixels::RollState::ON_FACE) {
      SEGMENT.fill(0);
    } else {
      uint16_t num_segments = float(roll.current_face + 1) / 20.0 * SEGLEN;
      for (int i = 0; i <= num_segments; i++) {
        SEGMENT.setPixelColor(i, SEGCOLOR(0));
      }
    }
  } else {
    SEGMENT.fill(0);
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_SIMPLE_DIE[] PROGMEM = "DieSimple@;!;;01";

static uint16_t pulse_roll() {
  if (!dice_effect_state.empty()) {
    // Only keep last state.
    if (dice_effect_state.size() > 1) {
      dice_effect_state[0] = dice_effect_state.back();
      dice_effect_state.resize(1);
    }

    auto roll = dice_effect_state.end()->second;

    if (roll.state != pixels::RollState::ON_FACE) {
      return mode_breath();
    } else {
      uint16_t ret = mode_blends();
      uint16_t num_segments = float(roll.current_face + 1) / 20.0 * SEGLEN;
      for (int i = num_segments; i < SEGLEN; i++) {
        SEGMENT.setPixelColor(i, SEGCOLOR(1));
      }
      return ret;
    }
  } else {
    return mode_breath();
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_PULSE_DIE[] PROGMEM =
    "DiePulse@!,!;!,!;!;01;sx=24,pal=50";

uint8_t target = 10;
static uint16_t check_roll() {
  if (!dice_effect_state.empty()) {
    // Only keep last state.
    if (dice_effect_state.size() > 1) {
      dice_effect_state[0] = dice_effect_state.back();
      dice_effect_state.resize(1);
    }

    auto roll = dice_effect_state.end()->second;

    if (roll.state != pixels::RollState::ON_FACE) {
      return running(SEGCOLOR(0), SEGCOLOR(2));
    } else {
      if (roll.current_face + 1 >= target) {
        return mode_glitter();
      } else {
        return mode_gravcenter();
      }
    }
  } else {
    return running(SEGCOLOR(0), SEGCOLOR(2));
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_CHECK_DIE[] PROGMEM =
    "DieCheck@!,!;1,2,3;!;01;pal=0,ix=128,m12=2,si=0";

void SetDefaultColors(uint8_t mode) {
  Segment& seg = strip.getMainSegment();
  switch (mode) {
    case FX_MODE_SIMPLE_D20:
      seg.setColor(0, GREEN);
      break;
    case FX_MODE_PULSE_D20:
      seg.setColor(0, GREEN);
      seg.setColor(1, RED);
      break;
    case FX_MODE_CHECK_D20:
      seg.setColor(0, RED);
      seg.setColor(1, 0);
      seg.setColor(2, GREEN);
      break;
  }
}

class RollCountWidget {
 private:
  // Could make configurable if needed.
  int16_t xs = 0;
  int16_t ys = 0;
  uint16_t border_color = TFT_RED;
  uint16_t bar_color = TFT_GREEN;
  uint16_t bar_width = 6;
  uint16_t max_bar_height = 60;
  unsigned roll_counts[20] = {0};
  unsigned total = 0;
  unsigned max_count = 0;

 public:
  RollCountWidget(int16_t xs = 0, int16_t ys = 0,
                  uint16_t border_color = TFT_RED,
                  uint16_t bar_color = TFT_GREEN, uint16_t bar_width = 6,
                  uint16_t max_bar_height = 60)
      : xs(xs),
        ys(ys),
        border_color(border_color),
        bar_color(bar_color),
        bar_width(bar_width),
        max_bar_height(max_bar_height) {}

  void Clear() {
    memset(roll_counts, 0, sizeof(roll_counts));
    total = 0;
    max_count = 0;
  }

  unsigned GetNumRolls() const { return total; }

  void AddRoll(unsigned val) {
    if (val > 19) {
      return;
    }
    roll_counts[val]++;
    total++;
    max_count = max(roll_counts[val], max_count);
  }

  void Draw() {
    // Add 2 pixels to lengths for boarder width.
    tft.drawRect(xs, ys, bar_width * 20 + 2, max_bar_height + 2, border_color);
    for (size_t i = 0; i < 20; i++) {
      if (roll_counts[i] > 0) {
        // Scale bar by highest count.
        uint16_t bar_height = round(float(roll_counts[i]) / float(max_count) *
                                    float(max_bar_height));
        // Add space between bars
        uint16_t padding = (bar_width > 1) ? 1 : 0;
        // Need to start from top of bar and draw down
        tft.fillRect(xs + 1 + bar_width * i,
                     ys + 1 + max_bar_height - bar_height, bar_width - padding,
                     bar_height, bar_color);
      }
    }
  }
};

enum class ButtonType { SINGLE, DOUBLE, LONG };

class MenuBase {
 public:
  virtual void Update() = 0;

  virtual void Draw(bool force_redraw) = 0;

  virtual void HandleButton(ButtonType type, uint8_t b) = 0;
};

class DiceStatusMenu : public MenuBase {
 public:
  DiceStatusMenu()
      : die_roll_counts{RollCountWidget{0, 20, TFT_BLUE, TFT_GREEN, 6, 40},
                        RollCountWidget{0, SECTION_HEIGHT + 20, TFT_BLUE,
                                        TFT_GREEN, 6, 40}} {}

  void Update() override {
    for (size_t i = 0; i < NUM_DICE; i++) {
      const auto die_id = connected_die_ids[i];
      const auto connected = die_id != 0;

      die_updated[i] |= connected != die_connection_status[i];
      die_connection_status[i] = connected;

      if (connected) {
        bool charging = false;
        for (const auto& battery : battery_updates) {
          if (battery.first == die_id) {
            if (die_battery[i].battery_level == INVALID_BATTERY ||
                battery.second.is_charging != die_battery[i].is_charging) {
              die_updated[i] = true;
            }
            die_battery[i] = battery.second;
          }
        }

        for (const auto& roll : roll_updates) {
          if (roll.first == die_id &&
              roll.second.state == pixels::RollState::ON_FACE) {
            die_roll_counts[i].AddRoll(roll.second.current_face);
            die_updated[i] = true;
          }
        }

        for (const auto& battery : battery_updates) {
          if (battery.first == die_id) {
            die_battery[i] = battery.second;
          }
        }
      }
    }
  }

  void Draw(bool force_redraw) override {
    // This could probably be optimized for partial redraws.
    for (size_t i = 0; i < NUM_DICE; i++) {
      const int16_t ys = SECTION_HEIGHT * i;
      const auto die_id = connected_die_ids[i];
      const auto connected = die_id != 0;
      // Screen updates might be slow, yield in case network task needs to do
      // work.
      yield();
      bool battery_update =
          connected && (millis() - last_update[i] > BATTERY_REFRESH_RATE_MS);
      if (force_redraw || die_updated[i] || battery_update) {
        last_update[i] = millis();
        tft.fillRect(0, ys, TFT_WIDTH, SECTION_HEIGHT, TFT_BLACK);
        tft.drawRect(0, ys, TFT_WIDTH, SECTION_HEIGHT, TFT_BLUE);
        if (!connected) {
          tft.setTextColor(TFT_RED);
          tft.setCursor(2, ys + 4);
          tft.setTextSize(2);
          tft.println(configured_die_names[i].c_str());
          tft.setCursor(2, tft.getCursorY());
          tft.print("Waiting...");
        } else {
          tft.setTextColor(TFT_WHITE);
          tft.setCursor(0, ys + 2);
          tft.setTextSize(1);
          tft.println(configured_die_names[i].c_str());
          tft.print("Cnt ");
          tft.print(die_roll_counts[i].GetNumRolls());
          if (die_battery[i].battery_level != INVALID_BATTERY) {
            tft.print(" Bat ");
            tft.print(die_battery[i].battery_level);
            tft.print("%");
            if (die_battery[i].is_charging) {
              tft.drawBitmap(tft.getCursorX(), tft.getCursorY(),
                             LIGHTNING_ICON_8X8, 8, 8, TFT_YELLOW);
            }
          }
          die_roll_counts[i].Draw();
        }
        die_updated[i] = false;
      }
    }
  }

  void HandleButton(ButtonType type, uint8_t b) override {
    if (type == ButtonType::LONG) {
      for (size_t i = 0; i < NUM_DICE; i++) {
        die_roll_counts[i].Clear();
        die_updated[i] = true;
      }
    }
  };

 private:
  static constexpr long BATTERY_REFRESH_RATE_MS = 60 * 1000;
  static constexpr int16_t SECTION_HEIGHT = TFT_HEIGHT / NUM_DICE;
  static constexpr uint8_t INVALID_BATTERY = 0xFF;
  std::array<long, NUM_DICE> last_update = {0};
  std::array<bool, NUM_DICE> die_connection_status = {false};
  std::array<bool, NUM_DICE> die_updated = {false};
  std::array<pixels::BatteryEvent, NUM_DICE> die_battery = {
      pixels::BatteryEvent{INVALID_BATTERY, false},
      pixels::BatteryEvent{INVALID_BATTERY, false}};
  std::array<RollCountWidget, NUM_DICE> die_roll_counts;
};

class EffectMenu : public MenuBase {
 public:
  EffectMenu() = default;

  void Update() override {}

  void Draw(bool force_redraw) override {
    if (force_redraw) {
      tft.fillScreen(TFT_BLACK);
      uint8_t mode = strip.getMainSegment().mode;
      if (Contains(DIE_LED_MODES, mode)) {
        char lineBuffer[CHAR_WIDTH_BIG + 1];
        extractModeName(mode, JSON_mode_names, lineBuffer, CHAR_WIDTH_BIG);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(0, 0);
        tft.setTextSize(2);
        PrintLnInBox(lineBuffer, (field_idx == 0) ? TFT_BLUE : TFT_BLACK);
        if (mode == FX_MODE_CHECK_D20) {
          snprintf(lineBuffer, sizeof(lineBuffer), "PASS: %u", target);
          PrintLnInBox(lineBuffer, (field_idx == 1) ? TFT_BLUE : TFT_BLACK);
        }
      } else {
        char lineBuffer[CHAR_WIDTH_SMALL + 1];
        extractModeName(mode, JSON_mode_names, lineBuffer, CHAR_WIDTH_SMALL);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(0, 0);
        tft.setTextSize(1);
        tft.println(lineBuffer);
      }
    }
  }

  void HandleButton(ButtonType type, uint8_t b) override {
    Segment& seg = strip.getMainSegment();
    auto mode_itr =
        std::find(DIE_LED_MODES.begin(), DIE_LED_MODES.end(), seg.mode);
    if (mode_itr != DIE_LED_MODES.end()) {
      mode_idx = mode_itr - DIE_LED_MODES.begin();
    }

    if (mode_itr == DIE_LED_MODES.end()) {
      seg.setMode(DIE_LED_MODES[mode_idx]);
    } else {
      if (type == ButtonType::LONG) {
        seg.loadModeDefaults();
        SetDefaultColors(DIE_LED_MODES[mode_idx]);
      } else if (b == 0) {
        field_idx = (field_idx + 1) % DIE_LED_MODE_NUM_FIELDS[mode_idx];
      } else {
        if (field_idx == 0) {
          mode_idx = (mode_idx + 1) % DIE_LED_MODES.size();
          seg.setMode(DIE_LED_MODES[mode_idx]);
        } else if (DIE_LED_MODES[mode_idx] == FX_MODE_CHECK_D20 &&
                   field_idx == 1) {
          target = last_die_value + 1;
        }
      }
    }
  };

 private:
  static constexpr std::array<uint8_t, 3> DIE_LED_MODES = {
      FX_MODE_SIMPLE_D20, FX_MODE_PULSE_D20, FX_MODE_CHECK_D20};
  static constexpr std::array<uint8_t, 3> DIE_LED_MODE_NUM_FIELDS = {1, 1, 2};
  static constexpr size_t CHAR_WIDTH_BIG = 10;
  static constexpr size_t CHAR_WIDTH_SMALL = 21;
  size_t mode_idx = 0;
  size_t field_idx = 0;

  void SetDefaults() {
    Segment& seg = strip.getMainSegment();
    switch (DIE_LED_MODES[mode_idx]) {
      case FX_MODE_SIMPLE_D20:
        seg.setColor(0, CYAN);
        seg.setColor(1, 0);
        break;
      case FX_MODE_PULSE_D20:
        seg.setPalette(50);
        seg.setColor(0, RED);
        break;
      case FX_MODE_CHECK_D20:
        seg.setPalette(0);
        seg.setColor(0, RED);
        seg.setColor(1, 0);
        break;
    }
  }
};

constexpr std::array<uint8_t, 3> EffectMenu::DIE_LED_MODES;
constexpr std::array<uint8_t, 3> EffectMenu::DIE_LED_MODE_NUM_FIELDS;

class InfoMenu : public MenuBase {
 public:
  InfoMenu() = default;

  void Update() override {}

  void Draw(bool force_redraw) override {
    if (force_redraw) {
      tft.fillScreen(TFT_BLACK);
      if (page_idx != INVALID_PAGE) {
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(0, 0);
        PrintRollInfo(page_idx);
      } else {
        tft.setTextColor(TFT_RED);
        tft.setCursor(0, 60);
        tft.setTextSize(2);
        tft.println("Set Roll");
      }
    }
  }

  void HandleButton(ButtonType type, uint8_t b) override {
    for (size_t i = 0; i < NUM_DICE; i++) {
      if (last_die_values[i] != INVALID_ROLL) {
        page_idx = last_die_values[i];
        break;
      }
    }
  };

 private:
  static  constexpr size_t INVALID_PAGE = 0xFFFFFFFF;
  static constexpr size_t CHAR_WIDTH_BIG = 10;
  static constexpr size_t CHAR_WIDTH_SMALL = 21;
  size_t page_idx = INVALID_PAGE;
};

class MenuController {
 public:
  void HandleButton(ButtonType type, uint8_t b) {
    force_redraw = true;
    // Switch menus with double click
    if (ButtonType::DOUBLE == type) {
      if (b == 0) {
        current_index =
            (current_index == 0) ? menu_ptrs.size() - 1 : current_index - 1;
      } else {
        current_index = (current_index + 1) % menu_ptrs.size();
      }
    } else {
      menu_ptrs[current_index]->HandleButton(type, b);
    }
  }

  void Update() {
    for (auto menu_ptr : menu_ptrs) {
      menu_ptr->Update();
    }
    menu_ptrs[current_index]->Draw(force_redraw);
    force_redraw = false;
  }

 private:
  size_t current_index = 0;
  bool force_redraw = true;

  DiceStatusMenu status_menu;
  EffectMenu effect_menu;
  InfoMenu info_menu;
  const std::array<MenuBase*, 3> menu_ptrs = {&status_menu, &effect_menu,
                                              &info_menu};
};
MenuController menu_ctrl;

class PixelsDiceTrayUsermod : public Usermod {
 private:
  bool enabled = true;

  // Settings
  unsigned font_size = USERMOD_PIXELS_DICE_TRAY_SCALE;
  unsigned rotation = USERMOD_PIXELS_DICE_TRAY_ROTATION;

  // Set the pin to turn the backlight on or off if available.
  static void EnableBacklight(bool enable) {
#if TFT_BL > 0
  #if USERMOD_PIXELS_DICE_TRAY_BL_ACTIVE_LOW
    enable = !enable;
  #endif
    digitalWrite(TFT_BL, enable);
#endif
  }

  static void center(String& line, uint8_t width) {
    int len = line.length();
    if (len < width)
      for (byte i = (width - len) / 2; i > 0; i--)
        line = ' ' + line;
    for (byte i = line.length(); i < width; i++)
      line += ' ';
  }

  // NOTE: THIS MOD DOES NOT SUPPORT CHANGING THE SPI PINS FROM THE UI! The
  // TFT_eSPI library requires that they are compiled in.
  static void SetSPIPinsFromMacros() {
    spi_mosi = TFT_MOSI;
    // Done in TFT library.
    if (TFT_MISO == TFT_MOSI) {
      spi_miso = -1;
    }
    spi_sclk = TFT_SCLK;
  }

 public:
  // Functions called by WLED

  /*
   * setup() is called once at boot. WiFi is not yet connected at this point.
   * You can use it to initialize variables, sensors or similar.
   */
  void setup() override {
    Serial.begin(115200);
    DEBUG_PRINTLN(F("Usermod TFT Display init"));
    SetSPIPinsFromMacros();
    PinManagerPinType spiPins[] = {
        {spi_mosi, true}, {spi_miso, false}, {spi_sclk, true}};
    if (!pinManager.allocateMultiplePins(spiPins, 3, PinOwner::HW_SPI)) {
      enabled = false;
    } else {
      PinManagerPinType displayPins[] = {
          {TFT_CS, true}, {TFT_DC, true}, {TFT_RST, true}, {TFT_BL, true}};
      if (!pinManager.allocateMultiplePins(
              displayPins, sizeof(displayPins) / sizeof(PinManagerPinType),
              PinOwner::UM_FourLineDisplay)) {
        pinManager.deallocateMultiplePins(spiPins, 3, PinOwner::HW_SPI);
        enabled = false;
      }
    }

    if (!enabled) {
      DEBUG_PRINTLN(F("Usermod TFT Display pin allocations failed."));
      return;
    }

    // Need to enable WiFi sleep:
    // "E (1513) wifi:Error! Should enable WiFi modem sleep when both WiFi and Bluetooth are enabled!!!!!!"
    noWifiSleep = false;

    strip.addEffect(FX_MODE_SIMPLE_D20, &simple_roll, _data_FX_MODE_SIMPLE_DIE);
    strip.addEffect(FX_MODE_PULSE_D20, &pulse_roll, _data_FX_MODE_PULSE_DIE);
    strip.addEffect(FX_MODE_CHECK_D20, &check_roll, _data_FX_MODE_CHECK_DIE);

    tft.init();
    tft.setRotation(rotation);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setCursor(0, 60);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2 * font_size);
    tft.print(" No Dice");

    // Start a background task scanning for dice.
    // On completion the discovered dice are connected to.
    pixels::ScanForDice(BLE_SCAN_DURATION_SEC, BLE_TIME_BETWEEN_SCANS_SEC);

    EnableBacklight(true);
  }

  /*
   * connected() is called every time the WiFi is (re)connected
   * Use it to initialize network interfaces
   */
  void connected() override {
    // Serial.println("Connected to WiFi!");
  }

  /*
   * loop() is called continuously. Here you can check for events, read sensors,
   * etc.
   *
   * Tips:
   * 1. You can use "if (WLED_CONNECTED)" to check for a successful network
   * connection. Additionally, "if (WLED_MQTT_CONNECTED)" is available to check
   * for a connection to an MQTT broker.
   *
   * 2. Try to avoid using the delay() function. NEVER use delays longer than 10
   * milliseconds. Instead, use a timer check as shown here.
   */
  void loop() override {
    static long last_loop_time = 0;
    static long last_die_connected_time = millis();

    // Check if we time interval for redrawing passes.
    if (millis() - last_loop_time < USERMOD_PIXELS_DICE_TRAY_REFRESH_RATE_MS) {
      return;
    }
    last_loop_time = millis();

    // Update dice_list with the connected dice
    pixels::ListDice(dice_list);
    // Get all the roll/battery updates since the last loop
    pixels::GetDieRollUpdates(roll_updates);
    pixels::GetDieBatteryUpdates(battery_updates);

    // Go through list of connected die.
    std::array<bool, NUM_DICE> die_connected = {false, false};
    for (auto die_id : dice_list) {
      // First check if we've already matched this ID to a connected die.
      bool matched = false;
      for (size_t i = 0; i < NUM_DICE; i++) {
        if (die_id == connected_die_ids[i]) {
          die_connected[i] = true;
          matched = true;
          break;
        }
      }

      // If this isn't already matched, check if its name matches an expected name.
      if (!matched) {
        auto description = pixels::GetDieDescription(die_id);
        for (size_t i = 0; i < NUM_DICE; i++) {
          if (0 == connected_die_ids[i] &&
              description.name == configured_die_names[i]) {
            connected_die_ids[i] = die_id;
            die_connected[i] = true;
            break;
          }
        }
      }
    }

    // Clear connected die that weren't still present.
    bool all_found = true;
    bool none_found = true;
    for (size_t i = 0; i < NUM_DICE; i++) {
      if (!die_connected[i]) {
        connected_die_ids[i] = 0;
        last_die_values[i] = INVALID_ROLL;
        all_found = false;
      } else {
        none_found = false;
      }
    }

    // Update last_die_values
    for (const auto& roll : roll_updates) {
      if (roll.second.state == pixels::RollState::ON_FACE) {
        last_die_value = roll.second.current_face;
        for (size_t i = 0; i < NUM_DICE; i++) {
          if (connected_die_ids[i] == roll.first) {
            last_die_values[i] = last_die_value;
          }
        }
      }
    }

#if USERMOD_PIXELS_DICE_TRAY_TIMEOUT_MS > 0
    if (none_found) {
      if (millis() - last_die_connected_time >
          USERMOD_PIXELS_DICE_TRAY_TIMEOUT_MS) {
        // Turn off backlight and go to sleep.
        // Since none of the wake up pins are wired up, expect to sleep
        // until power cycle or reset, so don't need to handle normal
        // wakeup.
        EnableBacklight(false);
        gpio_hold_en((gpio_num_t)TFT_BL);
        gpio_deep_sleep_hold_en();
        esp_deep_sleep_start();
      }
    } else {
      last_die_connected_time = millis();
    }
#endif

    if (pixels::IsScanning() && all_found) {
      pixels::StopScanning();
    } else if (!pixels::IsScanning() && !all_found) {
      pixels::ScanForDice(BLE_SCAN_DURATION_SEC, BLE_TIME_BETWEEN_SCANS_SEC);
    }

    // Add updates to the effect queue.
    dice_effect_state.insert(dice_effect_state.end(), roll_updates.begin(),
                             roll_updates.end());

    menu_ctrl.Update();
  }

  /*
   * addToJsonInfo() can be used to add custom entries to the /json/info part of
   * the JSON API. Creating an "u" object allows you to add custom key/value
   * pairs to the Info section of the WLED web UI. Below it is shown how this
   * could be used for e.g. a light sensor
   */
  void addToJsonInfo(JsonObject& root) override {
    JsonObject user = root["u"];
    if (user.isNull())
      user = root.createNestedObject("u");

    JsonArray lightArr = user.createNestedArray("TFT");      // name
    lightArr.add(enabled ? F("installed") : F("disabled"));  // unit
  }

  /*
   * addToJsonState() can be used to add custom entries to the /json/state part
   * of the JSON API (state object). Values in the state object may be modified
   * by connected clients
   */
  void addToJsonState(JsonObject& root) override {
    // root["user0"] = userVar0;
  }

  /*
   * readFromJsonState() can be used to receive data clients send to the
   * /json/state part of the JSON API (state object). Values in the state object
   * may be modified by connected clients
   */
  void readFromJsonState(JsonObject& root) override {
    // userVar0 = root["user0"] | userVar0; //if "user0" key exists in JSON,
    // update, else keep old value if (root["bri"] == 255)
    // Serial.println(F("Don't burn down your garage!"));
  }

  /*
   * addToConfig() can be used to add custom persistent settings to the cfg.json
   * file in the "um" (usermod) object. It will be called by WLED when settings
   * are actually saved (for example, LED settings are saved) If you want to
   * force saving the current state, use serializeConfig() in your loop().
   *
   * CAUTION: serializeConfig() will initiate a filesystem write operation.
   * It might cause the LEDs to stutter and will cause flash wear if called too
   * often. Use it sparingly and always in the loop, never in network callbacks!
   *
   * addToConfig() will also not yet add your setting to one of the settings
   * pages automatically. To make that work you still have to add the setting to
   * the HTML, xml.cpp and set.cpp manually.
   *
   * I highly recommend checking out the basics of ArduinoJson serialization and
   * deserialization in order to use custom settings!
   */
  void addToConfig(JsonObject& root) override {
    JsonObject top = root.createNestedObject("TFT");
    top["rotation"] = rotation;
    top["font_size"] = font_size;
    JsonArray pins = top.createNestedArray("pin");
    pins.add(TFT_CS);
    pins.add(TFT_DC);
    pins.add(TFT_RST);
    pins.add(TFT_BL);
  }

  void appendConfigData() override {
    oappend(SET_F("dd=addDropdown('TFT','rotation');"));
    oappend(SET_F("addOption(dd,'0 deg',0);"));
    oappend(SET_F("addOption(dd,'90 deg',1);"));
    oappend(SET_F("addOption(dd,'180 deg',2);"));
    oappend(SET_F("addOption(dd,'270 deg',3);"));
    oappend(
        SET_F("addInfo('TFT:font_size',1,'<br><i class=\"warn\">DO NOT CHANGE "
              "SPI PINS ABOVE OR BELOW.</i><br><i class=\"warn\">CHANGES ARE "
              "IGNORED EXCEPT FOR CHECKING PIN CONFLICTS.</i>','');"));
    oappend(SET_F("addInfo('TFT:pin[]',0,'','SPI CS');"));
    oappend(SET_F("addInfo('TFT:pin[]',1,'','SPI DC');"));
    oappend(SET_F("addInfo('TFT:pin[]',2,'','SPI RST');"));
    oappend(SET_F("addInfo('TFT:pin[]',3,'','SPI BL');"));
  }

  /*
   * readFromConfig() can be used to read back the custom settings you added
   * with addToConfig(). This is called by WLED when settings are loaded
   * (currently this only happens once immediately after boot)
   *
   * readFromConfig() is called BEFORE setup(). This means you can use your
   * persistent values in setup() (e.g. pin assignments, buffer sizes), but also
   * that if you want to write persistent values to a dynamic buffer, you'd need
   * to allocate it here instead of in setup. If you don't know what that is,
   * don't fret. It most likely doesn't affect your use case :)
   */
  bool readFromConfig(JsonObject& root) override {
    // we look for JSON object:
    // {"TFT":{"rotation":0,"font_size":1}}
    JsonObject top = root["TFT"];
    if (top.isNull()) {
      DEBUG_PRINTLN(F("TFT: No config found. (Using defaults.)"));
      return false;
    }
    unsigned new_rotation = min(top["rotation"] | rotation, 3u);
    unsigned new_font_size = max(top["font_size"] | font_size, 1u);

    // Restore the SPI pins to their compiled in defaults.
    SetSPIPinsFromMacros();

    if (new_rotation != rotation || font_size != new_font_size) {
      rotation = new_rotation;
      font_size = new_font_size;
    }

    // use "return !top["newestParameter"].isNull();" when updating Usermod with
    // new features
    return !top["TFT"].isNull();
  }

  /**
   * handleButton() can be used to override default button behaviour. Returning true
   * will prevent button working in a default way.
   * Replicating button.cpp
   */
  bool handleButton(uint8_t b) override {
    if (!enabled || b > 1  // buttons 0,1 only
        || buttonType[b] == BTN_TYPE_SWITCH || buttonType[b] == BTN_TYPE_NONE ||
        buttonType[b] == BTN_TYPE_RESERVED ||
        buttonType[b] == BTN_TYPE_PIR_SENSOR ||
        buttonType[b] == BTN_TYPE_ANALOG ||
        buttonType[b] == BTN_TYPE_ANALOG_INVERTED) {
      return false;
    }

    unsigned long now = millis();
    static bool buttonPressedBefore[2] = {false};
    static bool buttonLongPressed[2] = {false};
    static unsigned long buttonPressedTime[2] = {0};
    static unsigned long buttonWaitTime[2] = {0};

    //momentary button logic
    if (!buttonLongPressed[b] && isButtonPressed(b)) {  //pressed
      if (!buttonPressedBefore[b]) {
        buttonPressedTime[b] = now;
      }
      buttonPressedBefore[b] = true;

      if (now - buttonPressedTime[b] > WLED_LONG_PRESS) {  //long press
        menu_ctrl.HandleButton(ButtonType::LONG, b);
        buttonLongPressed[b] = true;
        return true;
      }
    } else if (!isButtonPressed(b) && buttonPressedBefore[b]) {  //released

      long dur = now - buttonPressedTime[b];
      if (dur < WLED_DEBOUNCE_THRESHOLD) {
        buttonPressedBefore[b] = false;
        return true;
      }  //too short "press", debounce

      bool doublePress = buttonWaitTime[b];  //did we have short press before?
      buttonWaitTime[b] = 0;

      if (!buttonLongPressed[b]) {  //short press
        // if this is second release within 350ms it is a double press (buttonWaitTime!=0)
        if (doublePress) {
          menu_ctrl.HandleButton(ButtonType::DOUBLE, b);
        } else {
          buttonWaitTime[b] = now;
        }
      }
      buttonPressedBefore[b] = false;
      buttonLongPressed[b] = false;
    }
    // if 350ms elapsed since last press/release it is a short press
    if (buttonWaitTime[b] && now - buttonWaitTime[b] > WLED_DOUBLE_PRESS &&
        !buttonPressedBefore[b]) {
      buttonWaitTime[b] = 0;
      menu_ctrl.HandleButton(ButtonType::SINGLE, b);
    }

    return true;
  }

  /*
   * getId() allows you to optionally give your V2 usermod an unique ID (please
   * define it in const.h!). This could be used in the future for the system to
   * determine whether your usermod is installed.
   */
  uint16_t getId() { return USERMOD_ID_PIXELS_DICE_TRAY; }

  // More methods can be added in the future, this example will then be
  // extended. Your usermod will remain compatible as it does not need to
  // implement all methods from the Usermod base class!
};
