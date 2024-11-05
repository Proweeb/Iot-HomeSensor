const os = require("os");

function getHostIp() {
  return os.networkInterfaces()["WLAN"][0]["address"].toString();
}

const mqtt = require("mqtt");
const { InfluxDB, Point } = require("@influxdata/influxdb-client");

// MQTT setup
const mqttBrokerUrl = process.env.MQTT_BROKER_URL || `mqtt://${getHostIp()}`;
const influxToken = process.env.INFLUX_TOKEN;
const influxUrl = process.env.INFLUX_URL || "http://localhost:8086";

const influxClient = new InfluxDB({ url: influxUrl, token: influxToken });

const org = process.env.INFLUX_ORG;
const bucket = process.env.INFLUX_BUCKET;
const writeClient = influxClient.getWriteApi(org, bucket, "ms"); // Set precision to milliseconds

const mqttClient = mqtt.connect(mqttBrokerUrl);

// Subscribe to the MQTT topics
mqttClient.on("connect", () => {
  console.log("Connected to MQTT Broker!");
  mqttClient.subscribe("home/temperature", (err) => {
    if (!err) {
      console.log("Subscribed to home/temperature");
    }
  });
  mqttClient.subscribe("home/humidity", (err) => {
    if (!err) {
      console.log("Subscribed to home/humidity");
    }
  });
  mqttClient.subscribe("home/light", (err) => {
    if (!err) {
      console.log("Subscribed to home/light");
    }
  });
});

// Handle incoming MQTT messages and write to InfluxDB
mqttClient.on("message", (topic, message) => {
  const valueString = message.toString(); // Convert message to string
  const location = "home"; // Location tag
  let point;

  // Check the topic and create the corresponding point
  if (topic === "home/temperature") {
    const value = parseFloat(valueString);
    point = new Point("temperature")
      .tag("location", location)
      .floatField("value", value);
    console.log(`Temperature: ${value}C`);
  } else if (topic === "home/humidity") {
    const value = parseFloat(valueString);
    point = new Point("humidity")
      .tag("location", location)
      .floatField("value", value);
    console.log(`Humidity: ${value}%`);
  } else if (topic === "home/light") {
    const value = parseInt(valueString, 10); // Convert message to an integer
    point = new Point("light")
      .tag("location", location)
      .intField("value", value);
    console.log(`Light: ${value}`);
  }

  // Write the point to InfluxDB if it's defined
  if (point) {
    // Adding a timestamp in milliseconds precision
    point.timestamp(Date.now());

    writeClient.writePoint(point);
    writeClient
      .flush()
      .then(() => {
        console.log(`Point written to InfluxDB: ${topic}`);
      })
      .catch((error) => {
        console.error(`Error writing point: ${error}`);
      });
  }
});
