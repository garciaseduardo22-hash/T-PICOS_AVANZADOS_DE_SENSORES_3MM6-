#include <Wire.h>           // Comunicación I2C
#include <MPU6050.h>        // Librería específica para el sensor MPU6050
#include <SSD1306Ascii.h>   // Librería ligera para pantallas OLED (solo texto)
#include <SSD1306AsciiWire.h>

#define BOTON_PIN 2         // Pin donde está conectado el botón físico
#define OLED_ADDRESS 0x3C   // Dirección I2C común de las pantallas OLED

SSD1306AsciiWire oled;      // Objeto para controlar la pantalla
MPU6050 mpu;                // Objeto para el sensor de movimiento

const char* tareas[] = {    // Nombres de las materias asignadas a cada cara
  "Sin tarea", "Sin tarea", "Control Clasico", 
  "Inst. Virtual", "Dis. Maquina", "DSP", "Automatas Ind."
};

unsigned long tiempos[7] = {0, 0, 0, 0, 0, 0, 0}; // Acumuladores de milisegundos por cara
unsigned long inicioTarea = 0; // Marca de tiempo cuando se detectó la cara actual
int caraActual = 0;            // Cara que está actualmente hacia arriba
bool corriendo = false;        // Estado: ¿Está contando el tiempo?
bool enResumen = false;        // Estado: ¿Estamos viendo la pantalla final?

bool estadoBoton = LOW;        // Estado actual del botón
bool ultimoBoton = LOW;        // Estado anterior del botón (para detectar flancos)
unsigned long ultimoDebounce = 0; // Tiempo para el filtro anti-rebote
#define DEBOUNCE_MS 50         // Tiempo de espera para el anti-rebote

int detectarCara() {
  int16_t ax, ay, az;
  long sumX = 0, sumY = 0, sumZ = 0;

  for (int i = 0; i < 5; i++) {
    mpu.getAcceleration(&ax, &ay, &az);
    sumX += ax;
    sumY += ay;
    sumZ += az;
    delay(20);
  }

  long x = sumX / 5;
  long y = sumY / 5;
  long z = sumZ / 5;
  if (z > 12000) return 6;
  if (y > 12000)  return 5;
  if (x > 12000)  return 4;
  if (x < -12000) return 3;
  if (y < -12000) return 2;
  if (z < -12000) return 1;

  return 0;
}

String formatTiempo(unsigned long ms) { // Convierte milisegundos a "MM:SS"
  unsigned long seg = ms / 1000;
  unsigned long min = seg / 60;
  seg = seg % 60;
  char buf[10];
  sprintf(buf, "%02lu:%02lu", min, seg);
  return String(buf);
}

unsigned long tiempoTotal(int cara) { // Calcula el tiempo acumulado + el tiempo actual
  unsigned long t = tiempos[cara];
  if (corriendo && caraActual == cara) {
    t += millis() - inicioTarea; // Suma el tiempo que ha pasado desde que se volteó
  }
  return t;
}

void mostrarPantalla() {
  oled.clear();

  oled.setCursor(0, 0);
  oled.print("Tarea:");
  oled.setCursor(0, 1);
  if (caraActual == 0) {
    oled.print("-- Sin tarea --");
  } else {
    oled.print(tareas[caraActual]);
  }

  oled.setCursor(0, 2);
  oled.print("Tiempo: ");
  oled.print(formatTiempo(tiempoTotal(caraActual)));

  oled.setCursor(0, 4);
  oled.print("C2:");
  oled.print(formatTiempo(tiempoTotal(2)));
  oled.print(" C3:");
  oled.print(formatTiempo(tiempoTotal(3)));

  oled.setCursor(0, 5);
  oled.print("C4:");
  oled.print(formatTiempo(tiempoTotal(4)));
  oled.print(" C5:");
  oled.print(formatTiempo(tiempoTotal(5)));

  oled.setCursor(0, 6);
  oled.print("C6:");
  oled.print(formatTiempo(tiempoTotal(6)));
}

void mostrarInicio() {
  oled.clear();
  oled.setCursor(0, 2);
  oled.print("  Dado Digital");
  oled.setCursor(0, 4);
  oled.print(" Presiona boton");
  oled.setCursor(0, 5);
  oled.print("  para iniciar");
}

void mostrarResumen() {
  oled.clear();
  oled.setCursor(0, 0);
  oled.print("   -- RESUMEN --");

  oled.setCursor(0, 1);
  oled.print("Ctrl Clas: ");
  oled.print(formatTiempo(tiempos[2]));

  oled.setCursor(0, 2);
  oled.print("Inst Virt: ");
  oled.print(formatTiempo(tiempos[3]));

  oled.setCursor(0, 3);
  oled.print("Dis Maq:   ");
  oled.print(formatTiempo(tiempos[4]));

  oled.setCursor(0, 4);
  oled.print("DSP:       ");
  oled.print(formatTiempo(tiempos[5]));

  oled.setCursor(0, 5);
  oled.print("Automatas: ");
  oled.print(formatTiempo(tiempos[6]));

  oled.setCursor(0, 7);
  oled.print("Boton=nuevo inicio");
}

void setup() {
  Serial.begin(9600);
  delay(1000);
  Wire.begin();
  pinMode(BOTON_PIN, INPUT_PULLUP);

  Serial.println("Iniciando pantalla...");
  oled.begin(&Adafruit128x64, OLED_ADDRESS);
  oled.setFont(System5x7);
  oled.clear();
  Serial.println("Pantalla OK");

  Serial.println("Iniciando MPU...");
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("Error MPU");
    while (true);
  }
  Serial.println("MPU OK");
  Serial.println("Todo listo");

  mostrarInicio();
}

void loop() {
  bool lecturaBoton = digitalRead(BOTON_PIN);
  if (lecturaBoton != ultimoBoton) { ultimoDebounce = millis(); }

  if ((millis() - ultimoDebounce) > DEBOUNCE_MS) { // Filtro Debounce
    if (lecturaBoton != estadoBoton) {
      estadoBoton = lecturaBoton;
      if (estadoBoton == HIGH) { // Cuando se suelta el botón...
        
        if (!corriendo && !enResumen) { // CASO 1: Iniciar por primera vez
          for (int i = 0; i < 7; i++) tiempos[i] = 0;
          caraActual = detectarCara();
          inicioTarea = millis();
          corriendo = true;
        } 
        else if (corriendo) { // CASO 2: Pausar y mostrar resumen final
          if (caraActual != 0) { tiempos[caraActual] += millis() - inicioTarea; }
          corriendo = false;
          enResumen = true;
          mostrarResumen();
        } 
        else if (enResumen) { // CASO 3: Resetear todo tras el resumen
          enResumen = false;
          for (int i = 0; i < 7; i++) tiempos[i] = 0;
          caraActual = detectarCara();
          inicioTarea = millis();
          corriendo = true;
        }
      }
    }
  }
  ultimoBoton = lecturaBoton;

  if (!corriendo) return; // Si está pausado o en resumen, no hace nada más

  int cara = detectarCara(); // Revisa si volteaste el dado
  if (cara != caraActual) { // Si cambiaste de cara...
    if (caraActual != 0) {
      tiempos[caraActual] += millis() - inicioTarea; // Guarda lo acumulado en la cara anterior
    }
    caraActual = cara;       // Cambia a la nueva materia
    inicioTarea = millis();  // Reinicia el cronómetro para esta cara
  }

  mostrarPantalla(); // Actualiza los números en la OLED
  delay(200); // Pequeña pausa para no refrescar la pantalla tan rápido
}