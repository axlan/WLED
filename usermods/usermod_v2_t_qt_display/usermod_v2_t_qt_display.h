#pragma once

// Adapted from TTGO-T-Display usermod.

#include "wled.h"

#include <TFT_eSPI.h>

// Font Size 1 (6x8) allows 21x16 characters with one pixel spacing
// Font Size 2 (12x16) allows 10x8 characters with two pixel spacing

static TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

class TQTDisplay : public Usermod
{
private:
  // needRedraw marks if redraw is required to prevent often redrawing.
  bool needRedraw = true;

  // Next variables hold the previous known values to determine if redraw is
  // required.
  String knownSsid = "";
  IPAddress knownIp;
  uint8_t knownBrightness = 0;
  uint8_t knownMode = 0;
  uint8_t knownPalette = 0;

  long lastUpdate = 0;
  long lastRedraw = 0;

  static constexpr uint8_t CHARS_PER_LINE = 21; // Number of chars that fit on screen with text size set to 1

  // How often we are redrawing screen
  static constexpr unsigned USER_LOOP_REFRESH_RATE_MS = 5000;

public:
  /*
   * setup() is called once at boot. WiFi is not yet connected at this point.
   * You can use it to initialize variables, sensors or similar.
   */
  void setup() override
  {
    DEBUG_PRINTLN(F("Usermod Rotary Encoder init"));
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(1, 10);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(3);
    tft.print("Loading...");
  }

  /*
   * loop() is called continuously. Here you can check for events, read sensors, etc.
   *
   * Tips:
   * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
   *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
   *
   * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
   *    Instead, use a timer check as shown here.
   */
  void loop() override
  {
    // Check if we time interval for redrawing passes.
    if (millis() - lastUpdate < USER_LOOP_REFRESH_RATE_MS)
    {
      return;
    }
    lastUpdate = millis();

    // Check if values which are shown on display changed from the last time.
    if (((apActive) ? String(apSSID) : WiFi.SSID()) != knownSsid)
    {
      needRedraw = true;
    }
    else if (knownIp != (apActive ? IPAddress(4, 3, 2, 1) : WiFi.localIP()))
    {
      needRedraw = true;
    }
    else if (knownBrightness != bri)
    {
      needRedraw = true;
    }
    else if (knownMode != strip.getMainSegment().mode)
    {
      needRedraw = true;
    }
    else if (knownPalette != strip.getMainSegment().palette)
    {
      needRedraw = true;
    }

    if (!needRedraw)
    {
      return;
    }
    needRedraw = false;

    lastRedraw = millis();

// Update last known values.
#if defined(ESP8266)
    knownSsid = apActive ? WiFi.softAPSSID() : WiFi.SSID();
#else
    knownSsid = WiFi.SSID();
#endif
    knownIp = apActive ? IPAddress(4, 3, 2, 1) : WiFi.localIP();
    knownBrightness = bri;
    knownMode = strip.getMainSegment().mode;
    knownPalette = strip.getMainSegment().palette;

    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    // First row with Wifi name
    tft.setCursor(1, 1);
    tft.print(knownSsid.substring(0,  CHARS_PER_LINE - 1));
    // Print `~` char to indicate that SSID is longer than our display
    if (knownSsid.length() > CHARS_PER_LINE)
      tft.print("~");

    // Second row with AP IP and Password or IP
    tft.setTextSize(1);
    tft.setCursor(1, 24);
    // Print AP IP and password in AP mode or knownIP if AP not active.
    // if (apActive && bri == 0)
    //   tft.print(apPass);
    // else
    //   tft.print(knownIp);

    if (apActive)
    {
      tft.print("AP IP: ");
      tft.print(knownIp);
      tft.setCursor(1, 46);
      tft.print("AP Pass:");
      tft.print(apPass);
    }
    else
    {
      tft.print("IP: ");
      tft.print(knownIp);
      tft.setCursor(1, 46);
      // tft.print("Signal Strength: ");
      // tft.print(i.wifi.signal);
      tft.print("Brightness: ");
      tft.print(((float(bri) / 255) * 100));
      tft.print("%");
    }

    // Third row with mode name
    tft.setCursor(1, 68);
    char lineBuffer[CHARS_PER_LINE + 1];
    extractModeName(knownMode, JSON_mode_names, lineBuffer, CHARS_PER_LINE);
    tft.print(lineBuffer);

    // Fourth row with estimated mA usage
    tft.setCursor(1, 90);
    // Print estimated milliamp usage (must specify the LED type in LED prefs for this to be a reasonable estimate).
    tft.print(strip.currentMilliamps);
    tft.print("mA (estimated)");
  }

  /*
   * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
   * This could be used in the future for the system to determine whether your usermod is installed.
   */
  uint16_t getId()
  {
    return USERMOD_ID_T_QT_DISPLAY;
  }
};
