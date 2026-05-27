#include <Wire.h>
#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 aht;
const int pinLM35 = A0; // Pin del LM35

void setup() {
  Serial.begin(9600);// velocidad comunicación serial 
  Wire.begin();// inicia la comunicación serial 

  // Inicializar AHT20
  if (!aht.begin()) {
    Serial.println("¡No se pudo encontrar el sensor AHT20!");// alerta 
    while (1) delay(10);
  }
  Serial.println("Temp_AHT20(C),Humedad_AHT20(%),Temp_LM35(C)");// encabezados para mi excel 
}

void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp); // Lee el AHT20

  // Leer y calcular la temperatura del LM35
  int lecturaAnalogica = analogRead(pinLM35);
  // Conversión: (lectura * 5000mV / 1023) / 10mV por grado Celsius
  float tempLM35 = (lecturaAnalogica * 5.0 * 100.0) / 1024.0;

  // separa los datos en comas 
  Serial.print(temp.temperature);//Datos del sensor AHT20
  Serial.print(",");
  Serial.print(humidity.relative_humidity);// datos de humedad 
  Serial.print(",");
  Serial.println(tempLM35);// datos de temperatura del LM35

  delay(2000); // Muestreo cada 2 segundos
}