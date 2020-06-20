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

  var click_time = 0;

  var current_map_params = "";
  function initMap() {
    var map = new google.maps.Map(document.getElementById('map'), {
      zoom: 4,
      center: {lat: -33, lng: 151},
      disableDefaultUI: true
    });
    // var layer = new google.maps.KmlLayer({
    //     // url: "http://www.google.com/maps/d/kml?mid=zNPjLbo835mE.k3TvWfqGG-AU",
    //     // https://www.google.com/maps/d/u/0/kml?mid=1dcALQQN6cRd5IfYzlf1FwijJM7xQM8LD
    //     url: "http://www.google.com/maps/d/kml?forcekml=1&mid=1dcALQQN6cRd5IfYzlf1FwijJM7xQM8LD",
    //     map: map
    // })
    // http://www.google.com/maps/d/kml?forcekml=1&mid=1dcALQQN6cRd5IfYzlf1FwijJM7xQM8LD
    // https://www.google.com/maps/d/u/0/kml?mid=1Vbldt9QSU5JU8YtWDHgtOd57kxAiBcZw
    goog.maps.kml.addInfoWindowsToMap([map], "http://www.google.com/maps/d/kml?forcekml=1&mid=1Vbldt9QSU5JU8YtWDHgtOd57kxAiBcZw");

    clk_func = function() { click_time = Date.now();};

    map.addListener('click', clk_func);
    map.addListener('drag', clk_func);

    setTimeout(function(){update_map(map);}, 1000)
  }
  

  var rotate_indexes = [];
  var cur_index = 0;
  var cur_window = null;
  var cur_marker = null;

  function update_map(map) {
    

    if (Date.now() - click_time < 5000) {
      setTimeout(function(){update_map(map);}, 1000);
      $.post( "/set_center", LatLng2LatLngLiteral(map.getCenter()), function( data ) {}, "json");
      cur_window.close(map, cur_marker);
      cur_marker = null;
      rotate_indexes = [];
      return;
    }
    else {
      setTimeout(function(){update_map(map);}, 5000);

      backend_api = "/get_map_params"
      var placeMarks = goog.maps.kml.placeMarks;
      $.getJSON( backend_api, {}).done(function( data ) {
          data_str = JSON.stringify(data)
          if (data_str != current_map_params) {
              console.info(data);
              current_map_params = data_str;
              rotate_indexes = [];
              cur_index = 0;
              for(var i=0;i<placeMarks.length;i++){ 
                if (haversine_distance(Marker2LatLngLiteral(placeMarks[i].marker), data) < data['range']) {
                  rotate_indexes.push(i);
                }
              }
          }
      });

      if (rotate_indexes.length == 0) {
        return
      }

      var i = rotate_indexes[cur_index];
      cur_index += 1
      cur_index %= rotate_indexes.length;
      next_window = placeMarks[i].infowindow;
      next_marker = placeMarks[i].marker;

      if (cur_marker == null) {
        next_window.open(map, next_marker);
      } else {
        pan_between_markers(map, cur_marker, cur_window, next_marker, next_window);
      }
      cur_window = next_window
      cur_marker = next_marker
  }
}

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
  