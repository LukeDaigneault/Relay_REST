#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
#include <string.h>

ESP8266WebServer server(80);

int relay1_pin = 4;

boolean relay1_state = false; // Turned on by default

boolean relay_module_active = HIGH;  // Are the relays turned on by going HIGH or LOW?

String page = "<!DOCTYPE html>\n<html>\n<head>\n<meta http-equiv=Content-Type content=\"text/html; charset=utf-8\" />\n<meta http-equiv=X-UA-Compatible content=\"IE=8,IE=9,IE=10\">\n<meta name=viewport content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0\"/>\n<style>body{font-family:sans-serif}.switch{cursor:pointer;width:5.5em;border:1px solid #5f645b;color:#fff;border-radius:.8em;margin-left:20px}.toggle,.state{margin:.1em;font-size:130%;font-weight:normal;text-align:center;float:left}.toggle{width:1.1em;background-color:#f5f5f5;color:#000;text-align:center;border:1px solid grey;border-radius:.5em;margin-right:.1em;margin-left:.2em}.state{padding-top:.05em;width:2em}.on{background-color:#56c94d}.off{background-color:#eceeef;color:#aaaab8}.on .toggle{float:right}.off .toggle{float:left}.clearfix{clear:both}table td{vertical-align:middle}table h2{margin:0;padding:0;font-weight:normal;margin-top:4px}</style>\n<script>function toggle(a){document.location.href=\"relay\"+a+\"/toggle\"};</script>\n</head>\n<body>\n<h1>Relay Manager</h1>\n<table border=0>\n<tr><td>\n<h2>Relay 1</h1>\n</td><td>\n<div class=\"switch {{relay1_state}}\" onclick=toggle(1)>\n<div class=toggle>&nbsp;</div>\n<div class=state>{{relay1_state}}</div><br class=clearfix />\n</div>\n</td></tr>\n</table>\n</body>\n</html>";

String get_human_state(bool relay_state){
    if(relay_state == 1){
        return "on";
    }else{
        return "off";
    }
}

String get_page(){
    String x = page;
    x.replace("{{relay1_state}}", get_human_state(relay1_state));
    return x;
}

void handle_root() {
    Serial.println("Request incoming...");
    server.send(200, "text/html", get_page());
    delay(100);
    Serial.println("Request handled.");
}

void handle_not_found() {
    Serial.println("404 Not Found ");
    server.send(404, "text/plain", "Not Found. You requested \"" + server.uri() + "\"");
    delay(100);
    Serial.println("Request handled.");
}

int get_relay_pin(int relay_number){
    if(relay_number == 1){
        return relay1_pin;
    }else
    {
      return 0;
    }

}

bool get_relay_state(int relay_number){
    if (relay_number == 1){
        return relay1_state;
    }else
    {
      return 0;
    }

}

void set_relay_state(int relay_number, bool state){
    if (relay_number == 1){
        relay1_state = state;
    }

}

void set_relay(int relay_number, bool state){
    // Responsible for implementin the pin change and keeping track of the pin state.
    // This method could be less verbose, but I like easily readable code.

    if (state == 1){
        // We want to turn this relay on, so we need to take it low
        digitalWrite(get_relay_pin(relay_number), relay_module_active);
        set_relay_state(relay_number, 1);
    }else{
        // We want to turn this relay off, so we need to take it high
        digitalWrite(get_relay_pin(relay_number), !relay_module_active);
        set_relay_state(relay_number, 0);
    }
}

void handle_relay() {
    int relay_number;

    if (server.uri() == "/api/relay1"){
        relay_number = 1;
    }

    if (server.method() == HTTP_GET){
        Serial.println("GET REQUESTed");
        server.send(200, "text/json", "{\"state\": " + String(get_relay_state(relay_number)) + "}");

    }else if (server.method() == HTTP_POST){
        Serial.println("POST REQUESTed");

        for ( uint8_t i = 0; i < server.args(); i++ ) {
            Serial.println( "" + server.argName ( i ) + ": " + server.arg ( i ));
            if (server.argName(i) == "state"){
                set_relay(relay_number, bool(server.arg(i).toInt()));
                server.send(200, "text/json", "{\"state\": " + String(get_relay_state(relay_number)) + ", \"set\": 1}");
            }
        }
    }else{
        server.send(501, "text/json", "{\"error\": \"Not Implemented: " + String(server.method()) + "\"}");
    }

    delay(100);
}

void handle_toggle(){
    int relay_number;

    if (server.uri() == "/relay1/toggle"){
        relay_number = 1;
    }

    set_relay(relay_number, !get_relay_state(relay_number));

    server.sendHeader("Location", "/", true);
    server.send (302, "text/plain", "");
    server.client().stop();
}


void setup() {
    // Set up the pins and apply the default state.
    pinMode(relay1_pin, OUTPUT);
    set_relay(1,relay1_state);

    Serial.begin(115200);

    WiFiManager wifiManager;
    //wifiManager.resetSettings();
    wifiManager.setTimeout(180);
  //  wifiManager.setSTAStaticIPConfig(sta_ip, sta_gw, sta_sn);

    if(!wifiManager.autoConnect("AutoConnectAP")) {
        Serial.println("Failed to connect and hit timeout");
        delay(3000);
        ESP.reset();
        delay(5000);
    }

    Serial.println("Connected to Wifi...");

    server.on("/", handle_root);
    server.on("/api/relay1", handle_relay);
    server.on("/relay1/toggle", handle_toggle);

    server.onNotFound (handle_not_found);
    server.begin();
    Serial.println("Server Started!");
}

void loop() {
    server.handleClient();
}
