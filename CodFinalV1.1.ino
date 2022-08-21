#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>

const char* ssid = "HUAWEI-2.4G-PJBP";
const char* password = "N75n7N5g";
const char* mqtt_server = "";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
DHTesp dht22;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("MuestradeValores");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(D4, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);  
  dht22.setup(D1, DHTesp::DHT22);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  int i = 0;
  if (now - lastMsg > 10000) {
    lastMsg = now;
    TempAndHumidity temp = dht22.getTempAndHumidity();
    String mensaje = "Temperatura: " + String(temp.temperature, 1) + "°C";
    int humedad = analogRead(A0);
    humedad = constrain(humedad, 420, 1023);
    humedad = map(humedad, 420, 1023, 100, 0);
    String mensaje2= "Humedad: " + String(humedad) +  "%";
    if(temp.temperature>20){
      digitalWrite(D2, HIGH);
    }else{
      digitalWrite(D3, HIGH);
      if(humedad < 20)
      {
        digitalWrite(D4, HIGH);
        Serial.println("------------------------");
        Serial.println("Bomba de agua encendida");
        Serial.println("------------------------");
      }
    }
    i++;
    Serial.print("Valor de dato ");
    Serial.println(i);
    Serial.println(mensaje);
    Serial.println(mensaje2);
    Serial.println("------------------------");
  }
}
