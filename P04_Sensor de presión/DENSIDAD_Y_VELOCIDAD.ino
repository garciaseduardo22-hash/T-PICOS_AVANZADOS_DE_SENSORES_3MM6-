#include <Wire.h>
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;// objeto que usara el sensor

// --- Constantes Físicas ---
const float R_SECO = 287.05;   // Constante aire seco [J/kg*K]
const float R_VAPOR = 461.495; // Constante vapor de agua [J/kg*K]
const float RADIO = 1.0;       // Radio de la columna de aire (1 metro) 
const float PI_VAL = 3.141592;// el numero es al redondeo 
const float AREA = PI_VAL * (RADIO * RADIO); 

const float v_viento = 8.24;         // Parámetro WS10M (m/s)
const float humedad_relativa = 23.11; // Parámetro RH2M (%) 

void setup() {
  Serial.begin(9600);// inicia el monitor serial
  if (!bmp.begin()) {
    Serial.println("ERROR: No se encontro el BMP180!");
    while (1); 
  }
  Serial.println("BMP180 detectado OK!");
}

void loop() {
  // 1. Lecturas del sensor
  float temperaturaC = bmp.readTemperature();       
  long presionPa = bmp.readPressure();              
  float tempK = temperaturaC + 273.15;

  // 2. Método Clásico (Aire Seco - Gases Ideales) 
  float densidadSeca = presionPa / (R_SECO * tempK);
  float potenciaSeca = 0.5 * densidadSeca * AREA * pow(v_viento, 3); 

  // 3. Método CIPM-2007 (Aire Húmedo) 
  // Presión de saturación usando la ecuación de Tetens
  float pSat = 610.78 * pow(10, (7.5 * temperaturaC) / (temperaturaC + 237.3));
  // Presión parcial del vapor de agua
  float pV = pSat * (humedad_relativa / 100.0);
  // Presión parcial del aire seco
  float pD = presionPa - pV;
  // Densidad húmeda final
  float densidadCIPM = (pD / (R_SECO * tempK)) + (pV / (R_VAPOR * tempK));
  float potenciaCIPM = 0.5 * densidadCIPM * AREA * pow(v_viento, 3); 

  // --- Impresión de Resultados ---
  Serial.println("============================================");
  Serial.print("Presion Act: "); Serial.print(presionPa); Serial.println(" Pa");
  Serial.print("Temp Act:    "); Serial.print(temperaturaC, 1); Serial.println(" C");
  Serial.print("Humedad (NASA): "); Serial.print(humedad_relativa); Serial.println(" %"); 
  
  Serial.println("\n--- METODO 1: AIRE SECO ---");
  Serial.print("Densidad: "); Serial.print(densidadSeca, 4); Serial.println(" kg/m3");
  Serial.print("Potencia: "); Serial.print(potenciaSeca, 2); Serial.println(" W (J/s)"); 

  Serial.println("\n--- METODO 2: CIPM-2007 ---");
  Serial.print("Densidad: "); Serial.print(densidadCIPM, 4); Serial.println(" kg/m3");
  Serial.print("Potencia: "); Serial.print(potenciaCIPM, 2); Serial.println(" W (J/s)"); 

  // Cálculo del porcentaje de diferencia para tu análisis
  float dif_porcentaje = ((densidadSeca - densidadCIPM) / densidadSeca) * 100.0;
  Serial.println("\n--- ANALISIS ---");
  Serial.print("El aire humedo es "); Serial.print(dif_porcentaje, 2); 
  Serial.println("% menos denso que el aire seco.");

  delay(5000);
}