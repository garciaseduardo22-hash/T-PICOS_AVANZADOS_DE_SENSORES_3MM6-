#include <Wire.h>// Librería para comunicación I2C
#include <Adafruit_VL53L0X.h>// Librería del sensor 

Adafruit_VL53L0X lox = Adafruit_VL53L0X();   // Crea un objeto para manejar el sensor

const int alturaSensor = 400;// Altura del sensor  a la mesa
const int pasoX = 5;// Distancia entre cada medición 


int indice = 0;// Contador de las mediciones

void setup() {
  Serial.begin(115200);         // baud rate de 115200
  Wire.begin();                 // Inicia comunicación I2C con el sensor

  if (!lox.begin()) {// Intenta inicializar el sensor VL53L0X
    Serial.println("Error VL53L0X"); // Si falla, muestra mensaje de error
    while (1);// Detiene el programa si el sensor no responde
  }

  Serial.println("Escribe m y presiona Enter para medir"); // Indica cómo tomar mediciones
  Serial.println("r = reiniciar"); // Comando para reiniciar el escaneo
  Serial.println("x_mm,y_mm");// titulo para los datos medidos.
}

void loop() 
  if (Serial.available() > 0) {// Verifica si se presiono m o r 
    char comando = Serial.read();// Lee el carácter enviado desde el monitor serial

   
    while (Serial.available() > 0) {   // descarta caracteres que no sean m y r
      Serial.read();                  
    }

    if (comando == 'm' || comando == 'M') {   // ejecuta la medición 
      medirPunto();                          
    }
    else if (comando == 'r' || comando == 'R') { // Si el usuario escribió 'r'
      indice = 0;// Reinicia el contador de posiciones
      Serial.println("x_mm,y_mm");// muestra de nuevo el titulo porque se reinicio 
    }
  }
}

void medirPunto() {
  VL53L0X_RangingMeasurementData_t measure;// Estructura donde se guardará la medición

  lox.rangingTest(&measure, false); // Realiza una medición con el sensor

  if (measure.RangeStatus == 0) {// Verifica si la medición fue válida
    int distancia = measure.RangeMilliMeter;  // Obtiene la distancia medida en milímetros
    int x = indice * pasoX;// Calcula la posición en X según el número de medición
    int y = alturaSensor - distancia;// Calcula la altura relativa del objeto

    if (y < 0) y = 0;// Si la altura es negativa (por ruido), la fija en 0

    Serial.print(x);// Envía la posición X al monitor serial
    Serial.print(",");// Separador para formato CSV
    Serial.println(y);// Envía la altura Y

    indice++;// Incrementa el contador de mediciones
  } else {
    Serial.print("Lectura invalida. Status = "); // Si la medición falla, muestra mensaje
    Serial.println(measure.RangeStatus);        // Indica el código de error del sensor
  }
}