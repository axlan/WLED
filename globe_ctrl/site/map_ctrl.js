
GOOGLE_API_KEY = "????????"
var GOOGLE_API_URL = "https://maps.googleapis.com/maps/api/js?key=" + GOOGLE_API_KEY + "&callback=initMap"

//var GLOBE_PATH = "http://192.168.1.100/json"
var GLOBE_PATH = "/json"

//////// KML load and parsing
// https://gist.github.com/rborn/3317635
var goog = {
  maps: {
    kml: {}
  }
};
  
(function(){

  function xmlParser(xml, maps) {
    var kml = goog.maps.kml.parse(xml);
    if( typeof(maps) && !(maps.length > 0) ){ maps = [maps]; }
    for(var m=0;m<maps.length;m++){
      goog.maps.kml.addInfoWindows(maps[m], kml);
    }
  }

  goog.maps.kml.addInfoWindowsToMap = function(maps, url){
    $.ajax({
      type: "GET",
      url: url,
      dataType: "xml",
      success: function(xml){ xmlParser(xml, maps)}
      });
  };

  goog.maps.kml.addInfoWindows = function(map, kml){
    goog.maps.kml.placeMarks = [];
    for(var pm=0; pm<kml.placeMarksLength; pm++){
      
      var infowindow = new google.maps.InfoWindow({
        content: "<img src='" + kml.placeMarks.item(pm).getElementsByTagName("value")[0].innerHTML + "'>"
      });

      var pos = kml.placeMarks.item(pm).getElementsByTagName("coordinates")[0].innerHTML.trim().split(',');
      pos = {lat: parseFloat(pos[1]), lng: parseFloat(pos[0])};

      var marker = new google.maps.Marker({
        position: pos,
        map: map,
        title: kml.placeMarks.item(pm).getElementsByTagName("name")[0].innerHTML
      });
      marker.addListener('click', function() {
        infowindow.open(map, marker);
      });
      goog.maps.kml.placeMarks.push({
        "marker": marker,
        "infowindow": infowindow
      });
    }
  };
  
  goog.maps.kml.parse = function(xml){
    var kml = {
      xml: xml,
      placeMarks: goog.maps.kml.getPlaceMarks(xml)
    };
    kml.placeMarksLength = kml.placeMarks.length;
    return kml;
  };
  
  goog.maps.kml.getPlaceMarks = function(xml){
    return xml.documentElement.getElementsByTagName("Placemark");
  };
  
})();
//////// coordinate conversions
function Marker2LatLngLiteral(marker) {
  return LatLng2LatLngLiteral(marker.getPosition());
}
function LatLng2LatLngLiteral(point) {
  return {"lat": point.lat(), "lng": point.lng()};
}

function GetBoundsDiagDistance(bounds) {
  var mk1 = LatLng2LatLngLiteral(bounds.getNorthEast());
  var mk2 = LatLng2LatLngLiteral(bounds.getSouthWest());
  return haversine_distance(mk1, mk2);
}
//https://gis.stackexchange.com/questions/7430/what-ratio-scales-do-google-maps-zoom-levels-correspond-to
//metersPerPx = 156543.03392 * Math.cos(latLng.lat() * Math.PI / 180) / Math.pow(2, zoom)
function haversine_distance(mk1, mk2) {
  var R = 3958.8; // Radius of the Earth in miles To use kilometers, set R = 6371.0710
  var rlat1 = mk1.lat * (Math.PI/180); // Convert degrees to radians
  var rlat2 = mk2.lat * (Math.PI/180); // Convert degrees to radians
  var difflat = rlat2-rlat1; // Radian difference (latitudes)
  var difflon = (mk2.lng-mk1.lng) * (Math.PI/180); // Radian difference (longitudes)

  var d = 2 * R * Math.asin(Math.sqrt(Math.sin(difflat/2)*Math.sin(difflat/2)+Math.cos(rlat1)*Math.cos(rlat2)*Math.sin(difflon/2)*Math.sin(difflon/2)));
  return d;
}
////////// WLED interface

FACE_CENTERS = [
  {
    "face": "SF",
    "center": [0.3393089,
               0.91804951,
               -0.17690642]
  },
  {
    "face": "NY",
    "center": [0.90721371,
               0.40666377,
               -0.07984313]
  },
  {
    "face": "ES",
    "center": [0.87375028,
               0.1569832,
               0.45839751]
  },
  {
    "face": "EU",
    "center": [0.71543029,
               -0.44834221,
               0.53423385]
  },
  {
    "face": "VN",
    "center": [-0.05432586,
               -0.99326585,
               -0.08080061]
  },
  {
    "face": "NZ",
    "center": [-0.97666959,
               -0.18362631,
               -0.08076036]
  },
  {
    "face": "TA",
    "center": [-0.82805054,
               0.55281974,
               -0.05135381]
  }
]
var GYRO_THRESH = 10;
var CITY_LEDS = [
  {"name": "new york", "face": "NY", "lat": 41.0083894, "lng": -74.1151662, "zoom": 10.0, "range": 100, "steps": [2]},
  {"name": "vancouver", "face": "SF", "lat": 49.2577143, "lng": -123.1939434, "zoom": 12.0, "range": 100, "steps": [1]},
  {"name": "san francisco", "face": "SF", "lat": 37.7183109, "lng": -122.4317655, "zoom": 11.0, "range": 100, "steps": [0]},
  {"name": "tahiti", "face": "TA", "lat": -17.4940458, "lng": -149.7985391, "zoom": 11.77, "range": 100, "steps": [31]},
  {"name": "aukland", "face": "NZ", "lat": -36.8626942, "lng": 174.5852864, "zoom": 10, "range": 100, "steps": [28, 30]},
  {"name": "nelson", "face": "NZ", "lat": -41.0996613, "lng": 172.6374937, "zoom": 9.4, "range": 100, "steps": [29]},
  {"name": "cat ba", "face": "VN", "lat": 20.7637212, "lng": 107.0065949, "zoom": 11.56, "range": 50, "steps": [19]},
  {"name": "hanoi", "face": "VN", "lat": 21.0378589, "lng": 105.8301002, "zoom": 13.48, "range": 50, "steps": [18, 20, 22]},
  {"name": "sa pa", "face": "VN", "lat": 22.3618903, "lng": 103.7483728, "zoom": 12.74, "range": 100, "steps": [21]},
  {"name": "chiang mai", "face": "VN", "lat": 18.793535, "lng": 98.8986844, "zoom": 12.26, "range": 100, "steps": [23]},
  {"name": "bangkok", "face": "VN", "lat": 13.7245601, "lng": 100.4930254, "zoom": 11, "range": 100, "steps": [17, 24, 26]},
  {"name": "ko tao", "face": "VN", "lat": 10.0922781, "lng": 99.8178946, "zoom": 14, "range": 100, "steps": [25]},
  {"name": "singapore", "face": "VN", "lat": 1.1755638, "lng": 103.7229753, "zoom": 10.22, "range": 100, "steps": [27]},
  {"name": "greece", "face": "EU", "lat": 37.1446277, "lng": 23.9410502, "zoom": 8.99, "range": 100, "steps": [15]},
  {"name": "prague", "face": "EU", "lat": 50.0595854, "lng": 14.325542, "zoom": 13, "range": 100, "steps": [6]},
  {"name": "berlin", "face": "EU", "lat": 52.5067614, "lng": 13.2846502, "zoom": 11, "range": 100, "steps": [5]},
  {"name": "munich", "face": "EU", "lat": 48.1548256, "lng": 11.4017536, "zoom": 11, "range": 100, "steps": [7]},
  {"name": "geneva", "face": "EU", "lat": 45.8195253, "lng": 6.6791726, "zoom": 10.92, "range": 100, "steps": [8]},
  {"name": "amsterdam", "face": "EU", "lat": 52.3362167, "lng": 4.7695917, "zoom": 10.96, "range": 100, "steps": [4]},
  {"name": "paris", "face": "ES", "lat": 48.8588377, "lng": 2.2770204, "zoom": 12, "range": 100, "steps": [3, 9]},
  {"name": "lagos", "face": "ES", "lat": 37.09884, "lng": -8.6960644, "zoom": 11.26, "range": 100, "steps": [12]},
  {"name": "seville", "face": "ES", "lat": 37.3753501, "lng": -6.0250981, "zoom": 12, "range": 100, "steps": [11, 13]},
  {"name": "barcelona", "face": "EU", "lat": 41.3947688, "lng": 2.0787282, "zoom": 12, "range": 100, "steps": [10, 14]},
  {"name": "naples", "face": "EU", "lat": 40.8321129, "lng": 14.0084072, "zoom": 11.22, "range": 100, "steps": [16]}
]
STOP_STEP = 32;

function calculate_norm(array) {
  var sum = 0;
  for(var i=0;i<array.length;i++){
    sum += array[i] ** 2;
  }
  return Math.sqrt(sum);
}

function array_add(a1, a2) {
  var ret = [];
  for(var i=0;i<a1.length;i++){
    ret.push(a1[i] + a2[i]);
  }
  return ret;
}

function array_sub(a1, a2) {
  var ret = [];
  for(var i=0;i<a1.length;i++){
    ret.push(a1[i] - a2[i]);
  }
  return ret;
}

function get_imu_cmd(imu_data) {
  var best_face = null;
  var best_dist = 100;
  for(var i=0;i<FACE_CENTERS.length;i++){ 
    center = FACE_CENTERS[i]['center']
    face = FACE_CENTERS[i]['face']
    var distance = calculate_norm(array_sub(imu_data.Gravity, center))
    if (distance < best_dist) {
        best_face = face
        best_dist = distance
    }
    //console.log(face, distance);
  }
  gyro_mag = calculate_norm(imu_data.Gyro);
  return {"face": best_face, "moving": gyro_mag > GYRO_THRESH};
}

function chase_leds() {
  var data = {'seg': [
  {
    "id": 0,
    "start": 0,
    "len": STOP_STEP,
    "col": [[255, 0,  0], [0, 0, 0], [0, 0, 0]],
    "fx": 30,
    "sx": 20,
    "ix": 0,
    "pal": 9
  },
  {
    "id": 1,
    "start": 0,
    "stop": 0
  },
  {
    "id": 2,
    "start": 0,
    "stop": 0
  }]};
  navigator.sendBeacon(GLOBE_PATH, JSON.stringify(data))
}

function highlight(target) {
  target_step = CITY_LEDS[target].steps[0];
  var data = {'seg': []}
  data['seg'].push({
    "id": 0,
    "start": 0,
    "stop": target_step,
    "col": [[0, 0,  255], [0, 0, 0], [0, 0, 0]],
    'fx': 15,
    // Relative effect speed
    'sx': 128,
    // Effect intensity
    'ix': 128,
    // Palette index
    "pal": 0
  })
  data['seg'].push({
      "id": 2,
      "start": target_step,
      "stop": target_step + 1,
      "col": [[255, 0,  0], [0, 0, 0], [0, 0, 0]],
      'fx': 0,
      'sx': 128,
      'ix': 128,
      "pal": 0
    })
  data['seg'].push({
    "id": 1,
    "start": target_step + 1,
    "stop": STOP_STEP,
    "col": [[0, 0,  255], [0, 0, 0], [0, 0, 0]],
    'fx': 15,
    'sx': 128,
    'ix': 128,
    "pal": 0
  })
  $.ajax({
    type: "POST",
    url: GLOBE_PATH,
    // The key needs to match your method's input parameter (case-sensitive).
    data: JSON.stringify(data),
    contentType: "application/json; charset=utf-8",
    dataType: "json",
    success: function(result){
        //console.log(result);
    },
    failure: function(errMsg) {
        console.log('error: ' + errMsg);
    }
  });
}

function get_closest_city(point) {
  var closest_dist = 10**10;
  var closest_idx = null;
  for(var i=0;i<CITY_LEDS.length;i++){
    var d = haversine_distance(point, CITY_LEDS[i]);
    if (d < closest_dist) {
      closest_dist = d;
      closest_idx = i;
    }
  }
  return closest_idx;
}

//////// KML load and parsing
var click_time = 0;
var picture_refresh_time = 0;
var rotate_indexes = [];
var cur_index = 0;
var cur_window = null;
var cur_marker = null;
var selected = {'idx': -1, 'step': 0, 'face': '', "locked": false}

var current_map_params = "";
function initMap() {
  var map = new google.maps.Map(document.getElementById('map'), {
    zoom: 4,
    center: {lat: -33, lng: 151},
    disableDefaultUI: true
  });
  goog.maps.kml.addInfoWindowsToMap([map], "http://www.google.com/maps/d/kml?forcekml=1&mid=1Vbldt9QSU5JU8YtWDHgtOd57kxAiBcZw");

  clk_func = function() { click_time = Date.now();};

  map.addListener('click', clk_func);
  map.addListener('drag', clk_func);

  setTimeout(function(){update_map(map);}, 1000);
  window.onbeforeunload = closingCode;
}

function select_city(idx) {
  if (selected.idx == idx) {
    return;
  }
  var placeMarks = goog.maps.kml.placeMarks;
  selected.idx = idx;
  rotate_indexes = [];
  cur_index = 0;
  var city = CITY_LEDS[idx]
  for(var i=0;i<placeMarks.length;i++){ 
    if (haversine_distance(Marker2LatLngLiteral(placeMarks[i].marker), city) < city['range']) {
      rotate_indexes.push(i);
    }
  }
  highlight(idx)
}

function closingCode(){
  chase_leds();
  return null;
}

function update_map(map) {
  
  var placeMarks = goog.maps.kml.placeMarks;
  
  if (placeMarks == null || placeMarks.length <= 1) {
    setTimeout(function(){update_map(map);}, 500);
    return;
  }

  if (Date.now() - click_time < 5000) {
    setTimeout(function(){update_map(map);}, 500);
    selected.locked = true;
    var closest_idx = get_closest_city(LatLng2LatLngLiteral(map.getCenter()));
    if (cur_marker != null) {
      cur_window.close(map, cur_marker);
      cur_marker = null;
    }
    select_city(closest_idx);
    return;
  }
  else {
    $.getJSON( GLOBE_PATH, {}).done(function( data ) {
      var imu_data = data.info.u.IMU
      //console.log(imu_data);
      var imu_cmd = get_imu_cmd(imu_data);
      //console.log(imu_cmd);

      if ((!selected.locked && selected.face != imu_cmd.face) || imu_cmd.moving) {
        selected.locked = false;
        var face_steps = []
        for(var i=0;i<CITY_LEDS.length;i++){ 
          city = CITY_LEDS[i]
          if (imu_cmd.face == city['face']) {
            var entry = {"i": i, "step": city['steps'][0]} ;
            face_steps.push(entry);
          }
        }
        face_steps.sort(function(a, b) {
          return a.step - b.step;
        });

        if (selected.face != imu_cmd.face) {
          selected['step'] = 0;
          selected.face = imu_cmd.face;
        } else if (imu_cmd.moving) {
          selected['step'] += 1;
          selected['step'] %= face_steps.length;
        }

        var new_idx = face_steps[selected['step']].i
        select_city(new_idx);
      }
      //console.log(selected);
      if (Date.now() > picture_refresh_time + 5000) { 
        picture_refresh_time = Date.now();
        var i = rotate_indexes[cur_index];
        cur_index += 1
        cur_index %= rotate_indexes.length;
        next_window = placeMarks[i].infowindow;
        next_marker = placeMarks[i].marker;

        if (cur_marker == null) {
          next_window.open(map, next_marker);
        } else if(cur_marker != next_marker) {
          pan_between_markers(map, cur_marker, cur_window, next_marker, next_window);
        }
        cur_window = next_window
        cur_marker = next_marker
      }
    }).always(function(){setTimeout(function(){update_map(map);}, 500);});
  }
}

//////// Google Map animations
// https://stackoverflow.com/questions/3817812/google-maps-v3-can-i-ensure-smooth-panning-every-time
function pan_between_markers(map, marker1, window1, marker2, window2) {
  var new_bounds = new google.maps.LatLngBounds(marker1.getPosition());
  var listener;
  new_bounds.extend(marker2.getPosition());
  
  var cur_bounds = map.getBounds();

  var d1 = GetBoundsDiagDistance(cur_bounds);
  var d2 = GetBoundsDiagDistance(new_bounds);

  if (d2 / d1 > 10.0) {        
    var zoom = map.getZoom() - 3.0
    map.setZoom(zoom);
    listener = google.maps.event.addListener(map, 'idle', function(){
      partial_zoom(map, marker1, window1, marker2, window2);
    });
    return;
  }
  
  listener = google.maps.event.addListener(map, 'idle', open_marker);
  map.fitBounds(new_bounds);

  function open_marker() {
    google.maps.event.removeListener(listener);
    window2.open(map, marker2);
    listener = google.maps.event.addListener(map, 'idle', close_marker);
  }
  function close_marker() {
    google.maps.event.removeListener(listener);
    window1.close(map, marker1);
  }
  function partial_zoom(map, marker1, window1, marker2, window2) {
    google.maps.event.removeListener(listener);
    pan_between_markers(map, marker1, window1, marker2, window2);
  }
}

// Load Google Maps API Javascript
var fileref=document.createElement('script')
fileref.setAttribute("type","text/javascript")
fileref.setAttribute("src", GOOGLE_API_URL)
document.getElementsByTagName("head")[0].appendChild(fileref)
