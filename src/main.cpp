#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <string.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

//#include <noWheel.h>
#include <colorWheel.h>
#include <web_iro_js.h>
#include <admin.h>

#include <css.h>
#include <javascript.h>

#define RED_LED 5
#define GREEN_LED 4
#define BLUE_LED 0

#define BUILTIN_LED 2

const char* ssid = "wifi";
const char* wpa2key = "key";

enum ResultCode{
	LC_APPLIED_MODE,
	LC_APPLIED_COLOR,
	LC_APPLIED_SPEED,
	LC_FALSE_REQUEST,
	LC_FALSE_COLOR,
	LC_FALSE_MODE,
    LC_TOGGLE
};

IPAddress ip(192,168,2,111);
IPAddress gateway(192,168,2,1);
IPAddress subnet(255,255,255,0);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

boolean webAppActive    = true;
String password         = "matthes";

int brightness_red      = 0;
int brightness_green    = 0;
int brightness_blue     = 0;

boolean blinking        = false;
boolean fading          = false;
boolean jumping         = false;

uint32_t blinkDelay[]   = {50, 145, 240, 335, 430, 525, 620, 715, 810, 905};
int speedPosition       = 4;
uint32_t prevMillis     = 0;

int hue                 = 0;
uint32_t fadeSpeed[]    = {4, 6, 8, 10, 15, 20, 25, 30, 35, 50};

boolean blinkHelper = false;
boolean on          = false;

void handleRequest();
void handleGet();
void handleSettings();
void setupOTA();
void setupServer();
void connectToWiFi();
void toggle();
void turnOff();
void turnOn();
void blink();
void fade();
void jump();
void setHue(int hue);

enum ResultCode handleRequest(char* msg);
enum ResultCode handleColorRequest(char* msg);
enum ResultCode setColor(int red, int green, int blue);
enum ResultCode setMode(char*);
enum ResultCode setSpeed(int speed);

void sendResultToClient(AsyncWebSocketClient* client,enum ResultCode result) {
	String msg = "";
	switch (result) {
		case LC_APPLIED_MODE:
			msg = "Mode has been applied";
			break;
		case LC_APPLIED_COLOR:
			msg = "Color has been applied";
			break;
		case LC_APPLIED_SPEED:
			msg = "Speed has been applied";
			break;
		case LC_FALSE_REQUEST:
			msg = "Didn't recognize request";
			break;
		case LC_FALSE_COLOR:
			msg = "Couldn't decode color";
			break;
		case LC_FALSE_MODE:
			msg = "Didn't recognize mode";
			break;
        case LC_TOGGLE:
            msg = "Toggled";
            break;
	}

	Serial.println(msg);
	client->text(msg);
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  	if(type == WS_EVT_CONNECT){
		Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
		client->printf("{\"r\": %d, \"g\": %d, \"b\": %d, \"s\": %d}", brightness_red, brightness_green, brightness_blue, speedPosition);
		client->ping();
  	} else if(type == WS_EVT_DISCONNECT){
		Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  	} else if(type == WS_EVT_ERROR){
		Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  	} else if(type == WS_EVT_PONG){
		Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  	} else if(type == WS_EVT_DATA){
		AwsFrameInfo * info = (AwsFrameInfo*)arg;
		char msg[info->len+1];
		msg[info->len] = '\0';
		if(info->final && info->index == 0 && info->len == len){
			//the whole message is in a single frame and we got all of it's data
			Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

			if(info->opcode == WS_TEXT){
				for(size_t i=0; i < info->len; i++) {
					msg[i] = (char) data[i];
				}
			} else {
				//Message would be binary
				Serial.println("Message is binary.");
			}
			Serial.printf("%s\n",msg);
			

			if(info->opcode == WS_TEXT)
				client->text("Request received.");
			else
				client->binary("Request received. (Binary)");

		} else {
			//message is comprised of multiple frames or the frame is split into multiple packets
			if(info->index == 0){
				if(info->num == 0)
					Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
				Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
			}

			Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

			if(info->opcode == WS_TEXT){
				for(size_t i=0; i < len; i++) {
					msg[i] = (char) data[i];
				}
			} else {
				//Message is binary
				Serial.println("Message is binary");
			}
			Serial.printf("%s\n",msg);

			if((info->index + len) == info->len){
				Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
				if(info->final){
					Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
					if(info->message_opcode == WS_TEXT)
						client->text("Request received.");
					else
						client->binary("Request received. (Binary)");
				}
			}
		}
		enum ResultCode result = handleRequest(msg);
		sendResultToClient(client, result);
  	}
}

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

    setupServer();

    Serial.println("Setup done");
    prevMillis = millis();
    pinMode(BUILTIN_LED, OUTPUT);
    digitalWrite(BUILTIN_LED, HIGH);    
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        connectToWiFi();
    }
    ArduinoOTA.handle();
    ws.cleanupClients();

    if (blinking) {
        blink();
    }

    if (fading) {
        fade();
    }

    if (jumping) {
        jump();
    }
}

void handleRequest(AsyncWebServerRequest *http) {
    if (!http->hasParam("mode")) {
        Serial.println("no mode specified");
        http->send(400, "text/plain", "no mode specified");
        return;
    }

    String request = http->getParam("mode")->value();

    if (request.equals("toggle")) {
        toggle();

        http->send(200, "text/plain", "toggled LEDs");
    } else {
        http->send(400, "text/plain", "argument not found");
    }
}

void handleGet(AsyncWebServerRequest *http) {
    if (!http->hasParam("arg")) {
        Serial.println("no argument specified");
        http->send(400, "text/plain", "no argument specified");
        return;
    }
    String request = http->getParam("arg")->value();
    String answer = "";

    if (request.equals("current")) {
        answer += (String(brightness_red));
        answer += (";" + String(brightness_green)); 
        answer += (";" + String(brightness_blue)); 
        answer += (";" + String((10-speedPosition)));

    } else {
        http->send(400, "text/plain", "argument not found");
    }
    http->send(200, "text/plain", answer);
}

void handleSettings(AsyncWebServerRequest *http) {
    if ((!http->hasParam("request")) || (!http->hasParam("password"))) {
        Serial.println("Request or password missing");
        http->send(401, "text/plain", "Request or password missing");
        return;
    }

    if (!http->getParam("password")->value().equals(password)) {
        Serial.println("Password wrong");
        http->send(401, "text/plain", "wrong password");
        return;
    }

    String request = http->getParam("request")->value();

    if (request.equals("lock")) {
        webAppActive = false;
        http->send(200, "text/plain", "locked");
    } else if (request.equals("unlock")) {
        webAppActive = true;
        http->send(200, "text/plain", "unlocked");
    }
}

void setupServer() {
    server.on("/request", HTTP_ANY, [](AsyncWebServerRequest *request){
        handleRequest(request);
    });
    server.on("/get", HTTP_ANY, [](AsyncWebServerRequest *request) {
        handleGet(request);
    });
    server.on("/settings", HTTP_ANY, [](AsyncWebServerRequest *request) {
        handleSettings(request);
    });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (webAppActive) {
            request->send_P(200, "text/html", GUI);   
        } else {
            request->send(400, "text/plain", "Zugriff blockiert");
        }
    });

    server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", ADMIN);
    });

    server.on("/iro.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "application/javascript", IRO_JS);
    });

    server.on("/colorpicker.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "application/javascript", JS);
    });

    server.on("/colorpicker.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/css", CSS);
    });

    ws.onEvent(onWsEvent);
 	server.addHandler(&ws);

	ws.onEvent(onWsEvent);
  	server.addHandler(&ws);

  	events.onConnect([](AsyncEventSourceClient *client){
		client->send("hello!",NULL,millis(),1000);
  	});
  	server.addHandler(&events);

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

void toggle() {
    if (on) {
        turnOff();
    } else {
        turnOn();
    }
    on = !on;
}

void turnOff() {
    jumping = false;
    blinking = false;
	fading = false;

    analogWrite(RED_LED, 0);
    analogWrite(GREEN_LED, 0);
    analogWrite(BLUE_LED, 0);
}

void turnOn() {
    analogWrite(RED_LED, brightness_red);
    analogWrite(GREEN_LED, brightness_green);
    analogWrite(BLUE_LED, brightness_blue);
}

void blink() {
    if (millis() > prevMillis + blinkDelay[speedPosition]) {
        if (!blinkHelper) {
            analogWrite(RED_LED, 0);
            analogWrite(GREEN_LED, 0);
            analogWrite(BLUE_LED, 0);
            blinkHelper = true;
        } else {
            turnOn();
            blinkHelper = false;
        }
        prevMillis = millis();
    }
}

void fade() {
    if (millis() > prevMillis + fadeSpeed[speedPosition]) {
        hue += 5;
        setHue(hue);
        prevMillis = millis();

        if (hue >= 360){
            hue %= 360;
        }
    }
}

void jump() {
    if (millis() > prevMillis + blinkDelay[speedPosition]) {
        hue += 60;
        setHue(hue);
        prevMillis = millis();

        if (hue >= 360){
            hue %= 360;
        }
    }
}

// Set the RGB LED to a given hue (color) (0° = Red, 120° = Green, 240° = Blue)
void setHue(int hue) { 
  hue %= 360;                   // hue is an angle between 0 and 359°
  float radH = hue*3.142/180;   // Convert degrees to radians
  float rf = 0, gf = 0, bf = 0;
  
  if(hue>=0 && hue<120){        // Convert from HSI color space to RGB              
    rf = cos(radH*3/4);
    gf = sin(radH*3/4);
    bf = 0;
  } else if(hue>=120 && hue<240){
    radH -= 2.09439;
    gf = cos(radH*3/4);
    bf = sin(radH*3/4);
    rf = 0;
  } else if(hue>=240 && hue<360){
    radH -= 4.188787;
    bf = cos(radH*3/4);
    rf = sin(radH*3/4);
    gf = 0;
  }
  int r = rf*rf*1023;
  int g = gf*gf*1023;
  int b = bf*bf*1023;
  
  analogWrite(RED_LED,   r);    // Write the right color to the LED output pins
  analogWrite(GREEN_LED, g);
  analogWrite(BLUE_LED,  b);
}

enum ResultCode handleRequest(char* msg) {
	char* request = strtok(msg, "-");
	char* rest = strtok(0, "");
	
	if (strcmp(request, "color") == 0) {
        fading = false;
        jumping = false;
        blinking = false;
		return handleColorRequest(rest);

	} else if (strcmp(request, "mode") == 0) {
		return setMode(rest);

	} else if (strcmp(request, "speed") == 0) {
		return setSpeed(atoi(rest));

	} else if (strcmp(request, "toggle") == 0) {
        toggle();
        return LC_TOGGLE;
    } else {
		return LC_FALSE_REQUEST;
	}
}

enum ResultCode handleColorRequest(char* msg) {

	char* color = strtok(msg, "-");
	int position = 0;
	int red = 0, green = 0, blue = 0;

	while (color != NULL) {

		if (position == 0) {
			red = atoi(color);
			Serial.printf("Red: %s\n", color);
		} else if(position == 1) {
			green = atoi(color);
			Serial.printf("Green: %s\n", color);
		} else if (position == 2) {
			blue = atoi(color);
			Serial.printf("Blue: %s\n", color);
		} else {
			return LC_FALSE_COLOR;
		}
		position++;
		color = strtok(0, "-");
	}

	if (red > 255) {red = 255;}
	if (red < 0) {red = 0;}
	if (green > 255) {green = 255;}
	if (green < 0) {green = 0;}
	if (blue > 255) {blue = 255;}
	if (green < 0) {green = 0;}

	return setColor(red, green, blue);
}

enum ResultCode setColor(int red, int green, int blue) {
	analogWrite(RED_LED, red);
	analogWrite(GREEN_LED, green);
	analogWrite(BLUE_LED, blue);

    brightness_red = red;
    brightness_green = green;
    brightness_blue = blue;

    on = true;

	return LC_APPLIED_COLOR;
}

enum ResultCode setMode(char* mode) {
	if (strcmp(mode, "blink") == 0) {
        fading = false;
        jumping = false;
        if(blinking) {turnOn();}
		blinking = !blinking;

	} else if (strcmp(mode, "fade") == 0) {
        jumping = false;
        blinking = false;
        if(fading) {turnOn();}
		fading = !fading;

	} else if (strcmp(mode, "jump") == 0) {
        fading = false;
        blinking = false;
        if(jumping) {turnOn();}
		jumping = !jumping;

	} else {
		return LC_FALSE_MODE;
	}

    on = true;
	return LC_APPLIED_MODE;
	Serial.printf("Set mode to %s\n", mode);
}

enum ResultCode setSpeed(int speed) {
    if (speed > 9) {speed = 9;}

    speedPosition = 9 - speed;

	return LC_APPLIED_SPEED;
	Serial.printf("Set speed to %d\n", speed);
}