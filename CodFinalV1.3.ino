#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>

const char* ssid = "HUAWEI-2.4G-PJBP";
const char* password = "N75n7N5g";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
DHTesp dht22;
unsigned long lastMsg = 0;

//FUNCION DE CONEXION WIFI
void setup_wifi() {
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  randomSeed(micros());
}


//CONEXION DEL MOSQUITTO (SUBSCRIPCION A UN TOPICO)
void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      client.subscribe("MuestradeSensores");
    } else {
      delay(5000);
    }
  }
}

void setup() {
  dht22.setup(D0, DHTesp::DHT22);
  pinMode(D1, OUTPUT); //ULTRASONIDO TRIGGER
  pinMode(D2, INPUT);  //ULTRASONIDO ECHO
  pinMode(D3, OUTPUT); // LED NORMAL ROJO
  pinMode(D4, OUTPUT); //LED RGB ROJO 
  pinMode(D5, OUTPUT); //LED RGB VERDE 
  pinMode(D6, OUTPUT); //LED RGB AZUL 
  pinMode(D7, OUTPUT); //PARLANTE 
  pinMode(D8, OUTPUT); //BOMBA DE AGUA
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

//FUNCION DE INICIALIZACION DE LOS LEDS
int unica = 0;
void inicializarLED()
{
  digitalWrite(D3, LOW);
  digitalWrite(D4, HIGH);
  digitalWrite(D6, HIGH);
  digitalWrite(D5, HIGH);
  digitalWrite(D8, HIGH);
  delay(100);
}


//FUNCION PARA CALCULAR LA LUZ DEL POCENTAJE
void calcularLuz(int porcentaje)
{
  int verde = 255, rojo = 255 , azul = 255 ;
  
  if(porcentaje >= 50)
  {
      verde = 0 ;
      rojo = 2 * ((porcentaje - 50) * 2.55);
      azul = 255;
  }else if(porcentaje >= 30)
  {
      verde = 255 - ((50 - porcentaje) * 2.55)/2;
      rojo = 0;
      azul = 255;
  }else
  {
      verde = 255; 
      rojo = 255; 
      azul = 255;
  }
  analogWrite(D4, rojo);
  analogWrite(D5, verde);
  analogWrite(D6, azul);
}

void loop() {
  if(unica == 0)
  {
    inicializarLED();
    unica++;
  }
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    
    //SENSOR DE TEMPERATURA
    TempAndHumidity temp = dht22.getTempAndHumidity();
    
    //SENSOR DE HUMEDAD
    int humedad = analogRead(A0);
    humedad = constrain(humedad, 420, 1023);
    humedad = map(humedad, 420, 1023, 100, 0);

    //SENSOR ULTRASONICO
    digitalWrite(D1, HIGH);
    delay(1);
    digitalWrite(D1, LOW);
    float duracion = pulseIn(D2,HIGH);
    float distancia = duracion / 58.3;
    
    //CONVERSION A PORCENTAJE DE ALMACEN
    //24 cm es el maximo es decir el 100%, entonces
    int AguaPorcentaje;
    if(distancia < 24 && distancia > 7)
    {
      AguaPorcentaje = 100 - ( (distancia - 7 ) * 100 ) / 17 ;  
    }else if(distancia < 7 )
    {
      AguaPorcentaje = 100;
    }else
    {
      AguaPorcentaje = 0;
    }

    //LED DE AVISO Y BUZZER
    if(AguaPorcentaje < 30)
    {
      digitalWrite(D3, HIGH);
      tone(D7, 400, 3000);
    }else
    {
      digitalWrite(D3, LOW);
    }
    
    //FUNCIONALIDAD DE LA BOMBA DE AGUA
    if(AguaPorcentaje > 30){
      if(humedad < 60)
      {
        if(temp.temperature < 25)
        {
          digitalWrite(D8, LOW);
          delay(2500);
          digitalWrite(D8, HIGH);
        }else
        {
          digitalWrite(D8, LOW);
          delay(1500);
          digitalWrite(D8, HIGH);
        }
      }else
      {
        digitalWrite(D8, HIGH);
      }
      
    }else{
      digitalWrite(D8, HIGH);
    }
  
    //PUBLICADO DE DATOS A NODE RED (MOSQUITTO)
    String sensorT = String(temp.temperature, 1);
    String sensorH = String(humedad);
    String sensorA = String(AguaPorcentaje);
    
    client.publish("proyectoIOT/sensorTemp", sensorT.c_str());
    client.publish("proyectoIOT/sensorHum", sensorH.c_str());
    client.publish("proyectoIOT/Tanque", sensorA.c_str());


    //RECOLECCION DE DATOS
    
    Serial.print(sensorT);
    Serial.print(",");
    Serial.print(sensorH);
    Serial.print(",");
    Serial.print(sensorA);
    Serial.println("");

    //FUNCION DE LOS LEDS
    calcularLuz(AguaPorcentaje);

    
  }
}
