
// nuber of bytes for the display
#define NUMPIXELS      64

// http://esp8266-server.de/wemos.html#Pinbelegung


//const char* ssid = "xxxxxxxxxxxxxxx";
//const char* password = "xxxxxxxxxxxxxxxxxx";
//const char* mqtt_server = "192.168.1.121";

const char* ssid = "mh";
const char* password = "xxxxxxxxxxxxxx";
const char* mqtt_server = "10.2.3.2";

const char* mqtt_client_id = "redshift2";          //has to be uniq per MQTT Broker!!!
const char* inTopic = "huette/clubraum/000/redshift/actuators/frame";
const char* inTopic2 ="huette/clubraum/000/redshift/actuators/set_pixel";

const char* outTopic ="huette/clubraum/000/redshift/status";



