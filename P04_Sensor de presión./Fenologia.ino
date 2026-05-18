#include <Wire.h>
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;

// --- CONSTANTES PUNTO 2 ---
const float TB = 10.0; // Temperatura base o umbral para frijol 

// --- VARIABLES GLOBALES ---
float tMax = -100.0;   // Valor inicial muy bajo para capturar la primera lectura
float tMin = 100.0;    // Valor inicial muy alto para capturar la primera lectura

// --- NUEVAS VARIABLES PARA EL ACUMULADOR ---
float ucAcumuladas = 0.0;        // Aqui se suma las UC para simular el paso de los dias
unsigned long tiempoAnterior = 0;
const long DURACION_DIA = 10000; // Cada 10 segundos se simula 1 dia

void setup() {
  Serial.begin(9600);// monitor serial 
  
  if (!bmp.begin()) {
    Serial.println("ERROR: No se encontro el BMP180!");// adevertencia por si funciona el sensor
    while (1); 
  }

  Serial.println("BMP180 detectado OK!");
}

void loop() {
  // 1. Lectura de temperatura constante
  float temperaturaActual = bmp.readTemperature(); // °C 

  // 2. Actualización de Máxima y Mínima del "día actual"
  if (temperaturaActual > tMax) tMax = temperaturaActual;
  if (temperaturaActual < tMin) tMin = temperaturaActual;

  // --- MONITOREO EN TIEMPO REAL ---
  Serial.println("--------------------------------------------");
  Serial.print("Temp Actual: "); Serial.print(temperaturaActual, 1); Serial.println(" °C"); // Temperaturas maximas y minimas en grafos Celsius
  Serial.print("Max del dia: "); Serial.print(tMax, 1); Serial.println(" °C");
  Serial.print("Min del dia: "); Serial.print(tMin, 1); Serial.println(" °C");
  Serial.print("UC Totales Acumuladas: "); Serial.println(ucAcumuladas, 2);

  //ETAPA FENOLÓGICA 
  Serial.print("ETAPA FENOLOGICA:   ");
  if (ucAcumuladas < 12.89) {
    Serial.println("Siembra / Pre-Emergencia");
  } else if (ucAcumuladas < 64.41) {
    Serial.println("Emergencia");
  } else if (ucAcumuladas < 91.33) {
    Serial.println("Formacion de guias");
  } else if (ucAcumuladas < 99.31) {
    Serial.println("Floracion");
  } else if (ucAcumuladas < 112.77) {
    Serial.println("Formacion de vaina");
  } else if (ucAcumuladas < 153.24) {
    Serial.println("Llenado de Vainas");
  } else {
    Serial.println("Maduracion");
  }

  // SIMULACIÓN DE FIN DE DÍA 
  if (millis() - tiempoAnterior >= DURACION_DIA) {
    tiempoAnterior = millis();

    // Calcular Unidades Calor obtenidas 
    float ucDelDia = ((tMax + tMin) / 2.0) - TB;
    if (ucDelDia < 0) ucDelDia = 0; // Evitar valores negativos

    //acumulador global para que la planta crezca
    ucAcumuladas += ucDelDia;

    Serial.println("\n>>> ¡HA PASADO UN DÍA SIMULADO! <<<");
    Serial.print("UC ganadas hoy: "); Serial.println(ucDelDia, 2);
    Serial.println("============================================\n");

    // REINICIAR tMax y tMin para registrar el nuevo día
    tMax = -100.0;
    tMin = 100.0;
  }

  delay(2000); // Intervalo de actualización en pantalla
}