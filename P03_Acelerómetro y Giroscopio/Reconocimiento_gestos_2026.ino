#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu(0x68); //Revisar la dirección de su módulo

int pitch=0;
int roll=0;
unsigned long past_time=0;


///////////////////////////////// Variables Red Neuronal ////////////////
const int node = 2;
const int input = 2;
const int outputs = 2;

float P[input][1];
// Modificar pesos y bias, utilziar las del entrenamiento
float W1[node][input]={{0.205957907259937,3.125521054443873},
                      {4.585458674134641,1.612950175426701}};
   
float b1[node][1]={{1.826452100350065},
                   {-0.035852728585982}};

float a1[node][1];

float W2[outputs][node]={{1.651645871718739,4.117220171354527},
                        { 3.791638591292258,-3.662819741133767}};


float b2[outputs][1]={{0.627826375854322},
                      {-1.889927226295911}};
                      
float a2[outputs][1];

float aux=0.0;

int maxValue = 1023;
int minValue = 0;


void setup()
{
  Serial.begin(115200);
  
  /*--Start I2C interface--*/
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin(); // Seleccionar los GPIO adecuados (ESP32)
  #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
    Fastwire::setup(400, true);
  #endif
  mpu.initialize();
  
}

void loop() {

  /////////////////////////////// Primera Capa Entrada ///////////////////
  int16_t ax, ay, az;

  mpu.getAcceleration(&ax, &ay, &az);
  float x = ax / 16384.0;
  float y = ay / 16384.0;
  float z = az / 16384.0;
  
  pitch = -(atan2(x, sqrt(y*y + z*z)) * 180.0) / M_PI;
  roll = (atan2(y, z) * 180.0) / M_PI;

  int mappedPitch = map(pitch, -180, 180, 0, 1023);
  int mappedRoll = map(roll, -180, 180, 0, 1023);

  
  P[0][0]=dataNormalized((float) mappedPitch,minValue,maxValue);
  P[1][0]=dataNormalized((float) mappedRoll,minValue,maxValue);

  ///////////////////////////// Segunda Capa //////////////////////////
  
  for(int i = 0 ; i<node; i++ ) {  
    aux=0.0;  
    for(int j = 0 ; j < input ; j++ ) { 
     aux=aux+W1[i][j]*P[j][0];
    }
    a1[i][0]=tansig(aux+b1[i][0]);
  }

    ///////////////////////////// Tercera Capa //////////////////////////
  
    for(int i = 0 ; i<outputs; i++ ) {  
    aux=0.0;  
    for(int j = 0 ; j < node ; j++ ) { 
     aux=aux+W2[i][j]*a1[j][0];
    }
    a2[i][0]=tansig(aux+b2[i][0]);
    }
    // Modificar etiquetas de acuerdo a la codificación
    if (a2[0][0]<=0 && a2[1][0]<=0){
      Serial.println("Izquierda");
      }
      else if(a2[0][0]<=0 && a2[1][0]>0){
        Serial.println("Derecha");
        } 
       else if(a2[0][0]>0 && a2[1][0]<=0){
        Serial.println("Adelante");
        }
        else
        {
          Serial.println("Atras");
          }
Serial.print(pitch); Serial.print(" ");
Serial.print(roll); Serial.print(" ");
// Esto dibujará dos líneas: una para Pitch y otra para Rolls
    delay(350);
  }

    
float tansig(float n)
{
  float a = exp(n);
  float b = exp(-n);
  return (a-b)/(a+b);
}

float dataNormalized(int inputData, int minData, int maxData) 
{
  float valueNorm;
  valueNorm = 2.0*(inputData-minData)/(maxData-minData)-1.0;
  return valueNorm;
}
