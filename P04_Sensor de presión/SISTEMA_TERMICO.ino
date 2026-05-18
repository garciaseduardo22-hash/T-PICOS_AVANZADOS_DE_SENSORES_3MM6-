#include <Wire.h>// Librería de Arduino para comunicación I2C.
#include <Adafruit_BMP085.h>// Libreria del sensor.

Adafruit_BMP085 bmp; // Se crea un objeto llamado "bmp" para usar la librería más fácil.

void setup() {
  Serial.begin(9600); // Inicia la comunicación serial a 9600 baudios.
  
  if (!bmp.begin()) {
    Serial.println("ERROR: No se encontro el BMP180!");//advertencia si no encuentra el sensor
    while (1); 
  }

  Serial.println("Tiempo_ms,T_BMP180,T_LM35");// encabezado
}

void loop() {
  //tiempo actual del experimento en milisegundos
  unsigned long tiempo = millis();

  //Lectura del BMP180
  float T_bmp = bmp.readTemperature(); // Devuelve la temperatura en °C.

  //Lectura y conversión del LM35 (A0)
  int lectura = analogRead(A0);// Lectura cruda del ADC (0-1023).
  float voltaje = lectura * (5.0 / 1023.0);// Conversión a voltaje (asume 5V).
  float T_lm35 = voltaje * 100.0;// El LM35 da 10mV/°C → T = V * 100.

  //Impresión de los valores 
  Serial.print(tiempo);
  Serial.print(",");
  Serial.print(T_bmp, 2);  
  Serial.print(",");
  Serial.println(T_lm35, 2);

  delay(500); // Pausa de 0.5 segundos para un muestreo óptimo en control de sistemas
}