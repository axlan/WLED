#pragma once

#include <TFT_eSPI.h>

extern TFT_eSPI tft;

static void PrintRoll0() {
  tft.setTextSize(2);
  tft.setTextColor(63488);
  tft.println("Barb Chain");
  tft.setTextColor(65535);
  tft.println("Atk/CMD 12");
  tft.println("Range: 70");
  tft.setTextSize(1);
  tft.println("Summon 3 chains. Make");
  tft.println("a melee atk 1d6 or a ");
  tft.println("trip CMD=AT. On a hit");
  tft.println("make Will save or sha");
  tft.println("ken 1d4 rnds.");
}

static void PrintRoll1() {
  tft.setTextSize(2);
  tft.setTextColor(2016);
  tft.println("Saves");
  tft.setTextColor(65535);
  tft.println("FORT 8");
  tft.println("REFLEX 8");
  tft.println("WILL 9");
}

static const char* GetRollName(uint8_t key) {
  switch (key) {
    case 0:
      return "Barb Chain";
    case 1:
      return "Saves";
  }
  return "";
}

static void PrintRollInfo(uint8_t key) {
  switch (key) {
    case 0:
      PrintRoll0();
      return;
    case 1:
      PrintRoll1();
      return;
  }
  tft.setTextColor(TFT_RED);
  tft.setCursor(0, 60);
  tft.setTextSize(2);
  tft.println("Unknown");
}
