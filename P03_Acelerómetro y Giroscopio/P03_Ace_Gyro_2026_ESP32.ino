/*  Funciona para ESP32C3. Modificar tarjeta, pines y dirección del MPU
    Librería para la cual funciona es MPU by Electronic Cats*/
#include <Wire.h>
#include "MPU6050.h"

MPU6050 mpu(0x68); //Revisar la dirección de su módulo

int pitch=0;
int roll=0;

unsigned long past_time=0;

void setup()
{
  Serial.begin(115200);
  
  /*--Start I2C interface--*/
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin(); // Seleccionar los GPIO adecuados (ESP32)
  #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
    Fastwire::setup(400, true);
  #endif
  mpu.initialize();
  
}
 
void loop()
{   
  if(millis() - past_time >= 10)
  {
    // 1. Declare variables for raw data
    int16_t ax, ay, az;
    
    // 2. Read raw accelerometer data (Electronic Cats style)
    mpu.getAcceleration(&ax, &ay, &az);

    // 3. Convert raw values to "g" units
    // For the default range (+/- 2g), the scale factor is 16384
    float x = ax / 16384.0;
    float y = ay / 16384.0;
    float z = az / 16384.0;

    // 4. Calculate Pitch and Roll using the converted values
    pitch = -(atan2(x, sqrt(y*y + z*z)) * 180.0) / M_PI;
    roll = (atan2(y, z) * 180.0) / M_PI;

    // 5. Map and send data
    int mappedPitch = map(pitch, -180, 180, 0, 1023);
    int mappedRoll = map(roll, -180, 180, 0, 1023);
    
    trama(mappedPitch, mappedRoll);
    
    past_time = millis();
  }
}

void trama(int value1, int value2) {
  char buffer[12];
  // %04d means: integer, 4 digits wide, pad with zeros
  sprintf(buffer, "%04d,%04d", value1, value2);
  Serial.println(buffer);
}
