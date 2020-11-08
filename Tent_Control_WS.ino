#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <DHT.h>

#define VENT_RELAY_PIN D5
#define LIGHT_RELAY_PIN D4
// DHT sensor type & pin
#define DHTPIN D1
#define DHTTYPE DHT11

#define MAX_ADC_READING 1023
#define ADC_REF_VOLTAGE 5.0
#define REF_RESISTANCE 9870 // measured your 10k resistor with a meter and record the value
#define LUX_CALC_SCALAR 12518931
#define LUX_CALC_EXPONENT -1.405

DHT dht(DHTPIN, DHTTYPE);
unsigned long previousMillis = 0; // will store last time sensor readings were updated

// Updates DHT readings every 10 seconds
const long interval = 5000;

WebSocketsServer webSocket = WebSocketsServer(81);
const char *ssid = "Odolabi I";
const char *password = "2WIRE318";

float lux = 0;
float temp = 0;
float humidity = 0;

int lightRelay = 0;
int ventRelay = 0;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:

    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.println(ip + " connected");
  }
  break;
  case WStype_TEXT:
  {
    String text = String((char *)&payload[0]);
    Serial.println(text + " received");

    if (text == "info")
    {
    }
    else if (text == "lighton")
    {
      lightRelay = 1;
      digitalWrite(LIGHT_RELAY_PIN, LOW);
    }
    else if (text == "lightoff")
    {
      lightRelay = 0;
      digitalWrite(LIGHT_RELAY_PIN, HIGH);
    }
    else if (text == "venton")
    {
      ventRelay = 1;
      digitalWrite(VENT_RELAY_PIN, LOW);
    }
    else if (text == "ventoff")
    {
      ventRelay = 0;
      digitalWrite(VENT_RELAY_PIN, HIGH);
    }

    broadcastData();
  }
  break;
  }
}

void broadcastData()
{
  String data = "{\"temp\": ";
  data += temp;
  data += ", \"humid\": ";
  data += humidity;
  data += ", \"lux\": ";
  data += lux;
  data += ", \"light\": ";
  data += lightRelay;
  data += ", \"vent\": ";
  data += ventRelay;
  data += "}";

  webSocket.broadcastTXT((char *)data.c_str(), data.length());
}

void setup()
{
  Serial.begin(9600);

  Serial.println("DHT Starting");
  dht.begin();

  Serial.println("Wi-Fi Starting");
  WiFi.begin(ssid, password);

  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  pinMode(VENT_RELAY_PIN, OUTPUT);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
  }

  Serial.println(WiFi.localIP());
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  digitalWrite(LIGHT_RELAY_PIN, HIGH);
  digitalWrite(VENT_RELAY_PIN, HIGH);
}

void loop()
{
  unsigned long currentMillis = millis();

  if ((currentMillis - previousMillis) >= interval)
  {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;

    int ldrValue = analogRead(A0);

    // RESISTOR VOLTAGE_CONVERSION
    // Convert the raw digital data back to the voltage that was measured on the analog pin
    float resistorVoltage = (float)ldrValue / MAX_ADC_READING * ADC_REF_VOLTAGE;
    float ldrVoltage = ADC_REF_VOLTAGE - resistorVoltage;
    float ldrResistance = ldrVoltage / resistorVoltage * REF_RESISTANCE;

    lux = LUX_CALC_SCALAR * pow(ldrResistance, LUX_CALC_EXPONENT);

    Serial.print("LDR LUX: ");
    Serial.println(lux);

    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
    if (isnan(newT))
    {
      Serial.println("Failed to read from DHT sensor!");
    }
    else
    {
      temp = newT;
      Serial.print("Temp: ");
      Serial.println(temp);
    }

    // Read Humidity
    float newH = dht.readHumidity();
    if (isnan(newH))
    {
      Serial.println("Failed to read from DHT sensor!");
    }
    else
    {
      Serial.print("Humidity: ");
      humidity = newH;
      Serial.println(humidity);
    }

    broadcastData();
  }

  webSocket.loop();
}
