// Definición del pin del sensor (Debe ser el 2 o 3 en Arduino UNO/Nano para soportar interrupciones)
const int pinSensor = 2;

// Variable volátil para contar los pulsos
volatile unsigned long contadorPulsos = 0;

// Variables para el control de tiempo y cálculo
unsigned long tiempoAnterior = 0;
const unsigned long intervalo = 1000;  // Tiempo de muestreo: 1 segundo (1000 ms)

// Factor de conversión teórico para sensores de 1/2" (Q = f / 7.5)
const float factorConversion = 7.72;

// FUNCIÓN CORREGIDA: Se eliminó el atributo de caché que rompía el código
void contarPulsos() {
  contadorPulsos++;
}

void setup() {
  // Inicializar comunicación serial a 9600 baudios
  Serial.begin(9600);

  // Configurar el pin del sensor como entrada con resistencia Pull-Up interna
  pinMode(pinSensor, INPUT_PULLUP);

  // Adjuntar la interrupción al pin 2.
  attachInterrupt(digitalPinToInterrupt(pinSensor), contarPulsos, FALLING);

  Serial.println("--- Sistema de Medición de Flujo Iniciado ---");
}

void loop() {
  unsigned long tiempoActual = millis();

  // Se ejecuta la medición cada vez que pasa 1 segundo (1000 ms)
  if (tiempoActual - tiempoAnterior >= intervalo) {

    // --- PASO 1: Deshabilitar interrupciones momentáneamente ---
    noInterrupts();
    unsigned long pulsosEnUnSegundo = contadorPulsos;
    contadorPulsos = 0;  // Reiniciamos el contador para el siguiente segundo
    interrupts();        // Volvemos a habilitar las interrupciones

    // --- PASO 2: Calcular Frecuencia (Pulsos por segundo) ---
    float frecuenciaHz = (float)pulsosEnUnSegundo;

    // --- PASO 3: Calcular Caudal en Litros por Minuto (L/min) ---
    float caudal_Lmin = frecuenciaHz / factorConversion;

    // --- PASO 4: Mostrar los resultados en el Monitor Serie ---
    Serial.print("Frecuencia: ");
    Serial.print(frecuenciaHz, 1);
    Serial.print(" Hz  |  Caudal: ");
    Serial.print(caudal_Lmin, 2);
    Serial.println(" L/min");

    // Guardar el tiempo actual para el siguiente ciclo
    tiempoAnterior = tiempoActual;
  }
}