#pragma once

#include "wled.h"

#include "PiHoleCtrl.h"

class PiHoleSwitch : public Usermod {
  private:
    PiHoleCtrl *pi_ctrl;

  public:
    //Functions called by WLED

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     */
    void setup() {
      pi_ctrl = new PiHoleCtrl("dummy1", "dummy2");
    }

    /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
    void connected() {
      //Serial.println("Connected to WiFi!");
    }


    /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     */
    void loop() {
     
    }



    void addToJsonInfo(JsonObject& root)
    {
    
    }


    /*
     * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void addToJsonState(JsonObject& root)
    {
      //root["user0"] = userVar0;
    }


    /*
     * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void readFromJsonState(JsonObject& root)
    {
      //if (root["bri"] == 255) Serial.println(F("Don't burn down your garage!"));
    }
    
   
    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     */
    uint16_t getId()
    {
      return USERMOD_ID_IMU;
    }

};