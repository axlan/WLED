#pragma once

#include <pixels_dice_interface.h>  // https://github.com/axlan/arduino-pixels-dice
#include "wled.h"

#include "dice_state.h"
#include "tft_menu.h"

// Set this parameter to rotate the display. 1-3 rotate by 90,180,270 degrees.
#ifndef USERMOD_PIXELS_DICE_TRAY_ROTATION
  #define USERMOD_PIXELS_DICE_TRAY_ROTATION 0
#endif

// How often we are redrawing screen
#ifndef USERMOD_PIXELS_DICE_TRAY_REFRESH_RATE_MS
  #define USERMOD_PIXELS_DICE_TRAY_REFRESH_RATE_MS 200
#endif

// Time with no updates before screen turns off (-1 to disable)
#ifndef USERMOD_PIXELS_DICE_TRAY_TIMEOUT_MS
  #define USERMOD_PIXELS_DICE_TRAY_TIMEOUT_MS 2 * 60 * 1000
#endif

// Duration of each search for BLE devices.
#ifndef BLE_SCAN_DURATION_SEC
  #define BLE_SCAN_DURATION_SEC 4
#endif

// Time between searches for BLE devices.
#ifndef BLE_TIME_BETWEEN_SCANS_SEC
  #define BLE_TIME_BETWEEN_SCANS_SEC 5
#endif

#define WLED_DEBOUNCE_THRESHOLD \
  50  // only consider button input of at least 50ms as valid (debouncing)
#define WLED_LONG_PRESS \
  600  // long press if button is released after held for at least 600ms
#define WLED_DOUBLE_PRESS \
  350  // double press if another press within 350ms after a short press

extern int getSignalQuality(int rssi);

// Reuse FX display functions.
extern uint16_t mode_breath();
extern uint16_t mode_blends();
extern uint16_t running(uint32_t color1, uint32_t color2, bool theatre = false);
extern uint16_t mode_glitter();
extern uint16_t mode_gravcenter();

DiceState dice_state;

// These are updated in the main loop, but accessed by the effect functions as
// well. My understand is that both of these accesses should be running on the
// same "thread/task" since WLED doesn't directly create additional threads. The
// exception would be network callbacks and interrupts, but I don't beleive
// these accesses are triggered by those. If synchronization was needed, I could
// look at the example in `requestJSONBufferLock()`.
static pixels::RollUpdates dice_effect_state;

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
      if (roll.current_face + 1 >= dice_state.roll_target) {
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

class PixelsDiceTrayUsermod : public Usermod {
 private:
  bool enabled = true;

#if USING_TFT_DISPLAY
  MenuController menu_ctrl;
#endif

  // Settings
  unsigned rotation = USERMOD_PIXELS_DICE_TRAY_ROTATION;

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
#if USING_TFT_DISPLAY
    spi_mosi = TFT_MOSI;
    // Done in TFT library.
    if (TFT_MISO == TFT_MOSI) {
      spi_miso = -1;
    }
    spi_sclk = TFT_SCLK;
#endif
  }

 public:
  // Functions called by WLED

  /*
   * setup() is called once at boot. WiFi is not yet connected at this point.
   * You can use it to initialize variables, sensors or similar.
   */
  void setup() override {
    Serial.begin(115200);

#if USING_TFT_DISPLAY
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
#endif

    // Need to enable WiFi sleep:
    // "E (1513) wifi:Error! Should enable WiFi modem sleep when both WiFi and Bluetooth are enabled!!!!!!"
    noWifiSleep = false;

    strip.addEffect(FX_MODE_SIMPLE_D20, &simple_roll, _data_FX_MODE_SIMPLE_DIE);
    strip.addEffect(FX_MODE_PULSE_D20, &pulse_roll, _data_FX_MODE_PULSE_DIE);
    strip.addEffect(FX_MODE_CHECK_D20, &check_roll, _data_FX_MODE_CHECK_DIE);

    // Start a background task scanning for dice.
    // On completion the discovered dice are connected to.
    pixels::ScanForDice(BLE_SCAN_DURATION_SEC, BLE_TIME_BETWEEN_SCANS_SEC);

#if USING_TFT_DISPLAY
    menu_ctrl.Init(rotation);
#endif
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

    char mqtt_topic_buffer[MQTT_MAX_TOPIC_LEN + 16];
    char mqtt_data_buffer[128];

    // Check if we time interval for redrawing passes.
    if (millis() - last_loop_time < USERMOD_PIXELS_DICE_TRAY_REFRESH_RATE_MS) {
      return;
    }
    last_loop_time = millis();

    // Update dice_state.dice_list with the connected dice
    pixels::ListDice(dice_state.dice_list);
    // Get all the roll/battery updates since the last loop
    pixels::GetDieRollUpdates(dice_state.roll_updates);
    pixels::GetDieBatteryUpdates(dice_state.battery_updates);

    // Go through list of connected die.
    std::array<bool, DiceState::NUM_DICE> die_connected = {false, false};
    for (auto die_id : dice_state.dice_list) {
      // First check if we've already matched this ID to a connected die.
      bool matched = false;
      for (size_t i = 0; i < DiceState::NUM_DICE; i++) {
        if (die_id == dice_state.connected_die_ids[i]) {
          die_connected[i] = true;
          matched = true;
          break;
        }
      }

      // If this isn't already matched, check if its name matches an expected name.
      if (!matched) {
        auto description = pixels::GetDieDescription(die_id);
        for (size_t i = 0; i < DiceState::NUM_DICE; i++) {
          if (0 == dice_state.connected_die_ids[i] &&
              description.name == dice_state.configured_die_names[i]) {
            dice_state.connected_die_ids[i] = die_id;
            die_connected[i] = true;
            break;
          }
        }
      }
    }

    // Clear connected die that weren't still present.
    bool all_found = true;
    bool none_found = true;
    for (size_t i = 0; i < DiceState::NUM_DICE; i++) {
      if (!die_connected[i]) {
        dice_state.connected_die_ids[i] = 0;
        dice_state.last_die_values[i] = DiceState::INVALID_ROLL;
        all_found = false;
      } else {
        none_found = false;
      }
    }

    // Update dice_state.last_die_values
    for (const auto& roll : dice_state.roll_updates) {
      if (roll.second.state == pixels::RollState::ON_FACE) {
        dice_state.last_die_value = roll.second.current_face;
        for (size_t i = 0; i < DiceState::NUM_DICE; i++) {
          if (dice_state.connected_die_ids[i] == roll.first) {
            dice_state.last_die_values[i] = dice_state.last_die_value;
          }
        }
      }
      if (WLED_MQTT_CONNECTED) {
        snprintf(mqtt_topic_buffer, sizeof(mqtt_topic_buffer), PSTR("%s/%s"),
                 mqttDeviceTopic, "dice/roll");
        const char* name = pixels::GetDieDescription(roll.first).name.c_str();
        snprintf(mqtt_data_buffer, sizeof(mqtt_data_buffer),
                 "{\"name\":\"%s\",\"state\":%d,\"val\":%d,\"time\":%d}", name,
                 int(roll.second.state), roll.second.current_face + 1,
                 roll.second.timestamp);
        mqtt->publish(mqtt_topic_buffer, 0, false, mqtt_data_buffer);
      }
    }

#if USERMOD_PIXELS_DICE_TRAY_TIMEOUT_MS > 0 && USING_TFT_DISPLAY
    if (none_found) {
      if (millis() - last_die_connected_time >
          USERMOD_PIXELS_DICE_TRAY_TIMEOUT_MS) {
        // Turn off backlight and go to sleep.
        // Since none of the wake up pins are wired up, expect to sleep
        // until power cycle or reset, so don't need to handle normal
        // wakeup.
        menu_ctrl.EnableBacklight(false);
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
    dice_effect_state.insert(dice_effect_state.end(),
                             dice_state.roll_updates.begin(),
                             dice_state.roll_updates.end());
#if USING_TFT_DISPLAY
    menu_ctrl.Update();
#endif
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

    JsonArray lightArr = user.createNestedArray("DiceTray");  // name
    lightArr.add(enabled ? F("installed") : F("disabled"));   // unit
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
    JsonObject top = root.createNestedObject("DiceTray");
#if USING_TFT_DISPLAY
    top["rotation"] = rotation;
    JsonArray pins = top.createNestedArray("pin");
    pins.add(TFT_CS);
    pins.add(TFT_DC);
    pins.add(TFT_RST);
    pins.add(TFT_BL);
#endif
  }

#if USING_TFT_DISPLAY
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
#endif
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
    // {"DiceTray":{"rotation":0,"font_size":1}}
    JsonObject top = root["DiceTray"];
    if (top.isNull()) {
      DEBUG_PRINTLN(F("DiceTray: No config found. (Using defaults.)"));
      return false;
    }
#if USING_TFT_DISPLAY
    unsigned new_rotation = min(top["rotation"] | rotation, 3u);

    // Restore the SPI pins to their compiled in defaults.
    SetSPIPinsFromMacros();

    if (new_rotation != rotation) {
      rotation = new_rotation;
    }
#endif

    // use "return !top["newestParameter"].isNull();" when updating Usermod with
    // new features
    return !top["DiceTray"].isNull();
  }

  /**
   * handleButton() can be used to override default button behaviour. Returning true
   * will prevent button working in a default way.
   * Replicating button.cpp
   */
#if USING_TFT_DISPLAY
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
#endif

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
