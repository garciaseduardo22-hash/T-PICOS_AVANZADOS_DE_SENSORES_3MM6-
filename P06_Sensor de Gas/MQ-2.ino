const int pinMQ2 = A0;

// Constante de resistencia de carga en el módulo comercial MQ-2 (usualmente 10 kOhm)
const float RL = 10.0; 

// Factor de relación en aire limpio según datasheet del MQ-2
const float AIR_FACTOR = 9.83; 

float Ro = 10.0; // Valor inicial estimado para Ro, se recalibrará en el setup()

void setup() {
  Serial.begin(9600);
  
  Serial.println("=================================================");
  Serial.println("Estabilizando sensor y calibrando Ro en aire limpio...");
  Serial.println("=================================================");
  
  
  delay(5000); // Esperar 5 segundos para que empiece a entibiarse el sensor

  // Calibración automática de Ro en aire limpio
  long sumaADC = 0;
  int lecturas = 50;
  for(int i = 0; i < lecturas; i++) {
    sumaADC += analogRead(pinMQ2);
    delay(100);
  }
  float promedioADC = (float)sumaADC / (float)lecturas;
  
  // Evitar divisiones por cero
  if(promedioADC < 1) promedioADC = 1;

  // Calcular el voltaje en aire limpio
  float v_suelo = promedioADC * (5.0 / 1023.0);
  
  // Calcular Rs en aire limpio
  float Rs_aire = ((5.0 - v_suelo) * RL) / v_suelo;
  
  // Obtener Ro base
  Ro = Rs_aire / AIR_FACTOR;

  Serial.print("Calibración terminada. Ro fijado en: ");
  Serial.print(Ro);
  Serial.println(" kOhms.");
  Serial.println("Iniciando mediciones en tiempo real...");
  Serial.println("-------------------------------------------------");
}

void loop() {
  int adc = analogRead(pinMQ2);
  
  // Evitar lecturas fuera de rango o divisiones por cero
  if (adc < 1) adc = 1; 
  if (adc >= 1023) adc = 1022;

  // 1. Calcular el Voltaje actual
  float voltaje = adc * (5.0 / 1023.0);

  // 2. Calcular la Resistencia del sensor actual (Rs)
  float Rs = ((5.0 - voltaje) * RL) / voltaje;

  // 3. Calcular la relación Rs/Ro
  float ratio = Rs / Ro;

  // 4. Conversiones matemáticas a PPM basadas en las curvas del Datasheet
  float ppmH2     = 943.12 * pow(ratio, -2.78);
  float ppmMetano = 5824.7 * pow(ratio, -2.85);
  float ppmEtanol = 3450.2 * pow(ratio, -3.05);
  float ppmCO     = 2959.5 * pow(ratio, -3.42);

  // [MEJORA] Filtro para aire limpio: Si el ratio es muy alto, significa que no hay gas químico objetivo.
  // Las curvas del MQ-2 suelen iniciar su medición útil cuando el ratio Rs/Ro baja de ~4.0 o 5.0.
  if (ratio >= 9.0) {
    ppmH2 = 0.0;
    ppmMetano = 0.0;
    ppmEtanol = 0.0;
    ppmCO = 0.0;
  }

  // 5. Imprimir variables de forma clara en el Monitor Serial
  Serial.print("Lectura [ADC: "); Serial.print(adc);
  Serial.print(" | V: "); Serial.print(voltaje, 2); 
  Serial.print("V | Ratio Rs/Ro: "); Serial.print(ratio, 2); Serial.println("]");
  
  Serial.print(" >> H2: ");      Serial.print(ppmH2, 1);     Serial.print(" ppm | ");
  Serial.print("Metano: "); Serial.print(ppmMetano, 1); Serial.print(" ppm | ");
  Serial.print("Etanol: "); Serial.print(ppmEtanol, 1); Serial.print(" ppm | ");
  Serial.print("CO: ");     Serial.print(ppmCO, 1);     Serial.println(" ppm");
  
  Serial.println("-----------------------------------------------------------------");

  delay(1500); // Muestreo cada 1.5 segundos (ideal para ver la reacción rápida del encendedor)
}