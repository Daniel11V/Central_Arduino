//En Funcionamiento, agregar Infrarrojo y promedio
#include "RF24.h"
RF24 myRadio (2, 5); // in Mega can use> (48, 49);
byte addresses[][6] = {"0"};

// Tiempo
bool base_ms;// V o F si llego a la unidad 
bool base_s;//  ^^^^^^
unsigned long base_ms_prev = millis(); //A cuanto estaba el ciclo anterior la unidad
unsigned long base_ms_now;// valor de la unidad ahora^
int base_s_t1 = 0;        // ^^^^^^
int base_s3 = 0;

// Tanque de Agua
char agua_nivel = 0;
int agua_ms = 0;
char valorAguaAlto;
char valorAguaBajo;

// Distancia Infrarrojo
char pin_Infrarrojo = A3;
float newDist;
int promedioDist = 0;
float listDist[30];

// Luz
char pinLuz = A6;
int valorLuz;
int promedioLuz = 0;
float listLuz[30];

// Temperatura
char pinTemp = A5;
int valorTemp;
int promedioTemp = 0;
float listTemp[30];

// Id
int envios = 0;

// Reloader
int base_luz = 0;
bool enviado = false;
bool estado_led = true;
int iniciar = 0;

void setup() {
  Serial.begin (9600);   // establemos la comucicacion serial

  myRadio.begin();
  myRadio.setChannel(115);
  myRadio.setPALevel(RF24_PA_MAX);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openWritingPipe( addresses[0]);

  pinMode(9, OUTPUT); // Elect. para los 2 sensores de lleno
  pinMode(4, INPUT);  // SensorAgua, lleno
  pinMode(6, INPUT);   // SensorAgua, llenando

  pinMode(10, OUTPUT);  // Pines Luz Reloader
  digitalWrite(10, LOW);
  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);

  pinMode(pin_Infrarrojo, INPUT); 
}

void loop() {
  BaseTiempo();
  Titilar10S();
  //NivelTanque();
  if (base_s) {  //Saque el iniciar
    base_s3++;
    if (base_s3 == 1){
      Infrarrojo();
    } else if (base_s3 == 2){
      Luz();
    } else if (base_s3 == 3) {
      Temperatura();
      base_s3 = 0;
    }
    Enviar();
  }
  TitilarSiempre();
}

void BaseTiempo() {
  base_ms = false;
  base_s = false;
  base_ms_now = millis(); //devuelve el número de milisegundos desde que Arduino se ha reseteado
  if (base_ms_now != base_ms_prev) {
    base_ms = true;
    base_ms_prev = base_ms_now;
    base_s_t1 = base_s_t1 + 1;
    if (base_s_t1 == 1000) {
      base_s = true;
      base_s_t1 = 0;
    }
  }
}

void Titilar10S() {
  if (base_ms && iniciar < 10000) { //titileo cada s los primeros 10s
    iniciar++;
    if (iniciar == 1000) { digitalWrite(10, HIGH); }
    if (iniciar == 2000) { digitalWrite(10, LOW); }
    if (iniciar == 3000) { digitalWrite(3, HIGH); }
    if (iniciar == 4000) { digitalWrite(3, LOW); }
    if (iniciar == 5000) { digitalWrite(10, HIGH); }
    if (iniciar == 6000) { digitalWrite(10, LOW); }
    if (iniciar == 7000) { digitalWrite(3, HIGH); }
    if (iniciar == 8000) { digitalWrite(3, LOW); }
  }
}

void NivelTanque() {   // EN DESHUSO
  if (base_s && agua_nivel == 0) {
    digitalWrite(9, HIGH);// prendemos pin 9 durante 10 microsegundos;
    agua_nivel = 1;
  }
  if (base_ms && agua_nivel == 1) {
    agua_ms++;
    if (agua_ms >= 10) {
      valorAguaAlto = digitalRead(4);
      valorAguaBajo = digitalRead(6);
      digitalWrite(9, LOW);
      agua_ms = 0;
      agua_nivel = 0;
    }
  }
}

void Infrarrojo(){
  newDist= analogRead(pin_Infrarrojo);
  if ((newDist > 111) && (newDist < 1200)) {    // lee solo si el sensa entre 110cm y 18cm
     newDist = 9462/(newDist - 16.92);
     promedioDist = Promedio(newDist, listDist, 20) + 10; // 10cm para conseguir distancia real
     valorAguaBajo = 0;
  } else {
     if (valorAguaBajo < 9) { valorAguaBajo++; }
  }
}

void Luz() {
  valorLuz = analogRead(pinLuz);
  promedioLuz = Promedio(valorLuz, listLuz, 20);
}

void Temperatura() {
  valorTemp = ((analogRead(pinTemp) * 5000L) / 1023) / 10;
  promedioTemp = Promedio(valorTemp, listTemp, 20);
}

int Promedio(int newValue, float list[], byte listCant) {
  //const byte listCant = 20;
  for(int i=listCant-2;i>=0;i--){
    list[i+1]=list[i];
  }
  list[0]= newValue;

  float suma=0;
  for(int i=0;i<listCant;i++){
    suma=suma+list[i];
  }  

  Serial.print(newValue);Serial.print("; [");
  for(int i=0;i<listCant;i++){
    Serial.print(list[i]);
    if(i!=listCant-1){Serial.print(", ");}
  }
  Serial.print("]; ");Serial.println(round(suma/listCant));
  
  return round(suma/listCant);
}

void Enviar() {
  envios++;
  if (envios > 1000) {
    envios = 0;
  }
  struct package
  {
    int id  = envios;
   // int nivelAgua = promedioDist;
    int nivelAguaInf = promedioDist;
    int temperatura = promedioTemp;
    int luz = promedioLuz;
    char aguaLlena = valorAguaAlto;
    char aguaLlenando = valorAguaBajo;
  };
  typedef struct package Package;
  Package data;

  enviado = myRadio.write(&data, sizeof(data));
  if (enviado) {
    Serial.print(" Enviado:");
    Serial.print(newDist);
    Serial.print(", ");
    Serial.print(promedioDist);
    Serial.print(", ");
    Serial.println(promedioTemp);
  } else {
    Serial.print(" No enviado:");
    Serial.print(newDist);
    Serial.print(", ");
    Serial.print(promedioDist);
    Serial.print(", ");
    Serial.print(promedioTemp);
    Serial.print(", ");
    Serial.println(promedioLuz);
  }
//  Serial.print(" Id = ");
//  Serial.println(data.id);
//  Serial.print(" DistanciaInf = ");
//  Serial.println (data.nivelAguaInf);
//  Serial.print(" Luz = ");
//  Serial.println(data.luz);
//  Serial.print(" Temperatura = ");
//  Serial.print (data.temperatura);
//  Serial.println ("C");
//  Serial.print(" Tanque de Agua = ");
//  if (data.aguaLlena == 0) {
//    Serial.print("Lleno, ");
//  } else {
//    Serial.print("No esta lleno, ");
//  }
//  if (data.aguaLlenando == 0) {
//    Serial.println("Llenando");
//  } else {
//    Serial.println("No esta llenando");
//  }
//  Serial.println("------------------------");
}

void TitilarSiempre() {
  if (base_ms && iniciar > 9000) {
    base_luz++;
    if (base_luz >= 500) {
      base_luz = 0;
      if (estado_led) {
        digitalWrite (3, HIGH);
        digitalWrite (10, LOW);
      } else {
        digitalWrite (3, LOW);
        digitalWrite (10, HIGH);
      }
      estado_led = !estado_led;
    }
  }
}
