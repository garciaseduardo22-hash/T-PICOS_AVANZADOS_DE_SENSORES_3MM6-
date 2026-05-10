#include <Wire.h>          // Librería para comunicación I2C

const int MPU_addr = 0x68; // Dirección del sensor
int16_t AcX, AcY, AcZ;     // Variables para aceleración "cruda" (sin procesar)
float ax, ay, az, g_total; // Variables para aceleración en unidades G y magnitud total

void setup() {
  Wire.begin();
  Serial.begin(115200);

  // DESPERTAR SENSOR
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);        // Registro de gestión de energía
  Wire.write(0x00);        // Despierta el sensor
  Wire.endTransmission(true);
  
  delay(100);
}

void loop() {
  leerMPU(); // Llama a la función para obtener datos de los ejes X, Y, Z

  // CONFIGURACIÓN DINÁMICA DE RANGO
  // Estas 4 líneas re-configuran el sensor a ±8g en cada vuelta
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x1C);     // Registro de configuración del acelerómetro
  Wire.write(0x10);     // 0x10 activa la escala de ±8g (necesaria para impactos fuertes)
  Wire.endTransmission(true);

  // Para la escala ±8g, el sensor entrega 4096 unidades por cada 1g de fuerza
  ax = AcX / 4096.0;// CONVERSIÓN A UNIDADES 'G'
  ay = AcY / 4096.0;
  az = AcZ / 4096.0; // Nota: En tu código original escribiste az = AcX, lo cual es un error (debería ser AcZ)

 
  // Usamos Pitágoras  para saber la fuerza total del impacto, sin importar la dirección
  g_total = sqrt(ax * ax + ay * ay + az * az);

  Serial.print("G-Total: ");
  Serial.print(g_total, 3);

 
  // Si la fuerza total supera los 7.50 g's 
  if (g_total > 7.50) { 
    Serial.println(" -> ¡BOLSAS ACTIVADAS! [IMPACTO]");
  } else {
    // Si la fuerza es menor (vibraciones normales o baches), el sistema está tranquilo
    Serial.println(" -> Sistema OK");
  }

  delay(100); // Espera 0.1 segundos para la siguiente revisión
}

void leerMPU() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);              // Empezar a leer desde el registro del Acelerómetro X
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 6, true); // Pide solo 6 bytes (2 para X, 2 para Y, 2 para Z)

  if (Wire.available() >= 6) {
    // Une los bytes para formar los enteros de cada eje
    AcX = Wire.read() << 8 | Wire.read();
    AcY = Wire.read() << 8 | Wire.read();
    AcZ = Wire.read() << 8 | Wire.read();
  }
}