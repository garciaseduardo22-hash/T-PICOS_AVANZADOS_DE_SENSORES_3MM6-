#include <Wire.h> // Librería para comunicación I2C (protocolo que usa el MPU6050)

const int MPU_addr = 0x68; // Dirección I2C estándar del sensor MPU6050

// Variables para almacenar los datos "crudos" (enteros de 16 bits) del sensor
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;
float ax, ay, az; // Variables para la aceleración convertida a unidades "g"

unsigned long lastSample = 0;     // Control de tiempo para tomar muestras constantes
unsigned long lastStepTime = 0;   // Almacena el tiempo del último paso detectado

float a_mag = 0.0;     // Magnitud total del vector de aceleración
float gravity_lp = 1.0; // Valor filtrado que representa la gravedad (tu "suelo" de referencia)
float a_dyn = 0.0;     // Aceleración dinámica (el movimiento real sin la gravedad)

int pasos = 0;           // Contador total de pasos
bool arribaUmbral = false; // Estado para saber si el sensor está en medio de un movimiento de paso

// --- PARAMETROS DE AJUSTE ---
const float alpha = 0.98;          // Qué tanto peso tiene el pasado (0.98) vs el presente (0.02)
const float threshold = 0.50;      // Fuerza mínima (en g) para considerar que un movimiento es un paso
const float rearmThreshold = 0.06; // El valor debe bajar de aquí para "rearmar" el contador
const unsigned long samplePeriod = 20;   // Tomamos una muestra cada 20ms (50 veces por segundo)
const unsigned long refractory = 500;    // Tiempo de espera (medio segundo) para evitar dobles conteos

void setup() {
  Wire.begin();        // Inicia el bus I2C
  Serial.begin(115200); // Inicia comunicación serie rápida

  // Despertar sensor (por defecto viene en modo sueño/sleep)
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B); // Registro de gestión de energía
  Wire.write(0x00); // Ponemos a 0 para encenderlo
  Wire.endTransmission(true);

  // Configurar Acelerómetro en escala +-2 g (máxima precisión)
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x1C); // Registro de configuración de acelerómetro
  Wire.write(0x00); // Valor para rango +-2g
  Wire.endTransmission(true);

  delay(200); // Espera a que el sensor se estabilice eléctricamente

  // --- AUTOCALIBRACIÓN INICIAL ---
  float sumMag = 0;
  for(int i=0; i<50; i++) { // Promediamos 50 lecturas iniciales
    leerMPU();
    // Convertimos valores crudos a "g" dividiendo por la sensibilidad (16384 para +-2g)
    float curAx = AcX / 16384.0;
    float curAy = AcY / 16384.0;
    float curAz = AcZ / 16384.0;
    sumMag += sqrt(curAx*curAx + curAy*curAy + curAz*curAz); // Pitágoras 3D
    delay(10);
  }
  gravity_lp = sumMag / 50.0; // Establecemos la base de gravedad real de TU sensor
  
  Serial.println("Calibracion lista...");
  lastSample = millis();
}

void loop() {
  // Solo ejecutamos la lógica si ya pasaron los 20ms del periodo de muestreo
  if (millis() - lastSample >= samplePeriod) {
    lastSample = millis();

    leerMPU(); // Pedimos datos nuevos al sensor

    // Conversión de valores enteros a punto flotante (Gs terrestres)
    ax = AcX / 16384.0;
    ay = AcY / 16384.0;
    az = AcZ / 16384.0;

    // Calculamos la Magnitud Total del vector (no importa en qué posición esté el sensor)
    a_mag = sqrt(ax*ax + ay*ay + az*az);

    // 1. Filtro Pasa-Bajas: Actualizamos lentamente dónde está la gravedad
    // Esto hace que si el sensor se inclina, el sistema se adapte solo
    gravity_lp = (alpha * gravity_lp) + ((1.0 - alpha) * a_mag);

    // 2. Componente Dinámica: Al valor actual le restamos la gravedad estimada
    // a_dyn debería ser cercano a 0 si el sensor no se mueve
    a_dyn = a_mag - gravity_lp;

    // 3. Detección de paso con Histéresis
    // Si detectamos un pico mayor al umbral y no estamos en tiempo de bloqueo (refractory)
    if (!arribaUmbral && a_dyn > threshold) {
      if ((millis() - lastStepTime) > refractory) {
        pasos++; // ¡Contamos un paso!
        lastStepTime = millis(); // Guardamos el tiempo para el bloqueo temporal
        arribaUmbral = true;      // Marcamos que estamos "dentro" de un paso
      }
    }

    // 4. Rearme del detector:
    // El valor dinámico debe bajar casi a cero para permitir detectar el siguiente paso
    if (arribaUmbral && a_dyn < rearmThreshold) {
      arribaUmbral = false;
    }

    // Imprimimos resultados para el Monitor Serie/Plotter
    Serial.print("amag: "); Serial.print(a_mag, 3);
    Serial.print(" | grav: "); Serial.print(gravity_lp, 3);
    Serial.print(" | adyn: "); Serial.print(a_dyn, 3);
    Serial.print(" | pasos: "); Serial.println(pasos);
  }
}

// Función para pedir los 14 registros de datos al MPU6050 vía I2C
void leerMPU() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B); // Empezamos a leer desde el registro 0x3B (Acelerómetro X)
  Wire.endTransmission(false);
  int n = Wire.requestFrom(MPU_addr, 14, true); // Pedimos 14 bytes (Acc, Temp, Gyro)

  if (n == 14) {
    // Unimos los dos bytes (alto y bajo) de cada eje usando desplazamiento de bits
    AcX = Wire.read() << 8 | Wire.read();
    AcY = Wire.read() << 8 | Wire.read();
    AcZ = Wire.read() << 8 | Wire.read();
    Tmp = Wire.read() << 8 | Wire.read();
    GyX = Wire.read() << 8 | Wire.read();
    GyY = Wire.read() << 8 | Wire.read();
    GyZ = Wire.read() << 8 | Wire.read();
  }
}