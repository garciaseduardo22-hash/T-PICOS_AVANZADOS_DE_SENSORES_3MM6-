#include <Wire.h>
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;

void setup() {
  Serial.begin(9600);//velocidad de monitor serial 
  Serial.println("=== Test BMP180 ===");

  if (!bmp.begin()) {
    Serial.println("ERROR: No se encontro el BMP180!");
    Serial.println("Revisa el cableado (SDA, SCL, VCC, GND)");
    while (1); // Se queda aqui si no encuentra el sensor
  }

  Serial.println("BMP180 detectado OK!");
}

void loop() {
  float temperatura = bmp.readTemperature();// temperatura en °C
  long  presion     = bmp.readPressure();// presión en Pa
  float presionHPa  = presion / 100.0;//presion en  hPa / mbar
  float altitud     = bmp.readAltitude(101325);// metros (presion al nivel del mar estandar)
//RESULTADOS
  Serial.println("------------------------------");
  Serial.print("Temperatura:  ");
  Serial.print(temperatura, 1);
  Serial.println(" °C");

  Serial.print("Presion:      ");
  Serial.print(presionHPa, 1);
  Serial.println(" hPa");

  Serial.print("Altitud aprox: ");
  Serial.print(altitud, 1);
  Serial.println(" m");

  delay(2000);
}