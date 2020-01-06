#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <noWheel.h>
//#include <colorWheel.h>
#include <web_iro_js.h>
#include <admin.h>

#define RED_LED 14
#define GREEN_LED 12
#define BLUE_LED 5

#define BUILTIN_LED 2

const char* ssid = "****";
const char* wpa2key = "****";

IPAddress ip(192,168,2,11);
IPAddress gateway(192,168,2,1);
IPAddress subnet(255,255,255,0);
ESP8266WebServer server(80);

boolean webAppActive    = true;
String password         = "matthes";

int brightness_red      = 0;
int brightness_green    = 0;
int brightness_blue     = 0;

int presets[4][3]       = {{255,119,46},{46,0,39},{0,115,23},{0,0,0}};

boolean blinking        = false;
boolean fading          = false;
boolean jumping         = false;

int blinkDelay[]        = {50, 145, 240, 335, 430, 525, 620, 715, 810, 905};
int speedPosition       = 4;

int fadeOrder[]       = {RED_LED, GREEN_LED, BLUE_LED};
int lastColor          = 0;    //0~2, refers to fadeOrder[]
int thisColor          = 1;    //0~2, refers to fadeOrder[]
int fadeSpeed[]       = {1, 1, 1, 2, 3, 4, 5, 5, 7, 9};

boolean blinkHelper = false;

void handleRequest();
void handleGet();
void handleSettings();
void setupOTA();
void setupHttpServer();
void connectToWiFi();
void turnOff();
void turnOn();
void applyColor();
void applyPreset();
void blink();
void fade();
void jump();

void setup() {
    Serial.begin(115200);
    delay(10);

    analogWriteRange(255);
    
    pinMode(BUILTIN_LED, OUTPUT);
    digitalWrite(BUILTIN_LED, LOW);

    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);

    analogWrite(RED_LED, 0);
    analogWrite(GREEN_LED, 0);
    analogWrite(BLUE_LED, 0);

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.hostname("LED Controller");
    WiFi.config(ip, gateway, subnet);
    WiFi.begin(ssid, wpa2key);

    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println(WiFi.localIP());

    setupOTA();

    setupHttpServer();

    Serial.println("Setup done");
    pinMode(BUILTIN_LED, OUTPUT);
    digitalWrite(BUILTIN_LED, HIGH);    
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        connectToWiFi();
    }
    ArduinoOTA.handle();
    server.handleClient();

    if (blinking) {
        blink();
    }

    if (fading) {
        fade();
    }

    if (jumping) {
        jump();
    }

    if (blinking || fading || jumping) {
        delay(blinkDelay[speedPosition]);
    } else {
        delay(50);
    }
}

void handleRequest() {
    if (!server.hasArg("mode")) {
        Serial.println("no mode specified");
        server.send(400, "text/plain", "no mode specified");
        return;
    }

    String request = server.arg("mode");

    if (request.equals("color")) {
        fading = false;
        jumping = false;
        applyColor();

    } else if (request.equals("preset")) {
        fading = false;
        jumping = false;
        applyPreset();

    } else if (request.equals("blink")) {
        if (blinking) {
            blinking = false;
        } else {
            turnOff();
            fading = false;
            jumping = false;
            blinking = true;
        }
        server.send(200, "text/plain", "applied blinking");
    } else if (request.equals("fade")) {
        if (fading) {
            fading = false;
        } else {
            turnOff();
            thisColor = 1;
            lastColor = 0;
            blinking = false;
            jumping = false;
            fading = true;
        }
        server.send(200, "text/plain", "applied fading");
    } else if (request.equals("jump")) {
        if (jumping) {
            jumping = false;
        } else {
            turnOff();
            thisColor = 1;
            lastColor = 0;
            blinking = false;
            fading = false;
            jumping = true;
        }
        server.send(200, "text/plain", "applied jumping");
    } else if (request.equals("speed")) {
        if (!server.hasArg("newSpeed")) {
            server.send(400, "text/plain", "no speed specified");
            return;
        }
        int speed = server.arg("newSpeed").toInt();
        speedPosition = 9 - speed;
        server.send(200, "text/plain", "applied speed");
    } else {
        server.send(400, "text/plain", "argument not found");
    }
}

void handleGet() {
    if (!server.hasArg("arg")) {
        Serial.println("no argument specified");
        server.send(400, "text/plain", "no argument specified");
        return;
    }
    String request = server.arg("arg");
    String answer = "";

    if (request.equals("current")) {
        answer += (String(brightness_red));
        answer += (";" + String(brightness_green)); 
        answer += (";" + String(brightness_blue)); 
        answer += (";" + String((10-speedPosition)));

    } else if (request.equals("presets")) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) {
                answer += String(presets[i][j]);
                answer += ";";
            }
        }
    } else {
        server.send(400, "text/plain", "argument not found");
    }
    server.send(200, "text/plain", answer);
}

void handleSettings() {
    if ((!server.hasArg("request")) || (!server.hasArg("password"))) {
        Serial.println("Request or password missing");
        server.send(401, "text/plain", "Request or password missing");
        return;
    }

    if (!server.arg("password").equals(password)) {
        Serial.println("Password wrong");
        server.send(401, "text/plain", "wrong password");
        return;
    }

    String request = server.arg("request");

    if (request.equals("lock")) {
        webAppActive = false;
        server.send(200, "text/plain", "locked");
    } else if (request.equals("unlock")) {
        webAppActive = true;
        server.send(200, "text/plain", "unlocked");
    }
}

void setupHttpServer() {
    server.on("/request", handleRequest);
    server.on("/get", handleGet);
    server.on("/settings", handleSettings);

    server.on("/", HTTP_GET, []() {
        if (webAppActive) {
            server.send_P(200, "text/html", GUI);   
        } else {
            server.send(400, "text/plain", "Zugriff blockiert");
        }
    });

    server.on("/admin", HTTP_GET, []() {
        server.send_P(200, "text/html", ADMIN);
    });

    server.on("/iro.min.js", HTTP_GET, []() {
        server.send_P(200, "application/javascript", IRO_JS);
    });

    server.begin();
}

void setupOTA() {
    ArduinoOTA.setHostname("ESP8266");
    ArduinoOTA.setPassword("password");

    ArduinoOTA.onStart([]() {
        Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
}

void connectToWiFi() {
  Serial.println("WiFi connection lost, reconnecting");
  digitalWrite(BUILTIN_LED, LOW);

  WiFi.begin(ssid, wpa2key);
  WiFi.config(ip, gateway, subnet);

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Wifi reconnected");
  digitalWrite(BUILTIN_LED, HIGH);
}

void turnOff() {
    analogWrite(RED_LED, 0);
    analogWrite(GREEN_LED, 0);
    analogWrite(BLUE_LED, 0);
}

void turnOn() {
    analogWrite(RED_LED, brightness_red);
    analogWrite(GREEN_LED, brightness_green);
    analogWrite(BLUE_LED, brightness_blue);
}

void applyColor() {
    Serial.println("applying new color ");
    brightness_red = 0;
    if (server.hasArg("red")) {
        brightness_red = server.arg("red").toInt();
    }

    brightness_green = 0;
    if (server.hasArg("green")) {
        brightness_green = server.arg("green").toInt();
    }

    brightness_blue = 0;
    if (server.hasArg("blue")) {
        brightness_blue = server.arg("blue").toInt();
    }

    turnOn();
    server.send(200, "text/plain", "applied new color");
}

void applyPreset() {
    if (!server.hasArg("index")) {
        Serial.println("no index specified");
        server.send(400, "text/plain", "no index specified");
        return;
    }

    int index = server.arg("index").toInt();

    if (server.hasArg("save")) {
        Serial.println("saving preset");
        presets[index][0] = brightness_red;
        presets[index][1] = brightness_green;
        presets[index][2] = brightness_blue;
        server.send(200, "text/plain", "preset saved");
    } else {
        Serial.println("applying preset");
        brightness_red = presets[index][0];
        brightness_green = presets[index][1];
        brightness_blue = presets[index][2];

        turnOn();
        server.send(200, "text/plain", "preset applied");
    }
}

void blink() {
    if (!blinkHelper) {
        turnOff();
        blinkHelper = true;
    } else {
        turnOn();
        blinkHelper = false;
    }
}

void fade() {
    int darker = fadeOrder[lastColor];
    int brighter = fadeOrder[thisColor];

    for (int i = 0; i <= 255; i++) {
        analogWrite(darker, 255 - i);
        analogWrite(brighter, i);
        delay(fadeSpeed[speedPosition]);
        server.handleClient();
        if (!fading) {
            break;
        }
    }
    lastColor = thisColor;

    if (thisColor == 2) {
        thisColor = 0;
    } else {
        thisColor++;
    }
}

void jump() {
    int off = fadeOrder[lastColor];
    int on = fadeOrder[thisColor];

    analogWrite(off, 0);
    analogWrite(on, 255);

    lastColor = thisColor;

    if (thisColor == 2) {
        thisColor = 0;
    } else {
        thisColor++;
    }
}