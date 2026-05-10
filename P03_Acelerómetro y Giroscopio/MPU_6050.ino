#include <Wire.h>   // Librería para comunicación I2C

const int MPU_addr = 0x68;// dirección i2c del sensor, 0x68 cuando esta conectado a GND

int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;//variables en crudo del sensor(no jalo la libreria)

float ax, ay, az;// variables del acelerometro en g's
float gx, gy, gz;//variables del giroscopio en g's
float roll_acc, pitch_acc;// angulos calculados solo con el acelerometro. 
float roll_gyro = 0.0;// angulos calculados con el giroscopio
float pitch_gyro = 0.0;

// Combinan acelerómetro y giroscopio
float roll_fil = 0.0;
float pitch_fil = 0.0;

// BIAS DEL GIROSCOPIO
float gx_bias = 0.0;
float gy_bias = 0.0;
float gz_bias = 0.0;

unsigned long lastTime = 0;// variable de tiempo, se usa para integrar el giroscopio. 

void setup() {
  
  Wire.begin();// Inicia comunicación I2C 
  Serial.begin(115200); // Inicia monitor serial

  
  // DESPERTAR EL SENSOR
  // El sensor inicia en modo sleep, así que hay que escribir
  // 0x00 en el registro PWR_MGMT_1 (0x6B)
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);     // Registro de gestión de energía
  Wire.write(0x00);     // Quitar modo sleep
  Wire.endTransmission(true);

  
  Wire.beginTransmission(MPU_addr);// CONFIGURAR ACELERÓMETRO
  Wire.write(0x1C);     // Registro ACCEL_CONFIG
  Wire.write(0x00);     // ±2g
  Wire.endTransmission(true);

  
  Wire.beginTransmission(MPU_addr);// CONFIGURAR GIROSCOPIO
  Wire.write(0x1B);     // Registro GYRO_CONFIG
  Wire.write(0x00);     // ±250 °/s
  Wire.endTransmission(true);

  delay(1000);

  
  Serial.println("Calibrando giroscopio, no mover el sensor...");// CALIBRACIÓN DEL GIROSCOPIO

  const int N = 1000;   // Número de muestras para calibración

  for (int i = 0; i < N; i++) {
    leerMPU();          // Leer datos crudos del sensor

    gx_bias += GyX;
    gy_bias += GyY;
    gz_bias += GyZ;

    delay(2);
  }

  // Promedio del bias en crudo
  gx_bias /= N;
  gy_bias /= N;
  gz_bias /= N;

  Serial.println("Calibracion lista.");
  Serial.println("Inicio de prueba con filtro complementario");

  // Guardar tiempo inicial
  lastTime = micros();
}

void loop() {
  
  leerMPU();// Leer datos crudos del sensor

  
  // dt es el tiempo transcurrido entre una iteración y otra y se usa para integrar el giroscopio
  unsigned long currentTime = micros();
  float dt = (currentTime - lastTime) / 1000000.0;  // en segundos
  lastTime = currentTime;

  
  // CONVERSIÓN DEL ACELERÓMETRO
  ax = AcX / 4096.0;
  ay = AcY / 4096.0;
  az = AcZ / 4096.0;

  // CONVERSIÓN DEL GIROSCOPIO
  gx = (GyX - gx_bias) / 131.0;
  gy = (GyY - gy_bias) / 131.0;
  gz = (GyZ - gz_bias) / 131.0;

  // PITCH Y ROLL CON ACELERÓMETRO Se calculan a partir del vector gravedad
  roll_acc  = atan2(ay, az) * 180.0 / PI;
  pitch_acc = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

  
  // PITCH Y ROLL CON GIROSCOPIO , ángulo = ángulo anterior + velocidad * dt
  roll_gyro  += gx * dt;
  pitch_gyro += gy * dt;

  
  // FILTRO COMPLEMENTARIO// 0.98 -> mayor peso al giroscopio // 0.02 -> corrección lenta con acelerómetro
  roll_fil  = 0.98 * (roll_fil + gx * dt) + 0.02 * roll_acc;
  pitch_fil = 0.98 * (pitch_fil + gy * dt) + 0.02 * pitch_acc;

  // MAGNITUD TOTAL DE GRAVEDAD
  float g_total = sqrt(ax * ax + ay * ay + az * az);

  
  // 1) Aceleración convertida en g
  Serial.print("A[g]: ");
  Serial.print(ax, 3); Serial.print(", ");
Serial.print(ay, 3); Serial.print(", ");
  Serial.print(az, 3);

  // 2) Magnitud del vector aceleración
  Serial.print(" | |g| = ");
Serial.print(g_total, 3);

  // 3) Ángulos calculados solo con acelerómetro
  Serial.print(" | AccRoll: ");
  Serial.print(roll_acc, 2);
  Serial.print(" AccPitch: ");
  Serial.println(pitch_acc, 2);


  // 4) Ángulos calculados solo con giroscopio
  Serial.print(" | GyroRoll: ");
  Serial.print(roll_gyro, 2);
  Serial.print(" GyroPitch: ");
  Serial.println(pitch_gyro, 2);

  // 5) Ángulos filtrados
  Serial.print(" | FilRoll: ");
  Serial.print(roll_fil, 2);
  Serial.print(" FilPitch: ");
  Serial.println(pitch_fil, 2);


  delay(500);
}


void leerMPU() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);              // Registro inicial ACCEL_XOUT_H
  Wire.endTransmission(false);

  // Pedir 14 bytes al sensor
  int n = Wire.requestFrom(MPU_addr, 14, true);

  // Solo si llegaron los 14 bytes se hace la lectura
  if (n == 14) {
    // Cada dato ocupa dos bytes: alto y bajo
    AcX = Wire.read() << 8 | Wire.read();
    AcY = Wire.read() << 8 | Wire.read();
    AcZ = Wire.read() << 8 | Wire.read();
    Tmp = Wire.read() << 8 | Wire.read();
    GyX = Wire.read() << 8 | Wire.read();
    GyY = Wire.read() << 8 | Wire.read();
    GyZ = Wire.read() << 8 | Wire.read();
  } else {
    // Si hubo error, se avisa por serial
    Serial.print("Error I2C, bytes recibidos: ");
    Serial.println(n);
  }
}