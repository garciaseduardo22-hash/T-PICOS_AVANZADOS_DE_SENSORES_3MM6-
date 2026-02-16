#include <SoftwareSerial.h> // crea el puerto serial necesario para el GPS 
#include <TinyGPS++.h>// sirve para los GPS, extrae latitud, longitud, satélites, velocidad, etc.
// los puertos RX y TX van intercambiados entre arduino y GPS
const byte pinRX = 8;
const byte pinTX = 7; //puertos del arduino 
const unsigned long velocidadGPS = 9600; // velocidad común en comunicación UART
const unsigned long intervaloLectura = 3000; //imprime cada 3 segundos 

SoftwareSerial moduloGPS(pinRX, pinTX); //convierte la variable modulogps en un puerto serial 
TinyGPSPlus lectorGPS; //objeto que procesara los datos del GPS 

unsigned long tiempoAnterior = 0; //intervalo de tiempo entre milis 


void leerDatosGPS() {
  if (moduloGPS.available()) { // busca si hay datos en el gps 
    while (moduloGPS.available()) {
      lectorGPS.encode(moduloGPS.read()); // llama los datos del gps constantemente para actualizarlos 
    }
  }
}

void mostrarInformacion() { // imprime la información 

  Serial.println("=== ESTADO ACTUAL GPS ===");

  Serial.print("Numero de satelites: ");
  Serial.println(lectorGPS.satellites.value());

  Serial.print("Posicion valida: ");
  Serial.println(lectorGPS.location.isValid() ? "ACTIVA" : "SIN FIX");

  if (lectorGPS.location.isValid()) {

    double latitudActual = lectorGPS.location.lat();
    double longitudActual = lectorGPS.location.lng();

    Serial.print("Coordenada LAT -> "); //imprime cada coordenada de latitud 
    Serial.println(latitudActual, 6);

    Serial.print("Coordenada LNG -> "); //imprime cada coordenada de longitud
    Serial.println(longitudActual, 6); // usa 6 decimales de exactitud 
  }

  Serial.println("=================================");
  Serial.println();
}

void setup() {

  Serial.begin(9600); // inicia comunicación con LA PC 
  moduloGPS.begin(velocidadGPS);// inicia comunicación con el GPS 

  Serial.println("Sistema de monitoreo GPS iniciado");
}

void loop() {

  leerDatosGPS(); // lee los datos de GPS 

  if (millis() - tiempoAnterior >= intervaloLectura) { // controla el tiempo sin usar delay 
    tiempoAnterior = millis();
    mostrarInformacion();  // actualiza y muestra dichos datos actualizados 
  }
}
