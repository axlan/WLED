#include <vector>

#include "wled.h"

class HTTPClient;

class PiHoleCtrl {

public:
  PiHoleCtrl(const String &server_address, const String &persistent_key);

  bool enable_blacklist(WiFiClient &client, const String &name, bool is_enabled);
  bool get_blacklists(WiFiClient &client);

  bool enable_group(WiFiClient &client, const String &name, bool is_enabled);
  bool get_groups(WiFiClient &client);

private:

  struct BlackListItem {
    int id;
    int type;
    String comment;
    bool enabled;
  };

  struct GroupItem {
    int id;
    String name;
    String description;
    bool enabled;
  };

  enum Method { GET, POST };

  bool make_req(WiFiClient &client, HTTPClient &http, Method method, const String &url, String *payload=nullptr);

  bool get_token(WiFiClient &client);

  const String _index_url;
  const String _group_url;
  const String _persistent_key_cookie;
  const char* GET_BLACKLISTS = "action=get_domains&showtype=black&token=";
  const char* GET_GROUPS = "action=get_groups&token=";
  String _token;
  String _php_session_cookie;
  std::vector<BlackListItem> _blacklist_items;
  std::vector<GroupItem> _group_items;
  StaticJsonDocument<1024> _json_doc;

public:
  const std::vector<BlackListItem>& get_blacklist_items();
  const std::vector<GroupItem>& get_group_items();
};
