//*
  //============================================================/*//
  // CAJA NEGRA - Telemetria para vehiculo RC  (v3)
  // UPIIZ-IPN | Topicos Avanzados de Sensores
  //============================================================
  

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <TinyGPSPlus.h>
#include <BluetoothSerial.h>

// ================== PINES ==================
#define PIN_SDA      21
#define PIN_SCL      22
#define PIN_GPS_RX   16
#define PIN_GPS_TX   17
#define PIN_SD_CS     5
#define PIN_FSR1     34
#define PIN_FSR2     35

// ============ MPU6050 (registros) ============
#define MPU_ADDR        0x68
#define REG_PWR_MGMT_1  0x6B
#define REG_CONFIG      0x1A
#define REG_GYRO_CFG    0x1B
#define REG_ACCEL_CFG   0x1C
#define REG_ACCEL_XOUT  0x3B
// Escalas configuradas: acel +/-8g (4096 LSB/g), giro +/-500 dps (65.5 LSB/dps)
const float ESCALA_ACC  = 9.81f / 4096.0f;
const float ESCALA_GYRO = 1.0f / 65.5f;

// ================ PARAMETROS ================
const float G = 9.81;
const unsigned long PERIODO_MS = 500;     // 2 Hz
const float UMBRAL_IMPACTO     = 14.7;    // |mag-g| > 1.5g => evento
const int   FSR_UMBRAL_OCUPADO = 1200;
const int   FSR_UMBRAL_LIBRE   = 800;
const float PRESION_NIVEL_MAR  = 1013.25;
const int   MUESTRAS_TARA      = 100;     // 2 s de nivelacion

// --- Geocerca ---
const float GEOCERCA_RADIO_M   = 50.0;    // radio en metros
// --- Volcadura (eje +X apunta arriba del vehiculo) ---
const float UMBRAL_VOLCADURA   = 45.0;    // grados de inclinacion lateral
// --- Detenido ---
const unsigned long TIEMPO_DETENIDO_MS = 30000;  // 30 s
const float UMBRAL_VEL_DETENIDO = 2.0;           // km/h (GPS)
const float UMBRAL_ACC_DETENIDO = 0.3;           // m/s2 variacion de mag
unsigned long taraTimestamp = 0;

// ================= OBJETOS =================
Adafruit_AHTX0   aht;
Adafruit_BMP280  bmp;
TinyGPSPlus      gps;
BluetoothSerial  SerialBT;
HardwareSerial&  gpsSerial = Serial2;

// ================= ESTADO ==================
bool mpuOK = false, ahtOK = false, bmpOK = false, sdOK = false;
unsigned long ultimaLectura = 0, ultimoIntentoSD = 0, numLectura = 0;
char nombreArchivo[20];

// --- Calibracion del MPU (todo se calcula en la tara) ---
float accScale = 1.0f;                       // correccion de escala: |g| -> 9.81
float gyroBiasX = 0, gyroBiasY = 0, gyroBiasZ = 0;  // bias de giro en reposo
float R[3][3] = {{1,0,0},{0,1,0},{0,0,1}};   // rotacion montaje -> marco nivelado
bool  taraLista = false;

float lastAX = 0, lastAY = 0, lastAZ = 0;    // deteccion de congelado
int   frozenCount = 0;
bool  asiento1 = false, asiento2 = false;

// --- Geocerca ---
double geocercaLat = 0, geocercaLng = 0;
bool   geocercaFijada = false;
bool   geocercaAlerta = false;

// --- Volcadura ---
bool   volcaduraAlerta = false;

// --- Detenido ---
unsigned long tiempoDetenidoInicio = 0;
bool   estaDetenido = false;
bool   detenidoAlerta = false;
float  lastMag = 0;

float ax, ay, az, gx, gy, gz, mag, pitch, roll;
float presion, tempBMP, altitud, tempAHT, humedad;
int   fsr1, fsr2, pasajeros;

// ============================================================
//  Utilidades de matriz 3x3
// ============================================================
void matmul3(const float A[3][3], const float B[3][3], float C[3][3]) {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) {
      C[i][j] = 0;
      for (int k = 0; k < 3; k++) C[i][j] += A[i][k] * B[k][j];
    }
}
void aplicarR(const float M[3][3], float x, float y, float z,
              float &ox, float &oy, float &oz) {
  ox = M[0][0]*x + M[0][1]*y + M[0][2]*z;
  oy = M[1][0]*x + M[1][1]*y + M[1][2]*z;
  oz = M[2][0]*x + M[2][1]*y + M[2][2]*z;
}

// ============================================================
//  MPU6050: driver directo por registros (acepta clones)
// ============================================================
bool mpuEscribir(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool iniciarMPU() {
  Wire.beginTransmission(MPU_ADDR);
  if (Wire.endTransmission() != 0) return false;   // no responde en el bus
  if (!mpuEscribir(REG_PWR_MGMT_1, 0x00)) return false;  // despertar
  delay(50);
  mpuEscribir(REG_CONFIG,    0x03);  // DLPF ~44 Hz (filtra vibracion del RC)
  mpuEscribir(REG_GYRO_CFG,  0x08);  // +/-500 dps
  mpuEscribir(REG_ACCEL_CFG, 0x10);  // +/-8 g
  delay(50);
  return true;
}

// Lee acel + giro crudos y los entrega ya escalados (SIN offsets ni correccion)
bool leerMPUcrudo(float &rax, float &ray, float &raz,
                  float &rgx, float &rgy, float &rgz) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(REG_ACCEL_XOUT);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, 14) != 14) return false;

  int16_t r[7];
  for (int i = 0; i < 7; i++) r[i] = (Wire.read() << 8) | Wire.read();
  // r[0..2]=acel  r[3]=temp(no usada)  r[4..6]=giro
  rax = r[0] * ESCALA_ACC;
  ray = r[1] * ESCALA_ACC;
  raz = r[2] * ESCALA_ACC;
  rgx = r[4] * ESCALA_GYRO;
  rgy = r[5] * ESCALA_GYRO;
  rgz = r[6] * ESCALA_GYRO;
  return true;
}

void leerMPU() {
  float rax, ray, raz, rgx, rgy, rgz;
  bool ok = leerMPUcrudo(rax, ray, raz, rgx, rgy, rgz);

  if (ok) {
    // Aceleracion: solo correccion de escala (sin offsets de gravedad)
    ax = rax * accScale;
    ay = ray * accScale;
    az = raz * accScale;

    // Giroscopio: quitar bias medido en reposo
    gx = rgx - gyroBiasX;
    gy = rgy - gyroBiasY;
    gz = rgz - gyroBiasZ;

    mag = sqrt(ax * ax + ay * ay + az * az);   // ~9.81 en reposo

    // Angulos en el marco nivelado (rotacion de montaje aplicada)
    float cx, cy, cz;
    aplicarR(R, ax, ay, az, cx, cy, cz);
    pitch = atan2(-cx, sqrt(cy * cy + cz * cz)) * RAD_TO_DEG;
    roll  = atan2(cy, cz) * RAD_TO_DEG;
  }

  // Congelado: lectura I2C fallida o valores identicos repetidos
  bool congelado = !ok || (ax == lastAX && ay == lastAY && az == lastAZ);
  lastAX = ax; lastAY = ay; lastAZ = az;

  if (congelado) frozenCount++;
  else           frozenCount = 0;

  if (frozenCount >= 4) {   // ~2 s de datos malos
    Serial.println("[!] MPU congelado -> reiniciando bus I2C y sensor");
    SerialBT.println("[!] MPU congelado -> reiniciando");
    Wire.end();
    delay(50);
    Wire.begin(PIN_SDA, PIN_SCL);
    mpuOK = iniciarMPU();
    frozenCount = 0;
    // Nota: la calibracion (escala/bias/R) se conserva, sigue siendo valida.
  }
}

// ============================================================
//  Tara: mide escala, bias de giro y rotacion de montaje
//  El vehiculo DEBE estar quieto y en su posicion real de montaje.
// ============================================================
void tararNivel() {
  Serial.println("Nivelando... NO MOVER el vehiculo (2 s)");
  double sax = 0, say = 0, saz = 0, sgx = 0, sgy = 0, sgz = 0;
  int validas = 0;

  for (int i = 0; i < MUESTRAS_TARA; i++) {
    float rax, ray, raz, rgx, rgy, rgz;
    if (leerMPUcrudo(rax, ray, raz, rgx, rgy, rgz)) {
      sax += rax; say += ray; saz += raz;
      sgx += rgx; sgy += rgy; sgz += rgz;
      validas++;
    }
    delay(20);
  }

  if (validas == 0) {
    Serial.println("[!] Tara fallida: sin lecturas. Usando valores por defecto.");
    return;
  }

  // 1) Bias del giroscopio = media en reposo
  gyroBiasX = sgx / validas;
  gyroBiasY = sgy / validas;
  gyroBiasZ = sgz / validas;

  // 2) Vector de gravedad medido y correccion de escala
  float gvx = sax / validas, gvy = say / validas, gvz = saz / validas;
  float gmag = sqrt(gvx * gvx + gvy * gvy + gvz * gvz);
  if (gmag < 1.0f) {
    Serial.println("[!] Gravedad invalida en tara. Escala/rotacion por defecto.");
    return;
  }
  accScale = G / gmag;          // corrige el sesgo de escala del clon

  // 3) Rotacion que alinea la gravedad medida (unitaria) con +Z
  //    Rodrigues: R = I + [v]x + [v]x^2 * 1/(1+c)
  float ux = gvx / gmag, uy = gvy / gmag, uz = gvz / gmag;
  float c = uz;                 // u . (0,0,1)

  if (c > 0.9999f) {            // ya nivelado -> identidad
    R[0][0]=1; R[0][1]=0; R[0][2]=0;
    R[1][0]=0; R[1][1]=1; R[1][2]=0;
    R[2][0]=0; R[2][1]=0; R[2][2]=1;
  } else if (c < -0.9999f) {    // sensor invertido -> 180 sobre X
    R[0][0]=1; R[0][1]=0;  R[0][2]=0;
    R[1][0]=0; R[1][1]=-1; R[1][2]=0;
    R[2][0]=0; R[2][1]=0;  R[2][2]=-1;
  } else {
    // v = u x (0,0,1) = (uy, -ux, 0)
    float Vx = uy, Vy = -ux, Vz = 0;
    float K[3][3] = {
      {  0,  -Vz,  Vy },
      { Vz,    0, -Vx },
      {-Vy,   Vx,   0 }
    };
    float K2[3][3];
    matmul3(K, K, K2);
    float k = 1.0f / (1.0f + c);
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        R[i][j] = (i == j ? 1.0f : 0.0f) + K[i][j] + K2[i][j] * k;
  }

  taraLista = true;
  taraTimestamp = millis();
  Serial.printf("[OK] Tara lista | escala:%.4f  |g|:%.2f  gyroBias(%.2f,%.2f,%.2f)  (%d muestras)\n",
                accScale, gmag, gyroBiasX, gyroBiasY, gyroBiasZ, validas);
}

// ============================================================
//  FSR (conteo de pasajeros con histeresis)
// ============================================================
void leerFSR() {
  fsr1 = analogRead(PIN_FSR1);
  fsr2 = analogRead(PIN_FSR2);

  if (!asiento1 && fsr1 > FSR_UMBRAL_OCUPADO) asiento1 = true;
  if ( asiento1 && fsr1 < FSR_UMBRAL_LIBRE)   asiento1 = false;
  if (!asiento2 && fsr2 > FSR_UMBRAL_OCUPADO) asiento2 = true;
  if ( asiento2 && fsr2 < FSR_UMBRAL_LIBRE)   asiento2 = false;

  pasajeros = (asiento1 ? 1 : 0) + (asiento2 ? 1 : 0);
}

// ============================================================
//  microSD
// ============================================================
bool iniciarSD() {
  if (!SD.begin(PIN_SD_CS)) return false;
  for (int i = 1; i < 1000; i++) {
    sprintf(nombreArchivo, "/log_%03d.csv", i);
    if (!SD.exists(nombreArchivo)) break;
  }
  File f = SD.open(nombreArchivo, FILE_WRITE);
  if (!f) return false;
  f.println("ms,lectura,fecha,hora,lat,lng,vel_kmh,sats,"
            "ax,ay,az,mag,gx,gy,gz,pitch,roll,"
            "presion_hPa,temp_bmp_C,alt_m,temp_aht_C,humedad_pct,"
            "fsr1,fsr2,pasajeros,evento,geocerca,volcadura,detenido");
  f.close();
  return true;
}

void registrarSD(bool evento) {
  if (!sdOK) return;
  File f = SD.open(nombreArchivo, FILE_APPEND);
  if (!f) { sdOK = false; return; }

  f.printf("%lu,%lu,", millis(), numLectura);

  if (gps.date.isValid() && gps.time.isValid())
    f.printf("%02d/%02d/%04d,%02d:%02d:%02d,",
             gps.date.day(), gps.date.month(), gps.date.year(),
             gps.time.hour(), gps.time.minute(), gps.time.second());
  else f.print(",,");

  if (gps.location.isValid())
    f.printf("%.6f,%.6f,%.1f,%d,",
             gps.location.lat(), gps.location.lng(),
             gps.speed.kmph(), gps.satellites.value());
  else f.print(",,,0,");

  f.printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.1f,%.1f,",
           ax, ay, az, mag, gx, gy, gz, pitch, roll);
  f.printf("%.2f,%.1f,%.1f,%.1f,%.1f,",
           presion, tempBMP, altitud, tempAHT, humedad);
  f.printf("%d,%d,%d,%s,%s,%s,%s\n",
           fsr1, fsr2, pasajeros,
           evento        ? "IMPACTO"   : "",
           geocercaAlerta ? "GEOCERCA" : "",
           volcaduraAlerta ? "VOLCADURA": "",
           detenidoAlerta  ? "DETENIDO" : "");
  f.close();
}

// ============================================================
//  GPS: alimentar el parser sin bloquear
// ============================================================
void alimentarGPS(unsigned long ms) {
  unsigned long inicio = millis();
  do {
    while (gpsSerial.available()) gps.encode(gpsSerial.read());
  } while (millis() - inicio < ms);
}

// ============================================================
//  Salida por Serial y Bluetooth
// ============================================================
void imprimir(Stream& out, bool evento) {
  out.printf("=== Lectura #%lu | %lu ms ===\n", numLectura, millis());
  if (mpuOK) {
    out.printf("MPU|AX:%f|AY:%f|AZ:%f|Pitch:%f|Roll:%f\n", ax, ay, az, pitch, roll);
  } else out.println("MPU  | FALLA");

  if (bmpOK) out.printf("BMP|%f|%f|%f\n", presion, tempBMP, altitud);
  else       out.println("BMP  | FALLA");

  if (ahtOK) out.printf("AHT|%f|%f\n", tempAHT, humedad);
  else       out.println("AHT  | FALLA");

  if (gps.location.isValid())
    out.printf("GPS|%f\n", gps.speed.isValid() ? gps.speed.kmph() : 0.0f);
  else if (gps.charsProcessed() > 10)
    out.printf("GPS  | Recibiendo NMEA (%lu chars), sin fix aun...\n", gps.charsProcessed());
  else
    out.println("GPS  | Sin datos (revisar cableado TX/RX)");

  out.printf("FSR|%d|%d\n", asiento1, asiento2);

  if (evento) out.printf(">>> EVENTO DE IMPACTO  |mag-g| = %.2f m/s2 <<<\n", fabs(mag - G));
  if (geocercaAlerta)  out.println(">>> ALERTA GEOCERCA: vehiculo fuera del area <<<");
  if (volcaduraAlerta) out.println(">>> ALERTA VOLCADURA detectada <<<");
  if (detenidoAlerta)  out.println(">>> ALERTA DETENIDO: parado mas de 30 s <<<");
  out.println();
}

// ============================================================
//  Geocerca: Haversine en metros
// ============================================================
float distanciaMetros(double lat1, double lng1, double lat2, double lng2) {
  const float R_TIERRA = 6371000.0f;
  float dLat = radians(lat2 - lat1);
  float dLng = radians(lng2 - lng1);
  float a = sin(dLat / 2) * sin(dLat / 2)
          + cos(radians(lat1)) * cos(radians(lat2))
          * sin(dLng / 2) * sin(dLng / 2);
  return R_TIERRA * 2.0f * atan2(sqrt(a), sqrt(1 - a));
}

void verificarGeocerca() {
  if (!gps.location.isValid()) return;

  // Fijar centro la primera vez que hay GPS valido
  if (!geocercaFijada) {
    geocercaLat    = gps.location.lat();
    geocercaLng    = gps.location.lng();
    geocercaFijada = true;
    Serial.printf("[OK] Geocerca fijada: %.6f, %.6f  radio:%.0f m\n",
                  geocercaLat, geocercaLng, GEOCERCA_RADIO_M);
    SerialBT.printf("[OK] Geocerca fijada: %.6f, %.6f  radio:%.0f m\n",
                    geocercaLat, geocercaLng, GEOCERCA_RADIO_M);
    return;
  }

  float dist = distanciaMetros(geocercaLat, geocercaLng,
                               gps.location.lat(), gps.location.lng());
  bool fueraDeCerca = (dist > GEOCERCA_RADIO_M);

  if (fueraDeCerca && !geocercaAlerta) {
    geocercaAlerta = true;
    Serial.printf("[!] GEOCERCA: fuera del radio (%.1f m)\n", dist);
    SerialBT.printf("[!] GEOCERCA: fuera del radio (%.1f m)\n", dist);
  } else if (!fueraDeCerca && geocercaAlerta) {
    geocercaAlerta = false;
    Serial.println("[OK] Geocerca: vehiculo regreso al area.");
    SerialBT.println("[OK] Geocerca: vehiculo regreso al area.");
  }
}

// ============================================================
//  Volcadura: +X apunta arriba del vehiculo
//  En el marco nivelado de la tara, la gravedad queda en +Z.
//  Con Z+=frente y X+=arriba, "roll lateral" es la inclinacion
//  alrededor del eje Z (frente), es decir atan2(cy, cx) sobre
//  el vector de gravedad corregido.
// ============================================================
void verificarVolcadura() {
  if (!mpuOK || !taraLista) return;


  // Esperar 1 s despues de la tara para que ax/ay/az esten estabilizados
  if (millis() - taraTimestamp < 1000) return;

  // mag = 0 significa que aun no hubo lectura valida del MPU
  if (mag < 1.0f) return;

  // +X apunta hacia arriba del vehiculo en el marco del sensor.
  // NO aplicamos R: R sirve para nivelar el marco del loop general,
  // pero aqui queremos el angulo real entre +X y la gravedad medida.
  // En reposo recto: ax ~ 9.81, ay ~ 0, az ~ 0  -> inclinacion ~ 0 deg
  // Si se vuelca: ax cae, ay/az suben             -> inclinacion > umbral
  float inclinacion = acos(constrain(ax / mag, -1.0f, 1.0f)) * RAD_TO_DEG;

  bool volteado = (inclinacion > UMBRAL_VOLCADURA);

  if (volteado && !volcaduraAlerta) {
    volcaduraAlerta = true;
    Serial.printf("[!!!] VOLCADURA detectada | inclinacion: %.1f deg\n", inclinacion);
    SerialBT.printf("[!!!] VOLCADURA detectada | inclinacion: %.1f deg\n", inclinacion);
  } else if (!volteado && volcaduraAlerta) {
    volcaduraAlerta = false;
    Serial.println("[OK] Volcadura: vehiculo nivelado nuevamente.");
    SerialBT.println("[OK] Volcadura: vehiculo nivelado nuevamente.");
  }
}
// ============================================================
//  Detenido: 30 s sin movimiento (GPS + variacion de mag)
// ============================================================
void verificarDetenido() {
  bool sinMovimientoGPS = gps.speed.isValid() && (gps.speed.kmph() < UMBRAL_VEL_DETENIDO);
  bool sinMovimientoACC = mpuOK && (fabs(mag - lastMag) < UMBRAL_ACC_DETENIDO);
  lastMag = mag;

  // Se considera detenido si ambas fuentes lo confirman (o solo ACC si no hay GPS)
  bool quieto = sinMovimientoACC && (gps.speed.isValid() ? sinMovimientoGPS : true);

  if (quieto) {
    if (!estaDetenido) {
      estaDetenido = true;
      tiempoDetenidoInicio = millis();
      detenidoAlerta = false;
    } else if (!detenidoAlerta && (millis() - tiempoDetenidoInicio >= TIEMPO_DETENIDO_MS)) {
      detenidoAlerta = true;
      Serial.printf("[!] DETENIDO: vehiculo parado por mas de %lu s\n",
                    TIEMPO_DETENIDO_MS / 1000);
      SerialBT.printf("[!] DETENIDO: vehiculo parado por mas de %lu s\n",
                      TIEMPO_DETENIDO_MS / 1000);
    }
  } else {
    if (estaDetenido) {
      estaDetenido = false;
      detenidoAlerta = false;
    }
  }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("============================================");
  Serial.println("  CAJA NEGRA - Telemetria RC  (v3)");
  Serial.println("  UPIIZ-IPN | Topicos Avanzados Sensores");
  Serial.println("============================================");

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);
  Serial.println("[OK] Bus I2C  SDA=21  SCL=22  @ 100kHz");

  Serial.print("[..] MPU6050 (0x68, driver directo)... ");
  mpuOK = iniciarMPU();
  Serial.println(mpuOK ? "OK" : "FALLA");

  Serial.print("[..] AHT20 (0x38)... ");
  ahtOK = aht.begin();
  Serial.println(ahtOK ? "OK" : "FALLA");

  Serial.print("[..] BMP280 (0x77)... ");
  bmpOK = bmp.begin(0x77);
  if (!bmpOK) bmpOK = bmp.begin(0x76);
  Serial.println(bmpOK ? "OK" : "FALLA");

  gpsSerial.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
  Serial.println("[..] GPS Neo-6M... UART2 abierto (RX=16, TX=17)");

  Serial.print("[..] Iniciando SD... ");
  sdOK = iniciarSD();
  if (sdOK) Serial.printf("OK -> %s\n", nombreArchivo);
  else      Serial.println("FALLA -> datos solo por BT (reintenta cada 30 s)");

  SerialBT.begin("ESP32_CajaNegra");
  Serial.println("[OK] Bluetooth -> 'ESP32_CajaNegra'");

  analogReadResolution(12);

  if (mpuOK) tararNivel();

  Serial.println("--------------------------------------------");
  Serial.println("Iniciando lectura...\n");
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  alimentarGPS(10);

  if (millis() - ultimaLectura < PERIODO_MS) return;
  ultimaLectura = millis();
  numLectura++;

  if (mpuOK) leerMPU();

  if (bmpOK) {
    presion = bmp.readPressure() / 100.0F;
    tempBMP = bmp.readTemperature();
    altitud = bmp.readAltitude(PRESION_NIVEL_MAR);
  }

  if (ahtOK) {
    sensors_event_t hum, temp;
    aht.getEvent(&hum, &temp);
    tempAHT = temp.temperature;
    humedad = hum.relative_humidity;
  }

  leerFSR();

  bool evento = mpuOK && (fabs(mag - G) > UMBRAL_IMPACTO);

  verificarGeocerca();
  verificarVolcadura();
  verificarDetenido();

  imprimir(Serial, evento);
  imprimir(SerialBT, evento);
  registrarSD(evento);

  if (!sdOK && millis() - ultimoIntentoSD > 30000) {
    ultimoIntentoSD = millis();
    sdOK = iniciarSD();
    if (sdOK) {
      Serial.printf("[OK] SD recuperada -> %s\n", nombreArchivo);
      SerialBT.printf("[OK] SD recuperada -> %s\n", nombreArchivo);
    }
  }
}
