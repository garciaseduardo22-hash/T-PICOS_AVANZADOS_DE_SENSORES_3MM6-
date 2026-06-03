const int pinMQ2 = A0;

// Constante de resistencia de carga en el módulo comercial MQ-2 (usualmente 10 kOhm)
const float RL = 10.0; 

// Factor de relación en aire limpio para Etanol (según datasheet del MQ-2)
const float AIR_FACTOR = 9.83; 

float Ro = 10.0; // Valor de referencia que se calculará en el setup()

// Umbral de alcohol: si las PPM superan este valor, se activa el modo alcoholímetro 
const float UMBRAL_ALCOHOL = 150.0; 

void setup() {
  Serial.begin(9600);
  

  Serial.println("Estabilizando sensor y calibrando en aire limpio...");
  Serial.println("Por favor, asegúrate de que NO haya vapores de alcohol cerca.");
  
  // Esperar 5 segundos para el precalentamiento inicial del sensor
  delay(5000); 

  // Calibración automática de Ro en aire limpio
  long sumaADC = 0;
  int lecturas = 50;
  for(int i = 0; i < lecturas; i++) {
    sumaADC += analogRead(pinMQ2);
    delay(100);
  }
  float promedioADC = (float)sumaADC / (float)lecturas;
  
  if(promedioADC < 1) promedioADC = 1; // Evitar división por cero

  // Calcular el voltaje y la resistencia Ro base en aire limpio
  float v_suelo = promedioADC * (5.0 / 1023.0);
  float Rs_aire = ((5.0 - v_suelo) * RL) / v_suelo;
  Ro = Rs_aire / AIR_FACTOR;

  Serial.println("Dispositivo LISTO. Puedes comenzar a realizar las pruebas.");
 }

void loop() {
  int adc = analogRead(pinMQ2);
  
  // Protecciones para el ADC
  if (adc < 1) adc = 1; 
  if (adc >= 1023) adc = 1022;

  //  Voltaje actual
  float voltaje = adc * (5.0 / 1023.0);

  //  Calcular la Resistencia actual del sensor (Rs)
  float Rs = ((5.0 - voltaje) * RL) / voltaje;

  // Calcular la relación Rs/Ro
  float ratio = Rs / Ro;

  //Calcular las PPM de Etanol
  float ppmEtanol = 3450.2 * pow(ratio, -3.05);

  // Filtro para estabilizar la lectura en aire limpio
  if (ratio >= 9.0) {
    ppmEtanol = 0.0;
  }
  
  if (ppmEtanol > UMBRAL_ALCOHOL) {
    // Caso 1: Se detecta aliento con presencia de alcohol
    Serial.print("Voltaje del sensor: "); Serial.print(voltaje, 2); Serial.println("V");
    Serial.print("Concentración de Etanol: "); Serial.print(ppmEtanol, 1); Serial.println(" PPM");
    
    // Veredicto según la intensidad de la muestra simulada
    if (ppmEtanol > 1500.0) {
      Serial.println("RESULTADO: REPROBADO (Alcoholemia Positiva - No Apto)");
    } else {
      Serial.println("RESULTADO: PRECAUCIÓN (Presencia moderada detectada)");
    }
    Serial.println("-------------------------------------------------");
  } 
  else {
    //  El aire está limpio o el usuario sopló sin alcohol
    Serial.print("[Monitoreo] Estado: AIRE LIMPIO | Etanol: ");
    Serial.print(ppmEtanol, 1);
    Serial.println(" PPM");
  }
if (ppmEtanol > UMBRAL_ALCOHOL) {
    //  Se detecta aliento con presencia de alcohol
    Serial.println("\n-------------------------------------------------");
    Serial.println(">>> [ALERTA] ANALIZANDO MUESTRA DE ALIENTO... <<<");
    Serial.print("Voltaje del sensor: "); Serial.print(voltaje, 2); Serial.println("V");
    Serial.print("Concentración de Etanol: "); Serial.print(ppmEtanol, 1); Serial.println(" PPM");
    
    // 1 PPM en aire equivale aproximadamente a 0.001 mg/L en aliento.
    // Multiplicando mg/L en aliento por 2.1 (o factor 2100) obtenemos g/L en sangre.
    float alcoholemiaSangre = (ppmEtanol * 0.001) * 2.1; 
    
    Serial.print("Nivel de Alcohol en Sangre estimado: "); 
    Serial.print(alcoholemiaSangre, 2); Serial.println(" g/L");
   
    // Nivel de sangre  (Estándar común: > 0.8 g/L es positivo)
    if (alcoholemiaSangre >= 0.8) {
      Serial.println("RESULTADO: REPROBADO (Alcoholemia Positiva - No Apto)");
    } else {
      Serial.println("RESULTADO: PRECAUCIÓN (Presencia moderada detectada)");
    }
  }
  delay(1500); // Muestreo cada 1.5 segundos para no saturar la pantalla
}