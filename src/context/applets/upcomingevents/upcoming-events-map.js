var map;
var markers = [];

function initialize() {
    var latitude = 0;
    var longitude = 0;
    var myLatlng = new google.maps.LatLng(latitude,longitude);
    var myOptions = {
        zoom: 16,
        center: myLatlng,
        mapTypeId: google.maps.MapTypeId.ROADMAP,
        mapTypeControlOptions: {
            style: google.maps.MapTypeControlStyle.DROPDOWN_MENU
        }
    }
    map = new google.maps.Map(document.getElementById("map_canvas"), myOptions);
}

function centerAt(latitude, longitude) {
    myLatlng = new google.maps.LatLng(latitude,longitude);
    map.panTo(myLatlng);
}

function addMarker(latitude, longitude, icon, info) {
    var marker = new google.maps.Marker({
        position: new google.maps.LatLng(latitude,longitude),
        map: map,
        icon: icon
    });
    var infowindow = new google.maps.InfoWindow({
        content: info,
        maxWidth: 300
    });
    google.maps.event.addListener(marker, 'click', function() {
        infowindow.open(map,marker);
    });
    markers.push(marker);
}

function removeMarker(latitude, longitude) {
    if (markers) {
        var pos = new google.maps.LatLng(latitude,longitude);
        for (var i=markers.length-1; i>=0; i--) {
            var markerPos = markers[i].getPosition();
            if (pos.equals(markerPos)) {
                markers[i].setMap(null);
                markers.splice(i, 1);
            }
        }
    }
}

function clearMarkers() {
    if (markers) {
        for (x in markers) {
            markers[x].setMap(null);
        }
    }
    markers.length = 0;
}
// vim:et:sw=4:sts=4:ts=4:
