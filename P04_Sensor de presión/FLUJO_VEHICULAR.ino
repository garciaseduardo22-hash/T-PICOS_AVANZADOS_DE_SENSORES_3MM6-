#include <Wire.h>
#include <Adafruit_BMP085.h>// libreria para el sensor

Adafruit_BMP085 bmp;//objeto que usara el sensor

// umbrales para cada vehiculo
const float UMBRAL_MOTO   = 1000.0;
const float UMBRAL_AUTO   = 2000.0;
const float UMBRAL_CAMION = 3000.0;

//Distancias Predeterminadas en metros
const float DIST_MOTO   = 1.30; 
const float DIST_AUTO   = 2.65;
const float DIST_CAMION = 5.00;

const unsigned long TIEMPO_MIN_ENTRE_EJES = 150; // Para evitar rebotes del mismo eje
const unsigned long TIEMPO_MAX_ENTRE_EJES = 3000; // Si no hay 2do pulso en 3s, se cancela
const unsigned long COOLDOWN_VEHICULO     = 800;  // Tiempo para esperar al siguiente carro

float baselinePresion = 0;//almacena la presión inicial
bool esperandoSegundoPulso = false;// activa la maquina de estados 
unsigned long tiempoPrimerPulso = 0;// cuando pisaron la manguera por primera vez 
unsigned long ultimaDeteccionTotal = 0;// segunda deteccion de la maguera 
float presionMaximaRegistrada = 0;

int contMoto = 0, contAuto = 0, contCamion = 0, contTotal = 0;

void setup() {
  Serial.begin(9600);

  if (!bmp.begin()) {
    Serial.println("ERROR: No se encontró el BMP180.");
    while (1);
  }

  Serial.println("Calibrando... no presiones la manguera.");
  delay(2000);

  long suma = 0;
  for (int i = 0; i < 50; i++) {
    suma += bmp.readPressure();
    delay(50);// muestra 50 lecturas
  }
  baselinePresion = suma / 50.0;

  Serial.print("Baseline: ");
  Serial.print(baselinePresion);
  Serial.println(" Pa - Sistema listo (Esperando primer pulso)");
}

void loop() {
  float presionActual = bmp.readPressure();
  float delta = presionActual - baselinePresion;// cálculo de la variación de presión. 
  unsigned long ahora = millis();

  //  PRIMER PULSO 
  if (!esperandoSegundoPulso && delta > UMBRAL_MOTO) {
    // Solo detecta si ya pasó el  vehículo anterior
    if (ahora - ultimaDeteccionTotal > COOLDOWN_VEHICULO) {
      esperandoSegundoPulso = true;
      tiempoPrimerPulso = ahora;
      presionMaximaRegistrada = delta;
      Serial.println("-> Primer pulso detectado. Esperando segundo...");
    }
  }

  // SEGUNDO PULSO 
  if (esperandoSegundoPulso) {
    // Actualizamos la presión máxima por si el segundo eje pisa más fuerte
    if (delta > presionMaximaRegistrada) presionMaximaRegistrada = delta;

    // Si detectamos el segundo impacto (respetando un tiempo mínimo de separación)
    if (delta > UMBRAL_MOTO && (ahora - tiempoPrimerPulso) > TIEMPO_MIN_ENTRE_EJES) {
      unsigned long tiempoTranscurrido = ahora - tiempoPrimerPulso;

      // Muestra los datos si se detecta la segunda detección. 
      procesarYMostrar(presionMaximaRegistrada, tiempoTranscurrido);

      // Reset total para el siguiente vehículo
      esperandoSegundoPulso = false;
      ultimaDeteccionTotal = ahora;
    }

    // Cancelación por tiempo (Si el vehículo se detuvo o fue un error)
    if (ahora - tiempoPrimerPulso > TIEMPO_MAX_ENTRE_EJES) {
      esperandoSegundoPulso = false;
      Serial.println("-> Segundo pulso no detectado (Timeout). Reintentando...");
    }
  }

  delay(10); // Lectura rápida para no perder el pico de presión
}

void procesarYMostrar(float presion, unsigned long tMs) {
  contTotal++;
  String tipo;
  float distancia = 0;

  // Clasificación por presión
  if (presion >= UMBRAL_CAMION)      { tipo = "CAMION/BUS"; contCamion++; distancia = DIST_CAMION; }
  else if (presion >= UMBRAL_AUTO)   { tipo = "AUTOMOVIL";  contAuto++;   distancia = DIST_AUTO;   }
  else                               { tipo = "MOTO/BICI";  contMoto++;   distancia = DIST_MOTO;   }

  // Velocidad = Distancia / Tiempo (convertido a km/h)
  float velocidadKmh = (distancia / (tMs / 1000.0)) * 3.6;

  Serial.println("\n=== VEHICULO COMPLETADO ===");
  Serial.print("Tipo        : "); Serial.println(tipo);
  Serial.print("Velocidad   : "); Serial.print(velocidadKmh); Serial.println(" km/h");
  Serial.print("Tiempo e/e  : "); Serial.print(tMs); Serial.println(" ms");
  Serial.print("Presion Max : "); Serial.print(presion); Serial.println(" Pa");
  Serial.println("----------- CONTEO -----------");
  Serial.print("Motos: "); Serial.print(contMoto);
  Serial.print(" | Autos: "); Serial.print(contAuto);
  Serial.print(" | Camiones: "); Serial.println(contCamion);
  Serial.print("TOTAL: "); Serial.println(contTotal);
  Serial.println("==============================\n");
}