#include <Wire.h>    // Librería para el bus I2C
#include <MPU6050.h> // Librería de alto nivel para el sensor (facilita la lectura)

// Instancia del sensor (Crea el objeto 'mpu' para acceder a sus funciones)
MPU6050 mpu;

// Variables globales para almacenar los ángulos finales en grados
int pitch = 0; // Inclinación adelante/atrás
int roll = 0;  // Inclinación izquierda/derecha

void setup() {
  Serial.begin(115200); // Comunicación serie rápida
  Wire.begin();         // Inicia el bus I2C

  // Inicialización del sensor (despierta y configura escalas por defecto)
  Serial.println("Iniciando MPU6050...");
  mpu.initialize();

  // Verificación de hardware: si el sensor no responde, detiene el programa
  if (mpu.testConnection()) {
    Serial.println("MPU6050 conectado correctamente");
  } else {
    Serial.println("Error de conexion con MPU6050");
    while (1); // Bucle infinito de seguridad
  }
}

void loop() {
  int16_t ax, ay, az; // Variables para la aceleración cruda (-32768 a 32767)
  
  // 1. Obtener aceleración cruda de los 3 ejes directamente del sensor
  mpu.getAcceleration(&ax, &ay, &az);

  // 2. Cálculo de ángulos (Normalización y Trigonometría)
  // Dividimos entre 16384 porque la escala por defecto es ±2g
  float x_norm = ax / 16384.0;
  float y_norm = ay / 16384.0;
  float z_norm = az / 16384.0;
  
  // Cálculo de Pitch y Roll usando atan2 (más estable que atan)
  // M_PI es la constante 3.1415... para convertir radianes a grados
  pitch = -(atan2(x_norm, sqrt(y_norm * y_norm + z_norm * z_norm)) * 180.0) / M_PI;
  roll = (atan2(y_norm, z_norm) * 180.0) / M_PI;

  // 3. IDENTIFICACIÓN MANUAL
  // Usamos el umbral de 12000 (casi 1g) para detectar inclinaciones mayores a 45 grados
  String posicion = "Atras"; // Por defecto, si nada se cumple, está plano o neutro

  if (ay < -12000) {
    posicion = "Adelante"; // El eje Y negativo apunta hacia abajo
  } 
  else if (ay > 12000) {
    posicion = "Atras";    // El eje Y positivo apunta hacia abajo
  } 
  else if (ax < -12000) {
    posicion = "Izquierda"; // El eje X negativo apunta hacia abajo
  } 
  else if (ax > 12000) {
    posicion = "Derecha";   // El eje X positivo apunta hacia abajo
  }
  else if (az < -12000) {
    posicion = "Horizontal"; // El sensor está boca abajo (Z negativo)
  }
  // 4. Salida de datos para el monitor serie
  Serial.print("Pitch: "); Serial.print(pitch);
  Serial.print("\tRoll: "); Serial.print(roll);
  Serial.print("\t-> POSICIÓN: ");
  Serial.println(posicion);

  // Pausa de 350ms: tiempo suficiente para que un humano lea el texto
  // y coincide con la frecuencia de procesamiento de una red neuronal típica.
  delay(350); 
}