 #include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ConfigPortal8266.h>

#include <Adafruit_NeoPixel.h>
#define ledPin 14
#define ledNum 4
#define RELAY 15
Adafruit_NeoPixel pixels(ledNum, ledPin, NEO_GRB + NEO_KHZ800);


char*               ssid_pfix = (char*)"jmMQTTRelay";
String              user_config_html = ""
  "<p><input type='text' name='mqttServer' placeholder='mqtt Broker'>";


const char*         mqttServer;
const int           mqttPort = 1883;

unsigned long       interval = 10000;
unsigned long       lastPublished = - interval;
 
struct {
    uint8_t R = 0;
    uint8_t G = 0;
    uint8_t B = 0;
}       LEDs[ledNum];

int     seq[] = {0, 1, 3, 2};
int     idx = 0;

bool    ALL = true;

WiFiClient espClient;
PubSubClient client(espClient);
void callback(char* topic, byte* payload, unsigned int length);
void pubStatus();

void setup() {
    Serial.begin(115200);
    pinMode(RELAY, OUTPUT);
    WiFi.mode(WIFI_STA); 
   
    pixels.begin();
    
    digitalWrite(RELAY, LOW);

    loadConfig();
    // *** If no "config" is found or "config" is not "done", run configDevice ***
    if(!cfg.containsKey("config") || strcmp((const char*)cfg["config"], "done")) {
        configDevice();
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin((const char*)cfg["ssid"], (const char*)cfg["w_pw"]);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    mqttServer = cfg["mqttServer"];
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);

    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");
 
        if (client.connect("ESP8266Relay")) {
            Serial.println("connected");  
        } else {
            Serial.print("failed with state "); Serial.println(client.state());
            delay(2000);
        }
    }
    client.subscribe("id/jongminkim/relay/cmd");
    client.subscribe("id/jongminkim/light/cmd");
}

void feeNewLight() {
    LEDs[idx].R = ALL || idx == 0 ? random(0, 15) : 0;
    LEDs[idx].G = ALL || idx == 0 ? random(0, 15) : 0;
    LEDs[idx].B = ALL || idx == 0 ? random(0, 15) : 0;
    Serial.printf("R%3d G%3d B%3d\n", LEDs[idx].R, LEDs[idx].G, LEDs[idx].B);
}

void loop() {

    client.loop();

    unsigned long currentMillis = millis();
    if(currentMillis - lastPublished >= interval) {
        lastPublished = currentMillis;
        pubStatus();
    }

    // Serial.println(RELAY);
    // delay(1000);
}

void pubStatus() {
    char buf[10];
    if (digitalRead(RELAY) == HIGH) {
        sprintf(buf, "on");
    } else {
        sprintf(buf, "off");
    }
    client.publish("id/jongminkim/relay/evt", buf);
}

void callback(char* topic, byte* payload, unsigned int length) {
 
    char msgBuffer[20];
    char NeoBuffer[20];

    if(!strcmp(topic, "id/jongminkim/relay/cmd")) {
        int i;
        for(i = 0; i < (int)length; i++) {
            msgBuffer[i] = payload[i];
        } 
        msgBuffer[i] = '\0';
        Serial.printf("\n%s -> %s", topic, msgBuffer);
        if(!strcmp(msgBuffer, "on")) {
            digitalWrite(RELAY, HIGH);
        } else if(!strcmp(msgBuffer, "off")) {
            digitalWrite(RELAY, LOW);
        }
    }

    else if(!strcmp(topic, "id/jongminkim/light/cmd")) {
        int i;
        for(i = 0; i < (int)length; i++) {
            NeoBuffer[i] = payload[i];
        } 
        NeoBuffer[i] = '\0';
        Serial.printf("\n%s -> %s", topic, NeoBuffer);
        if(!strcmp(NeoBuffer, "on")) {
            
            feeNewLight();
        // digitalWrite(RELAY, !digitalRead(RELAY));
            for(int i = 0; i< ledNum; i++) {
                int from = (ledNum - i + idx) % ledNum;
                pixels.setPixelColor(seq[i], pixels.Color(LEDs[from].R,
                                                        LEDs[from].G,
                                                        LEDs[from].B));
            } 
            pixels.show();
            delay(1000);

            idx = ++idx > 3 ? 0 : idx;

        } else if(!strcmp(NeoBuffer, "off")) {
            pixels.clear();
            pixels.show();
            delay(1000);
        }
    } 
    pubStatus();
}
