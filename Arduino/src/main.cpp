#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// WiFi MQTT server credentials
const char* ssid = "YourWifi";  // Adjust WIFI name/ssid
const char* password = "password"; // Adjust WIFI Password
const char* mqtt_server = "192.168.0.51";  // Adjust Your MQTT broker address,in this case the IP of the Computer running the backend Services

// Names of the Topic being published to
const char* temperture_topic="home/temperature";
const char* humidity_topic="home/humidity";
const char* light_topic="home/light";

// DHT Sensor Setup
#define DHTPIN 10        // Adjust Pin where the DHT11 is connected
#define DHTTYPE DHT11    // Adjust to Type of Sensor , in my case DHT 11
DHT dht(DHTPIN, DHTTYPE);

// LDR Light Sensor
const int gpioPin = GPIO_NUM_1;  //Adjust to the PIN where the Sensor is connected


WiFiClient espClient;
PubSubClient client(espClient);

// Function to connect to WiFi
void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
}

// Callback function for received messages
void callback(char* topic, byte* message, unsigned int length) {
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++) {
        messageTemp += (char)message[i];
    }
    Serial.println(messageTemp);
}

// Reconnect to MQTT broker
void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Connect without username and password
        if (client.connect("ESP32Client")) {
            Serial.println("connected");
            client.subscribe("test/topic"); // Optional: Subscribe to a topic if needed
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    setup_wifi();
    client.setServer(mqtt_server, 1883); // Connect to the Mosquitto broker on your PC
    client.setCallback(callback);

    dht.begin(); // Initialize the DHT sensor
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    // Publish data periodically
    static unsigned long lastMsg = 0;
    unsigned long now = millis();
    if (now - lastMsg > 2000) { // Adjust the delay as needed
        lastMsg = now;

        // Read temperature and humidity
        float temp = dht.readTemperature();
        float humidity = dht.readHumidity();
        int lightSensorValue = analogReadRaw(gpioPin); // Adjust Pin according to your own LDR

        // Check if any reads failed and exit early (to try again).
        if (isnan(temp) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            return;
        }

        // Publish temperature
        String tempMsg = String(temp);
        client.publish(temperture_topic, tempMsg.c_str()); //Adjust the home part of string , with the actual location of your Chip , examples=offfice/temperature
        Serial.print("Temperature published: ");
        Serial.println(tempMsg);

        // Publish humidity
        String humidityMsg = String(humidity);
        client.publish(humidity_topic, humidityMsg.c_str()); //Adjust the home part of string , with the actual location of your Chip ,
        Serial.print("Humidity published: ");
        Serial.println(humidityMsg);

        // Publish light sensor value
        String lightMsg = String(lightSensorValue);
        client.publish(light_topic, lightMsg.c_str()); //Adjust the home part of string , with the actual location of your Chip ,
        Serial.print("Light sensor value published: ");
        Serial.println(lightMsg);
        delay(5000);
    }
}
