#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#define WLAN_SSID			        "<SSID>"
#define WLAN_PSK			        "<Kennwort>"

#define MESSAGE_BROKER_DOMAIN       "message-broker.intern.michelblank.de"
#define MESSAGE_BROKER_PORT         8883
#define MESSAGE_BROKER_USERNAME	    "<Benutzername>"
#define MESSAGE_BROKER_PASS         "<Kennwort>"
#define MESSAGE_BROKER_DEVICE_NAME  MESSAGE_BROKER_USERNAME
#define MESSAGE_BROKER_QOS          1

#define ERR_LED_PIN				    LED_BUILTIN	// blaue LED (16) (OUTPUT)
#define BUZZER_PIN			        14		    // D5 (OUTPUT)

WiFiClientSecure espClient;
PubSubClient client(MESSAGE_BROKER_DOMAIN, MESSAGE_BROKER_PORT, espClient);

void setupWLAN();
void reconnect();
void alarm();

void setup() {
    WiFi.mode(WIFI_STA); //no AP
    // Serial
    Serial.begin(115200);
    delay(10);

    pinMode(ERR_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    // set outputs low
    digitalWrite(ERR_LED_PIN, HIGH); //LED is ON, when LOW
    digitalWrite(BUZZER_PIN, LOW);

    // WLAN
    setupWLAN();
}

void setupWLAN () {
    digitalWrite(ERR_LED_PIN, LOW);
    Serial.print("Connecting to ");
    Serial.println(WLAN_SSID);
    WiFi.begin(WLAN_SSID, WLAN_PSK);
    while (WiFi.status() != WL_CONNECTED) {
        delay(200);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected. IP address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(ERR_LED_PIN, HIGH);

    espClient.setInsecure(); // skip certificate validation
}

void loop() {
    if (!client.connected()) {
        digitalWrite(ERR_LED_PIN, LOW);
        reconnect();
        if(digitalRead(ERR_LED_PIN) != 1) {
            digitalWrite(ERR_LED_PIN, HIGH);
        }
    }

    client.loop(); //read messages
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    //Convert Data
    if(length == 1 && (char)payload[0] == '1') {
        alarm();
    }
}

void reconnect() {
    // Loop until we're reconnected
    while (!client.connected() && WiFi.status() == WL_CONNECTED) {
        if (client.connect(MESSAGE_BROKER_DEVICE_NAME, MESSAGE_BROKER_USERNAME, MESSAGE_BROKER_PASS, "/client/" MESSAGE_BROKER_DEVICE_NAME "/state", MESSAGE_BROKER_QOS, 1, "0")) { //qos, retain, message
            client.setCallback(mqttCallback);
            client.subscribe("/feuerwehr/alarm", MESSAGE_BROKER_QOS);

            client.publish("/client/" MESSAGE_BROKER_DEVICE_NAME "/state", "1", true);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
    if(WiFi.status() != WL_CONNECTED){
        setupWLAN();
    }
}

void alarm() {
    for (int i = 0; i < 5; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(400);
        digitalWrite(BUZZER_PIN, LOW);
        delay(200);
    }

    digitalWrite(BUZZER_PIN, LOW);
}