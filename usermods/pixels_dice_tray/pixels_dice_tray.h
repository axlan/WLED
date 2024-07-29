// Credits to @mrVanboy, @gwaland and my dearest friend @westward
// Also for @spiff72 for usermod TTGO-T-Display
#pragma once

#include "wled.h"
#include <TFT_eSPI.h>
#include <pixels_dice_interface.h> // https://github.com/axlan/arduino-pixels-dice

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
  #define USERMOD_PIXELS_DICE_TRAY_TIMEOUT_MS 5*60*1000
#endif

extern int getSignalQuality(int rssi);

static constexpr size_t BLE_SCAN_DURATION_SEC = 4;
static constexpr size_t BLE_TIME_BETWEEN_SCANS_SEC = 5;

// The vectors to hold results queried from the library
// Since vectors allocate data, it's more efficient to keep reusing objects
// instead of declaring them on the stack
std::vector<pixels::PixelsDieID> dice_list;
pixels::RollUpdates roll_updates;
pixels::BatteryUpdates battery_updates;

TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);  // Invoke custom library

static uint16_t my_blink() {
  uint32_t color1 = SEGCOLOR(0);
  uint32_t color2 = SEGCOLOR(1);
  uint32_t cycleTime = (255 - SEGMENT.speed)*20;
  uint32_t onTime = FRAMETIME;
  onTime += ((cycleTime * SEGMENT.intensity) >> 8);
  cycleTime += FRAMETIME*2;
  uint32_t it = strip.now / cycleTime;
  uint32_t rem = strip.now % cycleTime;

  bool on = false;
  if (it != SEGENV.step //new iteration, force on state for one frame, even if set time is too brief
      || rem <= onTime) {
    on = true;
  }

  SEGENV.step = it; //save previous iteration

  uint32_t color = on ? color1 : color2;
  SEGMENT.fill(color);

  return FRAMETIME;
}
//<Effect parameters>;<Colors>;<Palette>;<Flags>;<Defaults>
// https://kno.wled.ge/interfaces/json-api/#effect-metadata
// speed/intesity, 2 colors, no palette, flags
static const char _data_FX_MODE_MYBLINK[] PROGMEM = "MyBLINK@!,!;!,!;;01";



// Need to check if this needs some synchronization.
static pixels::RollUpdates dice_effect_state;

extern uint16_t mode_breath();
extern uint16_t mode_aurora();

static uint16_t basic_roll() {
  if (!dice_effect_state.empty()) {
    // Only keep last state.
    if (dice_effect_state.size() > 1) {
      dice_effect_state.erase(dice_effect_state.begin(), dice_effect_state.begin() + dice_effect_state.size() - 1);
    }

    auto roll = dice_effect_state.end()->second;

    if (roll.state != pixels::RollState::ON_FACE) {
      return mode_breath();
    }
    else {
      uint16_t ret = mode_aurora();
      uint16_t num_segments = float(roll.current_face + 1) / 20.0 * SEGLEN;
      for (int i = num_segments; i < SEGLEN; i++) {
        SEGMENT.setPixelColor(i, SEGCOLOR(1));
      }
      return ret;
    }
  }
  return FRAMETIME;

  // uint32_t color1 = SEGCOLOR(0);
  // uint32_t color2 = SEGCOLOR(1);
  // uint32_t cycleTime = (255 - SEGMENT.speed)*20;
  // uint32_t onTime = FRAMETIME;
  // onTime += ((cycleTime * SEGMENT.intensity) >> 8);
  // cycleTime += FRAMETIME*2;
  // uint32_t it = strip.now / cycleTime;
  // uint32_t rem = strip.now % cycleTime;

  // bool on = false;
  // if (it != SEGENV.step //new iteration, force on state for one frame, even if set time is too brief
  //     || rem <= onTime) {
  //   on = true;
  // }

  // SEGENV.step = it; //save previous iteration

  // uint32_t color = on ? color1 : color2;
  // SEGMENT.fill(color);

  // return FRAMETIME;
}
static const char _data_FX_MODE_DIEROLL[] PROGMEM = "DieRoll@!;!,!;!;01";



class RollCountWidget
{
  private:
    // Could make configurable if needed.
    const int16_t xs = 0;
    const int16_t ys = 0;
    const uint16_t border_color = TFT_RED;
    const uint16_t bar_color = TFT_GREEN;
    const uint16_t bar_width = 6;
    const uint16_t max_bar_height = 60;
    unsigned roll_counts[20] = {0};
    unsigned total = 0;
    unsigned max_count = 0;
    
  public:

    void Clear() {
      memset(roll_counts,0,sizeof(roll_counts));
      total = 0;
    }

    void AddRoll(unsigned val){
      if (val > 19) {
        return;
      }
      roll_counts[val]++;
      total++;
      max_count = max(roll_counts[val], max_count);
    }

    void Draw() {
      // Add 2 pixels to lengths for boarder width.
      tft.drawRect(xs, ys, bar_width * 20 + 2, max_bar_height + 2,
                   border_color);
      for (size_t i = 0; i < 20; i++) {
        if (roll_counts[i] > 0) {
          // Scale bar by highest count.
          uint16_t bar_height = round(float(roll_counts[i]) / float(max_count) *
                                      float(max_bar_height));
          // Add space between bars
          uint16_t padding = (bar_width > 1) ? 1 : 0;
          // Need to start from top of bar and draw down
          tft.fillRect(xs + 1 + bar_width * i,
                       ys + 1 + max_bar_height - bar_height,
                       bar_width - padding, bar_height, bar_color);
        }
      }
    }
};


class PixelsDiceTrayUsermod : public Usermod {
 private:
  unsigned long lastTime = 0;
  bool enabled = true;


  // Settings
  unsigned font_size = USERMOD_PIXELS_DICE_TRAY_SCALE;
  unsigned rotation = USERMOD_PIXELS_DICE_TRAY_ROTATION;

  // Number of chars that fit on screen with text size set to `font_size`
  static constexpr size_t TFT_CHAR_WIDTH = 19;
  // Extra char (+1) for null
  static constexpr size_t LINE_BUFFER_SIZE = TFT_CHAR_WIDTH + 1;  

  long lastUpdate = 0;

  RollCountWidget roll_widget;

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
      for (byte i = (width - len) / 2; i > 0; i--) line = ' ' + line;
    for (byte i = line.length(); i < width; i++) line += ' ';
  }

  // Make sure the next update redraws the screen.
  void ForceRedraw() {
    lastUpdate = 0;
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

    strip.addEffect(255, &my_blink, _data_FX_MODE_MYBLINK);
    strip.addEffect(255, &basic_roll, _data_FX_MODE_DIEROLL);

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
    char buff[LINE_BUFFER_SIZE];

    // Check if we time interval for redrawing passes.
    if (millis() - lastUpdate < USERMOD_PIXELS_DICE_TRAY_REFRESH_RATE_MS) {
      return;
    }
    lastUpdate = millis();

    // Update dice_list with the connected dice
    pixels::ListDice(dice_list);
    // Get all the roll/battery updates since the last loop
    pixels::GetDieRollUpdates(roll_updates);
    pixels::GetDieBatteryUpdates(battery_updates);

    bool rolled = false;
    for (const auto& roll: roll_updates) {
      if (roll.second.state == pixels::RollState::ON_FACE) {
        roll_widget.AddRoll(roll.second.current_face);
        rolled = true;
      }
    }
    // Add updates to the effect queue.
    dice_effect_state.insert(dice_effect_state.end(), roll_updates.begin(), roll_updates.end());

    if (rolled) {
      tft.fillScreen(TFT_BLACK);
      roll_widget.Draw();
    }
  }

  /*
   * addToJsonInfo() can be used to add custom entries to the /json/info part of
   * the JSON API. Creating an "u" object allows you to add custom key/value
   * pairs to the Info section of the WLED web UI. Below it is shown how this
   * could be used for e.g. a light sensor
   */
  void addToJsonInfo(JsonObject& root) override {
    JsonObject user = root["u"];
    if (user.isNull()) user = root.createNestedObject("u");

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
      ForceRedraw();
    }

    // use "return !top["newestParameter"].isNull();" when updating Usermod with
    // new features
    return !top["TFT"].isNull();
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
