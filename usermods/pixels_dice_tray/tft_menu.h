#pragma once

#ifndef TFT_WIDTH
  #warning TFT parameters not specified, not using screen.
#else
  #include <TFT_eSPI.h>
  #include <pixels_dice_interface.h>  // https://github.com/axlan/arduino-pixels-dice
  #include "wled.h"

  #include "dice_state.h"
  #include "roll_info.h"

  #define USING_TFT_DISPLAY 1

  #ifndef TFT_BL
    #define TFT_BL -1
  #endif

const uint8_t LIGHTNING_ICON_8X8[] PROGMEM = {
    0b00001111, 0b00010010, 0b00100100, 0b01001111,
    0b10000001, 0b11110010, 0b00010100, 0b00011000,
};

extern DiceState dice_state;

TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);  // Invoke custom library

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
    for (size_t i = 0; i < DiceState::NUM_DICE; i++) {
      const auto die_id = dice_state.connected_die_ids[i];
      const auto connected = die_id != 0;

      die_updated[i] |= connected != die_connection_status[i];
      die_connection_status[i] = connected;

      if (connected) {
        bool charging = false;
        for (const auto& battery : dice_state.battery_updates) {
          if (battery.first == die_id) {
            if (die_battery[i].battery_level == INVALID_BATTERY ||
                battery.second.is_charging != die_battery[i].is_charging) {
              die_updated[i] = true;
            }
            die_battery[i] = battery.second;
          }
        }

        for (const auto& roll : dice_state.roll_updates) {
          if (roll.first == die_id &&
              roll.second.state == pixels::RollState::ON_FACE) {
            die_roll_counts[i].AddRoll(roll.second.current_face);
            die_updated[i] = true;
          }
        }

        for (const auto& battery : dice_state.battery_updates) {
          if (battery.first == die_id) {
            die_battery[i] = battery.second;
          }
        }
      }
    }
  }

  void Draw(bool force_redraw) override {
    // This could probably be optimized for partial redraws.
    for (size_t i = 0; i < DiceState::NUM_DICE; i++) {
      const int16_t ys = SECTION_HEIGHT * i;
      const auto die_id = dice_state.connected_die_ids[i];
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
          tft.println(dice_state.configured_die_names[i].c_str());
          tft.setCursor(2, tft.getCursorY());
          tft.print("Waiting...");
        } else {
          tft.setTextColor(TFT_WHITE);
          tft.setCursor(0, ys + 2);
          tft.setTextSize(1);
          tft.println(dice_state.configured_die_names[i].c_str());
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
      for (size_t i = 0; i < DiceState::NUM_DICE; i++) {
        die_roll_counts[i].Clear();
        die_updated[i] = true;
      }
    }
  };

 private:
  static constexpr long BATTERY_REFRESH_RATE_MS = 60 * 1000;
  static constexpr int16_t SECTION_HEIGHT = TFT_HEIGHT / DiceState::NUM_DICE;
  static constexpr uint8_t INVALID_BATTERY = 0xFF;
  std::array<long, DiceState::NUM_DICE> last_update = {0};
  std::array<bool, DiceState::NUM_DICE> die_connection_status = {false};
  std::array<bool, DiceState::NUM_DICE> die_updated = {false};
  std::array<pixels::BatteryEvent, DiceState::NUM_DICE> die_battery = {
      pixels::BatteryEvent{INVALID_BATTERY, false},
      pixels::BatteryEvent{INVALID_BATTERY, false}};
  std::array<RollCountWidget, DiceState::NUM_DICE> die_roll_counts;
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
          snprintf(lineBuffer, sizeof(lineBuffer), "PASS: %u",
                   dice_state.roll_target);
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
          dice_state.roll_target = dice_state.last_die_value + 1;
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
      if (dice_state.roll_label != DiceState::INVALID_ROLL) {
        PrintRollInfo(dice_state.roll_label);
      } else {
        tft.setTextColor(TFT_RED);
        tft.setCursor(0, 60);
        tft.setTextSize(2);
        tft.println("Set Roll");
      }
    }
  }

  void HandleButton(ButtonType type, uint8_t b) override {
    if (dice_state.roll_label >= NUM_ROLL_INFOS) {
      dice_state.roll_label = 0;
    } else if (b == 0) {
      dice_state.roll_label = (dice_state.roll_label == 0)
                                  ? NUM_ROLL_INFOS - 1
                                  : dice_state.roll_label - 1;
    } else if (b == 1) {
      dice_state.roll_label = (dice_state.roll_label + 1) % NUM_ROLL_INFOS;
    }
    if (WLED_MQTT_CONNECTED) {
      char mqtt_topic_buffer[MQTT_MAX_TOPIC_LEN + 16];
      snprintf(mqtt_topic_buffer, sizeof(mqtt_topic_buffer), PSTR("%s/%s"),
               mqttDeviceTopic, "dice/dice_state.roll_label");
      mqtt->publish(mqtt_topic_buffer, 0, false,
                    GetRollName(dice_state.roll_label));
    }
  };
};

class MenuController {
 public:
  void Init(unsigned rotation) {
    tft.init();
    tft.setRotation(rotation);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setCursor(0, 60);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.print(" No Dice");

    EnableBacklight(true);
  }

  // Set the pin to turn the backlight on or off if available.
  static void EnableBacklight(bool enable) {
  #if TFT_BL > 0
    #if USERMOD_PIXELS_DICE_TRAY_BL_ACTIVE_LOW
    enable = !enable;
    #endif
    digitalWrite(TFT_BL, enable);
  #endif
  }

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
#endif
