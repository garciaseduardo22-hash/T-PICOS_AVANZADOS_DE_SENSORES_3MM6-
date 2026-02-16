#include <TinyGPSPlus.h>// elocaliza el gps 
#include <SoftwareSerial.h> // crea un puerto serial
#include <math.h>// trabaja con funciones trigonométricas y constantes

// pines 
static const int GPS_RX = 4;
static const int GPS_TX = 3;
static const int LED_PIN = 13;
static const int BUTTON_PIN = 7;

const double RADIUS_METERS = 30.0; // radio de la geocerca en metros 

TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);// UART o comunicación serial con el GPS

double baseLat = 0.0;
double baseLon = 0.0; // aqui se guarda el punto centro al presionar el botón 
bool baseSet = false; 


double toRadians(double deg) { // convierte de grados a radianes para que se pueda usar la función de haservine 
  return deg * (M_PI / 180.0);
}

double distanceMeters(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0;  // radio promedio de la tierra para haservine 
  double dLat = toRadians(lat2 - lat1); //diferencia entre latitud 1 y 2 en metros 
  double dLon = toRadians(lon2 - lon1); //diferencia entre longitud 1 y 2 en metros 

 
 // calculo del valor de haservine 
  double a = sin(dLat/2)*sin(dLat/2) + 
             cos(toRadians(lat1))*cos(toRadians(lat2)) *
             sin(dLon/2)*sin(dLon/2);

 
 // ángulo entre puntos en radianes 
  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  return R * c;
}

// inicia comunicación serial al GPS y PC
void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);

  pinMode(LED_PIN, OUTPUT); //salida del led 
  pinMode(BUTTON_PIN, INPUT_PULLUP); //entrada del boton 

  Serial.println("Esperando GPS...");
}

void loop() { // lee los datos del GPS 

  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isValid()) { // verifica si los datos de GPS son validos 

    double lat = gps.location.lat();
    double lon = gps.location.lng();

    //  Si presionas botón se guarda la ubicación 
    if (digitalRead(BUTTON_PIN) == LOW && !baseSet) { 
      baseLat = lat;
      baseLon = lon;
      baseSet = true; // activa la bandera del punto centro 

      Serial.println("Centro de geocerca guardado!");
      delay(1000); // evita rebote
    }

    if (baseSet) {
      double d = distanceMeters(baseLat, baseLon, lat, lon); // calcula la distancia una vez se guarda el punto 

      Serial.print("Distancia: ");
      Serial.println(d);

      if (d > RADIUS_METERS) { // el led se enciende si salgo de los 30 metros 
        digitalWrite(LED_PIN, HIGH);
      } else {
        digitalWrite(LED_PIN, LOW); // si me regreso el led se apaga 
      }
    }
  }
}
