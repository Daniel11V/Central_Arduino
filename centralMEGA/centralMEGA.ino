//En funcionamiento, corrección reloj
#include <Keypad.h>
#include <LiquidCrystal.h>
// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"
#include "RF24.h" 

// Receptor
RF24 myRadio (11, 12); // in Mega can use> (48, 49); 
byte addresses[][6] = {"0"}; 

// Reloj
RTC_DS1307 rtc;

// Display
LiquidCrystal lcd(53, 49, 47, 45, 43, 41); // Crea un Objeto LC. Parametros: (rs, enable, d4, d5, d6, d7)

// Teclado
const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 4, 3, 2}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// Variables
byte barra[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
};
byte up[8] = {
  B00000,
  B00100,
  B01110,
  B11111,
  B00100,
  B00100,
  B00100,
};
byte down[8] = {
  B00000,
  B00100,
  B00100,
  B00100,
  B11111,
  B01110,
  B00100,
};
byte men1[8] = {
  B01110,
  B01110,
  B00100,
  B01110,
  B10101,
  B00100,
  B01010,
};
byte men2[8] = {
  B01110,
  B01110,
  B00100,
  B11111,
  B00100,
  B00100,
  B01010,
};
byte men3[8] = {
  B01110,
  B01110,
  B10101,
  B01110,
  B00100,
  B00100,
  B01010,
};
unsigned long base_ms_prev = millis();
unsigned long base_ms_now;
bool base_ms;
unsigned long base_mic_prev = micros();
unsigned long base_mic_now;
bool base_mic;
int base_s_t1 = 0;
bool base_s;

String tecla_s;
int tecla_n;
bool tecla_new;

int brillo_ms = 0;
int brillo_paso;

int beep = 0;
bool beepSalida = false;
int base_beep = 0;
int base_beepSalida = 0;

int base_display = 0;
char base_men = 0;

int datos_ms = 0;
int distancia_max;
int distancia_min;

// Variables Menu
char menu = 0;
char menu_h = 0; 
char menu_b = 0;

// Variables Edit Num
String edit = " ";
char editando = 0;
char num_paso[4] = {0,0,0,0};
bool titila = false;

// Variables Edit Hora
char num_pasoH[5] = {0,0,0,0,0};
bool changeHour = true;

// Variables Recibidas
struct package
{
  int id = 0;
//  int nivelAgua = 0;
  int nivelAguaInf = 0;
  int temperatura = 0;
  int luz = 0;
  char aguaLlena = 0;
  char aguaLlenando = 0;
};
typedef struct package Package;
Package data;
int enviosLlego = 0;
int enviosGuardo = 0;
char enviosFallas = 0; 
bool sinConeccion = false;

// Variables Default
char temperatura = 27;
char nivel = 70;
char nivelsonar = 1;
char nivelError = 0;
char nivelMin = 100;
char nivelMax = 0;
char nivelIni = 0;
int nivel_s = 0; //segundos sin cambios en los max y min del nivel
int nivelDur = 0;
bool nivelSubiendo = false;
bool nivelBajando = false;
char nivel_barra = 7;
int hist[9][7] = {};
bool tip_sensor = true; //true=Infrarrojo;false=Luz;
bool noSeLleno = false;

char aguaLlena;
char nivelErrorNano;

bool bomba = true;
char bomba1 = 34;
char bomba2 = 67;
char bomba_init = 12;
char bomba_maxT = 60;

char alerta_paso = 0;

int nuevaHora[5] = {};
char ajustarHora = 0;
char ajustarMin = 0;
char ajustarDia = 0;
char ajustarMes = 0;
int ajustarAnio = 0;
 
void setup() {   
  Serial.begin(9600);
  pinMode(39, OUTPUT);
  pinMode(37, OUTPUT);
  pinMode(33, OUTPUT);
  if (! rtc.begin()) {
//    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
//    Serial.println("RTC is NOT running!");
//    rtc.adjust(DateTime(F(__DATE__), F(__ TIME__)));
//    rtc.adjust(DateTime(2018, 2, 11, 17, 13, 5));
  }
//  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//  rtc.adjust(DateTime(Anio, Mes, Dia, Hora, Min, Seg));
//  rtc.adjust(DateTime(2018, 10, 21, 17, 18, 00));
  
  myRadio.begin(); 
  myRadio.setChannel(115); 
  myRadio.setPALevel(RF24_PA_MAX);
  myRadio.setDataRate( RF24_250KBPS ) ; 
  myRadio.setRetries(5,0);
  myRadio.openReadingPipe(1, addresses[0]);
  myRadio.startListening();
  
  lcd.createChar(0, barra);
  lcd.createChar(1, up);
  lcd.createChar(2, down);
  lcd.createChar(3, men1);
  lcd.createChar(4, men2);
  lcd.createChar(5, men3);
  lcd.begin(16,2); 
  delay(2000);
}

void loop() { 
  base_tiempo();
  Teclado();
  Brillo();
  Beep();
  Receptor();
  Display();
  if (tecla_new) {
    ajustarTeclas();
  }
}


void base_tiempo() {
  base_mic = false;
  base_ms = false;
  base_s = false;
  base_ms_now = millis();
  base_mic_now = micros();
  if (base_mic_now > base_mic_prev + 10) {
    base_mic = true;
    base_mic_prev = base_mic_now;
  }
  if (base_ms_now > base_ms_prev + 10) {
    base_ms = true;
    base_ms_prev = base_ms_now;
    base_s_t1 = base_s_t1 + 10;
    if (base_s_t1 > 1000) {
      base_s = true;
      base_s_t1 = 0;
    }
  }
}

void Teclado() {
  char key = keypad.getKey();
  tecla_new = false;
  if (key) {
    tecla_s = String(key);
    tecla_n = tecla_s.toInt();
    if (tecla_s != "0" && tecla_n == 0){ tecla_n = 99;}
    tecla_new = true;
  }
}

void Brillo() {
  if (tecla_new) {
    digitalWrite(39, HIGH);
    brillo_paso = 1;
    brillo_ms = 0;
  }
  if (brillo_paso == 1 && base_ms && editando == 0){
    brillo_ms++;
    if (brillo_ms == 10000){
      digitalWrite(39, LOW);
      brillo_paso = 0;
      brillo_ms = 0;
      menu = 0;
      menu_b = 0;
      menu_h = 0;
    }
  }
}

void Beep() {  
  if (base_ms) {base_beep++;}
  
  // Beep 1
  if (beep == 1) {beep = 101;base_beep = 0;beepSalida = true;}
  if (beep == 101 && base_beep > 10) {beepSalida = false;beep = 0;}

  // Beep 2
  if (beep == 2) {beep = 201;base_beep = 0;beepSalida = true;}
  if (beep == 201 && base_beep > 10) {beepSalida = false;beep = 202;base_beep = 0;}
  if (beep == 202 && base_beep > 5) {beepSalida = true;beep = 203;base_beep = 0;}
  if (beep == 203 && base_beep > 10) {beepSalida = false;beep = 0;}

  // Beep 3
  if (beep == 3) {beep = 301;base_beep = 0;beepSalida = true;}
  if (beep == 301 && base_beep > 30) {beepSalida = false;beep = 0;}

  // Beep 4
  if (beep == 4) {beep = 401;base_beep = 0;beepSalida = true;}
  if (beep == 401 && base_beep > 50) {beepSalida = false;beep = 402;base_beep = 0;}
  if (beep == 402 && base_beep > 200) {beepSalida = true;beep = 4;base_beep =0;}
  
  if (beepSalida) {
    digitalWrite(37, HIGH);
  } else {
    digitalWrite(37, LOW);
  }
}

void Receptor() {
  if (base_s) {
    enviosGuardo = enviosLlego;
    nivel_s++;
    nivelDur++;
  }
  
  if ( myRadio.available()) {
    myRadio.read( &data, sizeof(data) ); 
    if (data.nivelAguaInf > 25 && data.nivelAguaInf < 100) {
      nivel = (1000 - float (data.nivelAguaInf) * 10) / ((100-25)/10); 
      nivelError = 0; 
    } else if (nivelError < 9) { nivelError++; }
    if (nivel == 99) {nivel_barra = 10;} else {nivel_barra = nivel / 10;}
    
    /////////////
    aguaLlena = data.aguaLlena;
    nivelErrorNano = data.aguaLlenando;
    enviosGuardo = enviosLlego;
    enviosLlego = data.id;
    ///////////////////////////
    
    DateTime now = rtc.now();
    if (nivel > 40) {nivelsonar = 1;}
    if (nivel <= 30 && nivelsonar == 1) {
      if(now.hour() > 7 && now.hour() < 23) { beep = 4;}
      nivelsonar = 0;
    }

    if (nivel > 70 || now.hour() == 1) { digitalWrite(33, LOW); noSeLleno = false;}
    if (nivel < 60 && now.hour() > 5 && now.hour() < 9 && !noSeLleno) {digitalWrite(33, HIGH); noSeLleno = true;} 
    if (nivelMin > nivel) {nivelMin = nivel; nivel_s = 0;}
    if (nivelMax < nivel) {nivelMax = nivel; nivel_s = 0;}

    //Comienza a subir
    if (nivelMin + 3 <= nivel && !nivelSubiendo) {
      nivel_s = 0;
      nivelSubiendo = true;
      nivelBajando = false;
      nivelIni = nivelMin;
      nivelDur = 0;
    }

    //Comienza bajar 
    if (nivelMax - 3 >= nivel && !nivelBajando) {
      nivel_s = 0;
      nivelBajando = true;
      if (nivelSubiendo) {nuevoHistorial();}
      nivelSubiendo = false;
    }

    //Queda estable, deja de subir y bajar
    if (nivel_s >= 600) {
      nivel_s = 0;
      if (nivelSubiendo) {
        nivelSubiendo = false;
        nuevoHistorial();
      } else if (nivelBajando) {
        nivelBajando = false;
      }
    }
    //////////////////////////

    if (base_s && enviosGuardo == enviosLlego) {
      if (enviosFallas >= 5) {
        sinConeccion = true;
      } else {
        enviosFallas++;
      }
    } else {
      enviosFallas = 0;
      sinConeccion = false;
    }
  }
}

void nuevoHistorial() {
//{dia, mes, hora, min, nivelIni, nivelFin, duracion(min)}
  DateTime now = rtc.now();
  for (char i=9; i>0; i--) {
    hist[i][0] = hist[i-1][0];
    hist[i][1] = hist[i-1][1];
    hist[i][2] = hist[i-1][2];
    hist[i][3] = hist[i-1][3];
    hist[i][4] = hist[i-1][4];
    hist[i][5] = hist[i-1][5];
    hist[i][6] = hist[i-1][6];
  }
  hist[0][0] = now.day();
  hist[0][1] = now.month();
  hist[0][2] = now.hour();
  hist[0][3] = now.minute();
  hist[0][4] = nivelIni;
  hist[0][5] = nivel;
  hist[0][6] = nivelDur / 60;

//  Serial.print("[");
//  Serial.print(now.day());
//  Serial.print(", ");
//  Serial.print(now.month());
//  Serial.print(", ");
//  Serial.print(now.hour());
//  Serial.print(", ");
//  Serial.print(now.minute());
//  Serial.print(", ");
//  Serial.print(nivelIni, DEC);
//  Serial.print(", ");
//  Serial.print(nivel, DEC);
//  Serial.print(", ");
//  Serial.print(nivelDur / 60, DEC);
//  Serial.print("]");
//  Serial.println();
  
  nivelIni = 0;
  nivelDur = 0;
  nivelMin = nivel;
  nivelMax = nivel;
}

void ajustarTeclas() {
  noSeLleno = false;
  
  if (beep == 4 || beep == 401 || beep == 402) {
    beep = 0;
    beepSalida = false;
  } else if (tecla_s == "*") {
    beep = 3;  // beep largo
  } else if (tecla_s == "#" && editando != 0) {
    beep = 2;  // 2 beep
  } else {
    beep = 1;  // 1 beep
  } 
  
  if (tecla_s == "*" && editando == 0){ 
    menu = 0;
    menu_b = 0;
    menu_h = 0;
  } else if (tecla_s == "A") {
    if (menu == 0) {menu = 1;} else if (menu == 1) {menu = 0;}
  } else if (tecla_s == "1") {
    if (menu == 1){ menu = 11;}
  } else if (tecla_s == "2") {
    if (menu == 1){ menu = 12; tecla_s = "";}
  } else if (tecla_s == "3") {
    if (menu == 1){ menu = 13; tecla_s = "";}
  } else if (tecla_s == "4") {
    if (menu == 1){ 
      DateTime now = rtc.now();
      ajustarMin = now.minute();
      ajustarHora = now.hour();
      ajustarDia = now.day();
      ajustarMes = now.month();
      ajustarAnio = now.year();
      menu = 14;
      editando = 1; //(id(0) + 1)
      num_pasoH[0] = 1;
      Serial.print("Menu14");
//      Serial.print(ajustarAnio, DEC);
//      Serial.println();
    }
  }else if (menu == 11) {
    if (tecla_s == "B"){
      if (menu_h == 0){ 
        menu_h = 8; 
      } else { menu_h = menu_h - 1; }
    }
    if (tecla_s == "C"){
      if (menu_h == 8){ 
        menu_h = 0; 
      } else { menu_h = menu_h + 1; }
    }
  } else if (menu == 12) {
    if (menu_b == 0) {
      if (tecla_s == "5" && editando == 0) { bomba=!bomba;}
    }
    if (tecla_s == "C" && editando == 0){ menu_b = 1;}
    if (tecla_s == "B" && editando == 0){ menu_b = 0;}
  } else if (menu == 13 && tecla_s == "5") { tip_sensor=!tip_sensor;}
}

void Display() {
  if (base_ms) {base_display++;}
  if (base_display > 20 || tecla_new) {
    base_display = 0;
    lcd.clear();
    if (menu == 0) { Menu0();}
    if (menu == 1) { Menu1();}
    if (menu == 11){ Menu11();}
    if (menu == 12){ Menu12();}
    if (menu == 13){ Menu13();}
    if (menu == 14){ Menu14();}
  }
}

void Menu0() {
  DateTime now = rtc.now();
  if (now.hour()==19 && now.second()>55 && changeHour){
      changeHour=false;
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), 05));
  } else if (now.hour()==18){
      changeHour=true;
  }
 
  if (now.day() == 165) {
    lcd.print("00/00 00:00");
  } else {
    if (now.day()<10){lcd.print("0");}
    lcd.print(now.day(), DEC);
    lcd.print('/');
    if (now.month()<10){lcd.print("0");}
    lcd.print(now.month(), DEC);
    lcd.print(' ');
    if (now.hour()<10){lcd.print("0");}
    lcd.print(now.hour(), DEC);
    lcd.print(':');
    if (now.minute()<10){lcd.print("0");}
    lcd.print(now.minute(), DEC);
  }
  
  if (sinConeccion) {
    lcd.setCursor(13,0);
    lcd.print("***");
  } else {
    if (tip_sensor){
      if ( data.temperatura < 10) {
        lcd.setCursor(14,0);
      } else {
        lcd.setCursor(13,0);
      }
      lcd.print(data.temperatura, DEC);
      lcd.print("C");
    } else {
      if ( data.luz >= 100) {
        lcd.setCursor(12,0);
      } else if (data.luz >= 10 ) {
        lcd.setCursor(13,0);
      } else if (data.luz >= 0) {
        lcd.setCursor(14,0);
      } else if (data.luz > -10) {
        lcd.setCursor(13,0);
      } else if (data.luz > -100) {
        lcd.setCursor(12,0);
      } else {
        lcd.setCursor(11,0);
      }
      lcd.print(data.luz, DEC);
      lcd.print("L");
    }
  }

  AguaNivel(nivel, nivelSubiendo, nivelBajando, nivel_barra);
  //lcd.print("N");
  if(!nivelSubiendo && !nivelBajando){
    lcd.write(byte(5));
  } else {
    if (base_men == 0){lcd.write(byte(3));} 
    if (base_men == 1){lcd.write(byte(4));}
    if (base_men == 2){lcd.write(byte(5)); base_men = 0;} else {base_men++;}
  }
}

void AguaNivel(char nivelR, bool nivelSubiendoR, bool nivelBajandoR, char nivel_barraR) {
  if ( nivelR < 10) {
    lcd.setCursor(1,1);
  } else {
    lcd.setCursor(0,1);
  }
  lcd.print(nivelR, DEC);
  lcd.print("%[");
  for (char i = 0; i < nivel_barraR; i++) {
    if (sinConeccion) {
      lcd.write('-');
    } else if (nivelSubiendoR) {
      lcd.write('>');
    } else if (nivelBajandoR) {
      lcd.write('<');
    } else {
      lcd.write(byte(0));
    }
  }
  lcd.setCursor(12,1);
  if (nivelError > 0) { lcd.print(nivelError, DEC); }
  lcd.setCursor(13,1);
  if (nivelErrorNano > 0) { lcd.print(nivelErrorNano, DEC); }
  lcd.setCursor(14,1);
  lcd.print("]");
}

void Menu1() {
  lcd.print("1=Nivel 3=Sensor");
  lcd.setCursor(0,1);
  lcd.print("2=Bomba 4=Reloj");
}

void Menu11() {
  lcd.print(menu_h + 1, DEC);
  lcd.print(":");
  if (hist[menu_h][0]<10){lcd.print("0");}
  lcd.print(hist[menu_h][0], DEC);
  lcd.print("/");
  if (hist[menu_h][1]<10){lcd.print("0");}
  lcd.print(hist[menu_h][1], DEC);
  lcd.print(" ");
  if (hist[menu_h][2]<10){lcd.print("0");}
  lcd.print(hist[menu_h][2], DEC);
  lcd.print(":");
  if (hist[menu_h][3]<10){lcd.print("0");}
  lcd.print(hist[menu_h][3], DEC);
  lcd.print(" B");
  lcd.write(byte(1));
  lcd.setCursor(0,1);
  if (hist[menu_h][4]<10){lcd.print("0");}
  lcd.print(hist[menu_h][4], DEC);
  lcd.print("%>");
  if (hist[menu_h][5]<10){lcd.print("0");}
  lcd.print(hist[menu_h][5], DEC);
  lcd.print("% ");
  if (hist[menu_h][6]<100){lcd.print("0");}
  if (hist[menu_h][6]<10){lcd.print("0");}
  lcd.print((hist[menu_h][6]), DEC);
  lcd.print("mn C");
  lcd.write(byte(2));
}

void Menu12() {
  if (menu_b == 0) {
    lcd.print("Bomba: 5=");
    if (bomba) {
      lcd.print("AUTO");
    } else {
      lcd.print("APAGADO");
    }
    lcd.setCursor(0,1);
    lcd.print("1=");
    bomba1 = editarNum(0, "1", bomba1);
    lcd.setCursor(4,1);
    lcd.print("% 2=");
    bomba2 = editarNum(1, "2", bomba2);
    lcd.setCursor(10,1);
    lcd.print("%   C");
    lcd.write(byte(2));
  } else if (menu_b == 1){
    lcd.print("1=");
    bomba_init = editarNum(2, "1", bomba_init);
    lcd.setCursor(4,0);
    lcd.print("h (init.) B");
    lcd.write(byte(1));
    lcd.setCursor(0,1);
    lcd.print("2=");
    bomba_maxT = editarNum(3, "2", bomba_maxT);
    lcd.setCursor(4,1);
    lcd.print("mn(max t)");
  }
}

int editarNum(char id, String tecla, char valor) {
  if (tecla_new) {
    if (num_paso[id] == 1 && tecla_n != 99 && edit.toInt() < 10) {
      if (edit == " "){
        edit = tecla_s;
      } else {
        edit = edit + tecla_s;
      }
    }
    if (tecla_s == tecla && editando == 0) {
      editando = id + 1;
      num_paso[id] = 1;
    }
    if (tecla_s == "#" && num_paso[id] == 1) {
      num_paso[id] = 0;
      editando = 0;
      if (edit != " "){
        if (id == 2) {
          if (edit.toInt() <= 24) {
            valor = edit.toInt();
          }
        } else {
          valor = edit.toInt();
        }
      }
      edit = " ";
    }
    if (tecla_s == "*" && num_paso[id] == 1) {
      num_paso[id] = 0;
      edit = " ";
      editando = 0;
    }
  }
  
  if (editando == id + 1){
    titila = !titila;
    if (edit.toInt() < 10) {lcd.print(" ");}
    if (titila) {
      if (edit == " ") {
        lcd.write(byte(0));
      } else {
        lcd.print(edit);
      }
    }
  } else {
    if (valor < 10) {lcd.print(" ");}
    lcd.print(valor, DEC);
  }
  return valor;
}

void Menu13() {
  lcd.print("TIPO SENSOR");
  lcd.setCursor(0,1);
  lcd.print(" 5=");
  if (tip_sensor) {
    lcd.print("TEMPERATURA");
  } else {
    lcd.print("CANT.DE LUZ");
  }
}

void Menu14() {
  DateTime now = rtc.now();
  lcd.print("     RELOJ     ");
  lcd.setCursor(0,1);
  ajustarHora = editarNum2(0,ajustarHora);
  lcd.setCursor(2,1);
  lcd.print(":");
  ajustarMin = editarNum2(1,ajustarMin);
  lcd.setCursor(5,1);
  lcd.print(" ");
  ajustarDia = editarNum2(2,ajustarDia);
  lcd.setCursor(8,1);
  lcd.print("/");
  ajustarMes = editarNum2(3,ajustarMes);
  lcd.setCursor(11,1);
  lcd.print("/");
  ajustarAnio = editarNum2(4,ajustarAnio);
//  rtc.adjust(DateTime(ajustarAnio, ajustarMes, ajustarDia, ajustarHora, ajustarMin, 00));
}

int editarNum2(char id, int valor) {
//  Serial.print("Valor");
//  Serial.print(valor, DEC);
//  Serial.println();
  if (tecla_new) {
    if (num_pasoH[id] == 1 && tecla_n != 99 && edit.toInt() < 10 || id == 4 && num_pasoH[4] == 1 && tecla_n != 99 && edit.toInt() < 1000) {
      if (edit == " "){
        edit = tecla_s;
      } else {
        edit = edit + tecla_s;
      }
    }
//    if (tecla_s == tecla && editando == 0) {
//      editando = id + 1;
//      num_pasoH[id] = 1;
//    }
    if (tecla_s == "#" && num_pasoH[id] == 1) {
      tecla_new = false;
      if (id == 0 && edit.toInt() > 24) {
      } else if (id == 1 && edit.toInt() > 59) {
      } else if (id == 2 && edit.toInt() > 31) {
      } else if (id == 3 && edit.toInt() > 12) {
      } else if (id == 4 && edit.toInt() < 2000 && edit.toInt() != 0) {
      } else {
        num_pasoH[id] = 0;
        if (id == 4) {
          editando = 0;
          rtc.adjust(DateTime(ajustarAnio, ajustarMes, ajustarDia, ajustarHora, ajustarMin, 00));
          menu = 0;
        } else {
          editando = editando + 1;
          num_pasoH[id + 1] = 1;
        }
        if (edit != " "){
          valor = edit.toInt();
        }  
      }
      edit = " ";
    }
    if (tecla_s == "*" && num_pasoH[id] == 1) {
      num_pasoH[id] = 0;
      edit = " ";
      editando = 0;
    }
  }

  if (editando == id + 1) {
    titila = !titila;
    if (titila) {
      if (edit == " ") {
        if (valor < 10) {lcd.print("0");}
        lcd.print(valor, DEC);
      } else {
        if (editando == 5) {
          if (edit.toInt() < 1000) {lcd.print(" ");}
          if (edit.toInt() < 100) {lcd.print(" ");}
        }
        if (edit.toInt() < 10) {lcd.print(" ");}
        lcd.print(edit);
      }
    }
  } else {
    if (valor < 10) {lcd.print("0");}
    lcd.print(valor, DEC);
  }
  Serial.print(edit);
  Serial.println();
  return valor;
}
