#pragma once

#include "wled.h"

#include "PiHoleCtrl.h"

class PiHoleSwitch : public Usermod {
  private:
    std::unique_ptr<PiHoleCtrl> pi_ctrl;
    String pi_host;
    String pi_key;
    unsigned long next_update_time = 0;
    const unsigned long UPDATE_PERIOD = 10000;
    const int GROUP1_PIN = 4;
    const int GROUP2_PIN = 5;

  public:
    //Functions called by WLED

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     */
    void setup() {
      pinMode(GROUP1_PIN, INPUT);
      pinMode(GROUP2_PIN, INPUT);
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
      bool updated = false;
      if (!pi_host.equals(piholeServer)) {
        pi_host = piholeServer;
        updated = true;
      }
      if (!pi_key.equals(piholeKey)) {
        pi_key = piholeKey;
        updated = true;
      }
      if (updated) {
        pi_ctrl = std::unique_ptr<PiHoleCtrl>(new PiHoleCtrl(pi_host, pi_key));
      }

      if (WLED_CONNECTED && pi_ctrl && millis() > next_update_time) {
        WiFiClient client;
        if (pi_ctrl->_group_items.size() > 0) {
          bool failed = false;
          if (strnlen_P(piholeGroup1, 15) > 0) {
            failed &= pi_ctrl->enable_group(client, piholeGroup1, digitalRead(GROUP1_PIN) == HIGH);
          }
          if (!failed && strnlen_P(piholeGroup2, 15) > 0) {
            failed &= pi_ctrl->enable_group(client, piholeGroup2, digitalRead(GROUP2_PIN) == HIGH);
          }
          if (failed) {
            pi_ctrl->_group_items.clear();
          }
        } else {
          pi_ctrl->get_groups(client);
        }
        if (piholeLed >= 0) {
          //setRealtimePixel(0, 255, 0, 0, 0);
          if (digitalRead(GROUP1_PIN) == HIGH) {
            strip.getSegment(piholeLed).colors[0] = RED;
          } else if (digitalRead(GROUP2_PIN) == HIGH) {
            strip.getSegment(piholeLed).colors[0] = GREEN;
          } else {
            strip.getSegment(piholeLed).colors[0] = BLUE;
          }
          colorUpdated(NOTIFIER_CALL_MODE_FX_CHANGED);
        }
        next_update_time = millis() + UPDATE_PERIOD;
      }
      
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
     
    }
   
    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     */
    uint16_t getId()
    {
      return USERMOD_ID_IMU;
    }

};