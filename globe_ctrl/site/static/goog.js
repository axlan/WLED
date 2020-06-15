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