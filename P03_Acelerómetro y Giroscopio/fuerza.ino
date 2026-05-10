#include <Wire.h>          // Incluye la librería para comunicación I2C (protocolo que usa el sensor)
#include <MPU6050.h>       // Incluye la librería específica para controlar el sensor MPU6050

// Declaración del objeto MPU para poder acceder a sus funciones
MPU6050 mpu;

// --- CONFIGURACIÓN ---
// Definimos la masa del teléfono en kilogramos (dato necesario para F = m * a)
float masaObjeto = 0.235; 

void setup() {
  // Inicia la comunicación serial a 115200 baudios para ver datos en la PC
  Serial.begin(115200);
  
  // Inicia el bus I2C para la comunicación entre el Arduino y el sensor
  Wire.begin();
  
  // Ejecuta la rutina de configuración interna del sensor (escalas, filtros, etc.)
  mpu.initialize();
  
  // Verifica si el sensor responde correctamente antes de empezar
  if (mpu.testConnection()) {
    Serial.println("Sensor MPU6050 listo para medir Fuerza");
    Serial.println("Masa configurada: " + String(masaObjeto) + " kg");
  } else {
    // Si hay un error de cables o dirección, detiene el programa aquí
    Serial.println("Error de conexión");
    while (1);
  }
}

void loop() {
  // Declaramos variables para almacenar las lecturas "crudas" (enteros de 16 bits)
  int16_t ax, ay, az;
  
  // Obtenemos los valores de aceleración de los tres ejes directamente del sensor
  mpu.getAcceleration(&ax, &ay, &az);

  // 1. Convertir la lectura cruda (bits) a unidades físicas (m/s^2)
  // Dividimos entre 16384.0 (sensibilidad por defecto) y multiplicamos por la gravedad terrestre (9.81)
  float aceleracion_x = (ax / 16384.0) * 9.81; 

  // 2. Aplicar la Segunda Ley de Newton: Fuerza = Masa * Aceleración
  // Usamos abs() para obtener la magnitud de la fuerza sin importar la dirección del empuje
  float fuerza = masaObjeto * abs(aceleracion_x);

  // 3. Envío de resultados al Monitor Serial
  Serial.print("Fuerza: ");
  Serial.print(fuerza, 4); // Imprime el valor de la fuerza con 4 decimales de precisión
  Serial.println(" N");    // Agrega la unidad "Newtons" y un salto de línea

  // Pausa de 1 segundo entre lecturas (puedes bajarlo a 100 para capturar mejor el movimiento)
  delay(1000); 
}