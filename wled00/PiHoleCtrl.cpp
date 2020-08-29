#include "PiHoleCtrl.h"

#include <ESP8266HTTPClient.h>

#ifdef DEBUG_PI_HOLE
#ifdef DEBUG_PI_HOLE_PORT
#define DEBUG_PI_HOLE_PRINT(fmt, ...) DEBUG_PI_HOLE_PORT.printf_P( (PGM_P)PSTR(fmt), ## __VA_ARGS__ )
#endif
#endif

#ifndef DEBUG_PI_HOLE_PRINT
#define DEBUG_PI_HOLE_PRINT(...) do { (void)0; } while (0)
#endif

PiHoleCtrl::PiHoleCtrl(const String &server_address, const String &persistent_key):
  _index_url("http://" + server_address + "/admin/index.php"),
  _group_url("http://" + server_address + "/admin/scripts/pi-hole/php/groups.php"),
  _persistent_key_cookie("persistentlogin=" + persistent_key + ";") {}

bool PiHoleCtrl::enable_blacklist(WiFiClient &client, const String &name, bool is_enabled) {
  const BlackListItem* target_item = nullptr; 
  for(const BlackListItem &item : _blacklist_items) {
    if (item.comment.compareTo(name) == 0) {
      target_item = &item;
      break;
    }
  }
  if (target_item == nullptr) {
    return false;
  }

  DEBUG_PI_HOLE_PRINT("[PI] enable_blacklist...");
  String data = "action=edit_domain&id=" + String(target_item->id) + 
    "&type=" + String(target_item->type) +
    "&comment=" + name +
    "&status=" + String(is_enabled) +
    "&token=" + _token;
  DEBUG_PI_HOLE_PRINT("[PI] %s...", data.c_str());
  HTTPClient http;
  if (make_req(client, http, Method::POST, _group_url, &data)) {
    data = http.getString();
    DEBUG_PI_HOLE_PRINT("[PI] %s \n", data.c_str());
    http.end();
    return data.compareTo("{\"success\":true,\"message\":null}") == 0;
  }
  return false;
}

bool PiHoleCtrl::get_blacklists(WiFiClient &client) {
  if (_token.length() == 0) {
    get_token(client);
    return false;
  }
  String data = GET_BLACKLISTS + _token;
  HTTPClient http;
  DEBUG_PI_HOLE_PRINT("[PI] get_blacklists...");
  if (make_req(client, http, Method::POST, _group_url, &data)) {
    // data = http.getString();
    // DEBUG_PI_HOLE_PRINT("[PI] blacklists %s \n", data.c_str());
    DeserializationError error = deserializeJson(_json_doc, http.getStream());
    if (!error) {
      DEBUG_PI_HOLE_PRINT("[PI] blacklists: ");
      //{"data":[{"id":2,"type":3,"domain":"(\\.|^)reddit\\.com$","enabled":0,"date_added":1597384685,"date_modified":1597622169,"comment":"reddit","groups":[0]},{"id":3,"type":1,"domain":"vpn.swiftnav.com","enabled":0,"date_added":1597385916,"date_modified":1597430138,"comment":null,"groups":[0]}]}
      JsonArray items = _json_doc["data"].as<JsonArray>();
      _blacklist_items.clear();
      for(JsonVariant v : items) {
        BlackListItem item = {
          v["id"],
          v["type"],
          v["comment"],
          v["enabled"]};
        _blacklist_items.push_back(item);
        DEBUG_PI_HOLE_PRINT("%s, ", item.comment.c_str());
      }
      DEBUG_PI_HOLE_PRINT("\n");
      http.end();
      return true;
    } else {
      _token.clear();
      _php_session_cookie.clear();
      DEBUG_PI_HOLE_PRINT("[PI] json failed: %s\n", error.c_str());
    }
    http.end();
    return true;
  }
  return false;
}

  
bool PiHoleCtrl::enable_group(WiFiClient &client, const String &name, bool is_enabled) {
  const GroupItem* target_item = nullptr; 
  for(const GroupItem &item : _group_items) {
    if (item.name.compareTo(name) == 0) {
      target_item = &item;
      break;
    }
  }
  if (target_item == nullptr) {
    return false;
  }

  DEBUG_PI_HOLE_PRINT("[PI] enable_group...");
  String data = "action=edit_group&id=" + String(target_item->id) + 
    "&name=" + name +
    "&desc=" + target_item->description +
    "&status=" + String(is_enabled) +
    "&token=" + _token;
  DEBUG_PI_HOLE_PRINT("[PI] %s...", data.c_str());
  HTTPClient http;
  if (make_req(client, http, Method::POST, _group_url, &data)) {
    data = http.getString();
    DEBUG_PI_HOLE_PRINT("[PI] %s \n", data.c_str());
    http.end();
    return data.compareTo("{\"success\":true,\"message\":null}") == 0;
  }
  return false;
}

bool PiHoleCtrl::get_groups(WiFiClient &client) {
  if (_token.length() == 0) {
    get_token(client);
    return false;
  }
  String data = GET_GROUPS + _token;
  HTTPClient http;
  DEBUG_PI_HOLE_PRINT("[PI] get_groups...");
  if (make_req(client, http, Method::POST, _group_url, &data)) {
    // data = http.getString();
    // DEBUG_PI_HOLE_PRINT("[PI] blacklists %s \n", data.c_str());
    DeserializationError error = deserializeJson(_json_doc, http.getStream());
    if (!error) {
      DEBUG_PI_HOLE_PRINT("[PI] groups: ");
      //{"data":[{"id":2,"type":3,"domain":"(\\.|^)reddit\\.com$","enabled":0,"date_added":1597384685,"date_modified":1597622169,"comment":"reddit","groups":[0]},{"id":3,"type":1,"domain":"vpn.swiftnav.com","enabled":0,"date_added":1597385916,"date_modified":1597430138,"comment":null,"groups":[0]}]}
      JsonArray items = _json_doc["data"].as<JsonArray>();
      _group_items.clear();
      for(JsonVariant v : items) {
        GroupItem item = {
          v["id"],
          v["name"],
          v["description"],
          v["enabled"]};
        _group_items.push_back(item);
        DEBUG_PI_HOLE_PRINT("%s, ", item.name.c_str());
      }
      DEBUG_PI_HOLE_PRINT("\n");
      http.end();
      return true;
    } else {
      _token.clear();
      _php_session_cookie.clear();
      DEBUG_PI_HOLE_PRINT("[PI] json failed: %s\n", error.c_str());
    }
    http.end();
    return true;
  }
  return false;
}

bool PiHoleCtrl::make_req(WiFiClient &client, HTTPClient &http, Method method, const String &url, String *payload) {
  
  if (!http.begin(client, url)) {
    DEBUG_PI_HOLE_PRINT("[PI} Unable to connect\n");
    return false;
  }
  
  if (_php_session_cookie.length() > 0) {
    http.addHeader("Cookie", _persistent_key_cookie + " " + _php_session_cookie);
  } else {
    http.addHeader("Cookie", _persistent_key_cookie);
  }
  http.addHeader("Content-type", "application/x-www-form-urlencoded; charset=UTF-8");

  const char * headers[] = {"Set-Cookie"};
  DEBUG_PI_HOLE_PRINT("[PI] SEND...");
  http.collectHeaders(headers, 1);
  int httpCode;
  // start connection and send HTTP header
  switch(method)
  {
      case GET:
        httpCode = http.GET();
        break;
      case POST:
        httpCode = http.POST(*payload);
        break;
      default:
        httpCode = -1;
        break;
  }

  // HTTP header has been send and Server response header has been handled
  // httpCode will be negative on error
  if (httpCode > 0) {
    DEBUG_PI_HOLE_PRINT("[PI] size: %d...", http.getSize());
    // file found at server
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      return true;
    } else {
      DEBUG_PI_HOLE_PRINT("[PI] bad code: %d\n", httpCode);
    }
  } else {
    DEBUG_PI_HOLE_PRINT("[PI] SEND... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  return false;
}

bool PiHoleCtrl::get_token(WiFiClient &client){
  HTTPClient http;
  DEBUG_PI_HOLE_PRINT("[PI] get_token\n");
  if (make_req(client, http, Method::GET, _index_url)) {
    WiFiClient* payload_client = http.getStreamPtr();
    if (http.hasHeader("Set-Cookie")) {
      //PHPSESSID=f0ahlhjddv4jrg2v4i400pe8c2; path=/,persistentlogin=35ca35cbae1861dfad63fe000578de08e4409c5450f7d6310ad7e4c834a4fce7; expires=Fri, 28-Aug-2020 23:34:02 GMT; Max-Age=604800
      if (http.header("Set-Cookie").startsWith("PHPSESSID=")) {
          _php_session_cookie = http.header("Set-Cookie").substring(0, 37);
          DEBUG_PI_HOLE_PRINT("[PI] session: %s...", _php_session_cookie.c_str());
          while (payload_client->available()) {
            String line = payload_client->readStringUntil('\n');
            //<div id="token" hidden>dzRSDSIG9GCwSTehSoM7pLW2+vcTmHWy6nsql69+Bac=</div>
            if (line.startsWith("<div id=\"token\"")) {
              // replace = with %3D
              _token = line.substring(23, 66) + "%3D";
              _token.replace("+", "%2B");
              DEBUG_PI_HOLE_PRINT("[PI] token: %s\n", _token.c_str());
              http.end();
              return true;
            }
          }
          DEBUG_PI_HOLE_PRINT("[PI] no token\n");
      } else {
        DEBUG_PI_HOLE_PRINT("[PI] no PHPSESSID in %s\n", http.header("Set-Cookie").c_str());
      }
    } else {
      DEBUG_PI_HOLE_PRINT("[PI] no cookies\n");
    }
    http.end();
  }
  return false;
}
