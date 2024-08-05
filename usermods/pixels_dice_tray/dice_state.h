#pragma once

#include <pixels_dice_interface.h>  // https://github.com/axlan/arduino-pixels-dice

struct DiceState {
  static constexpr uint8_t INVALID_ROLL = 0xFF;
  static constexpr size_t NUM_DICE = 2;
  // The vectors to hold results queried from the library
  // Since vectors allocate data, it's more efficient to keep reusing objects
  // instead of declaring them on the stack
  std::vector<pixels::PixelsDieID> dice_list;
  pixels::RollUpdates roll_updates;
  pixels::BatteryUpdates battery_updates;

  // NOTE: The ordering is taken into account when referencing die 1 vs die 2.
  std::array<std::string, NUM_DICE> configured_die_names = {{
      "Aurora",
      "Midnight",
  }};
  // The ordering here matches configured_die_names.
  std::array<pixels::PixelsDieID, NUM_DICE> connected_die_ids = {{0, 0}};
  std::array<uint8_t, NUM_DICE> last_die_values = {
      {INVALID_ROLL, INVALID_ROLL}};
  uint8_t last_die_value = INVALID_ROLL;

  uint8_t roll_target = 10;
  uint8_t roll_label = INVALID_ROLL;
};

template <typename C, typename T>
static bool Contains(const C& container, T value) {
  return std::find(container.begin(), container.end(), value) !=
         container.end();
}
