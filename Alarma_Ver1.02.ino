  
/* Julio 06 de 2018
implemantacion puerto para entradas*/


#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include "Gsender.h"
#include <EEPROM.h>
#include <Separador.h>
#include <Wire.h> 
#include <RtcDS3231.h>
RtcDS3231<TwoWire> Rtc(Wire);
WebSocketsServer webSocket = WebSocketsServer(81);
RtcDateTime now;
RtcDateTime alarmTime;
int estado;
uint8_t num;
int pulsador1;
int pulsador2;
int pulsador3;
int val = 0;
int val_p = 0;
byte conf = B00000111;
byte conf1 =B00000101;
byte puerto=0;
boolean Zona1=false;
boolean Zona2=false;
boolean Zona3=false;
boolean ALARMA=false;
boolean bandera= true;
boolean bandera1= true;
boolean bandera2= true;
boolean bandera3= true;
boolean Alarma1=false;
boolean FueActivada=false;

#define Pin_Alarma 15
#define Config 16
#define Sensor_Z1 14
#define Sensor_Z2 0
#define Sensor_Z3 2

//-------------------VARIABLES GLOBALES--------------------------
int contconexion = 0;
unsigned long previousMillis = 0;
char ssid[20];      
char pass[20];
char dir_ip[20];
char geteway[20];
char opcion[20];
const char *ssidConf = "Configuracion";
const char *passConf = "12345678";
int dir[8];
String mensaje = "";
//-------------------------IMPRIME HORA FECHA----------------------------
#define countof(a) (sizeof(a) / sizeof(a[0]))
void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}
void enviar(int dato)
  {
    for (int i=0; i <=7; i++){
      if( bitRead(dato, i))
        {
        if (bitRead(conf, i) && bitRead(conf1, i))
        {  
        Serial.print("ALARMA Zona:");
        Serial.println(i,DEC);
        }
        if (bitRead(conf, i) && !bitRead(conf1, i))
        {  
        Serial.print("ALARMA SILENCIOSA Zona:");
        Serial.println(i,DEC);
        }
        }
    }
  } 
//-------------------------ALARMA----------------------------
void Alarma(boolean Activador){
  FueActivada = Activador;
 if (FueActivada){
  digitalWrite(Pin_Alarma,HIGH);
  webSocket.sendTXT(num,"AA",2);
  now = Rtc.GetDateTime();
  alarmTime = now + 45; // tiempo que dura la sirena sonando en segundos
 }
 else {
  digitalWrite(Pin_Alarma,LOW);
  webSocket.sendTXT(num, "AD", 2);
 }
} 
  
  
//-----------CODIGO HTML PAGINA DE CONFIGURACION---------------
String pagina = "<!DOCTYPE html>"
"<html>"
"<head>"
"<title>Pagina de Configuracion</title>"
"<meta charset='UTF-8'>"
"</head>"
"<body>"
"</form>"
"<form action='guardar_conf' method='get'>"
"SSID:<br><br>"
"<input class='input1' name='ssid' type='text'><br>"
"PASSWORD:<br><br>"
"<input class='input1' name='pass' type='password'><br><br>"
"DIR_IP:<br><br>"
"<input class='input1' name='dir_ip' type='text'><br><br>"
"GETEWAY:<br><br>"
"<input class='input1' name='gateway' type='text'><br><br>"
"<input type='radio' name='opcion' value='activado'> Activado<br>"
"<input type='radio' name='opcion' value='atencion'> Atencion<br>"
"<input type='radio' name='opcion' value='desactivado'> Desactivado<br>" 
"<input class='boton' type='submit' value='GUARDAR'/><br><br>" 
"</form>"
"<a href='escanear'><button class='boton'>ESCANEAR</button></a><br><br>";

String paginafin = "</body>"
"</html>";

//------------------------SETUP WIFI-----------------------------
void setup_wifi() {
  pinMode(Sensor_Z1,INPUT);
  pinMode(Sensor_Z2,INPUT);
  pinMode(Sensor_Z3,INPUT);
  pinMode(Pin_Alarma,OUTPUT);
  pinMode(Config,OUTPUT);
  
// If you don't want to config IP manually disable the next four lines
  IPAddress ip(dir[0], dir[1], dir[2], dir[3]);            // where 150 is the desired IP Address   
  IPAddress gateway(dir[4], dir[5], dir[6], dir[7]);         // set gateway to match your network
  IPAddress subnet(255, 255, 255, 0);        // set subnet mask to match your network
  WiFi.config(ip, gateway, subnet);          // If you don't want to config IP manually disable this line
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.println(".");
  while(WiFi.status() != WL_CONNECTED and contconexion <50 ) {
    ++contconexion;
    delay(250);
    Serial.print(".");
    digitalWrite(Config, HIGH);
    delay(250);
    digitalWrite(Config, LOW);
  }
  if (contconexion <50) {   
      Serial.println("");
      Serial.println("WiFi conectado");
      Serial.println(WiFi.localIP());
      digitalWrite(Config, HIGH);  
  }
  else { 
      Serial.println("");
      Serial.println("Error de conexion");
      digitalWrite(Config, LOW);
  }
  Serial.println(WiFi.localIP());
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}
  

//--------------------------------------------------------------
WiFiClient espClient;
ESP8266WebServer server(80);
//--------------------------------------------------------------

//-------------------PAGINA DE CONFIGURACION--------------------
void paginaconf() {
  server.send(200, "text/html", pagina + mensaje + paginafin); 
}

//--------------------MODO_CONFIGURACION------------------------
void modoconf() {
   
  delay(100);
  digitalWrite(Config, HIGH);
  delay(100);
  digitalWrite(Config, LOW);
  delay(100);
  digitalWrite(Config, HIGH);
  delay(100);
  digitalWrite(Config, LOW);

  WiFi.softAP(ssidConf, passConf);
  IPAddress myIP = WiFi.softAPIP(); 
  Serial.print("IP del acces point: ");
  Serial.println(myIP);
  Serial.println("WebServer iniciado...");

  server.on("/", paginaconf); //esta es la pagina de configuracion

  server.on("/guardar_conf", guardar_conf); //Graba en la eeprom la configuracion

  server.on("/escanear", escanear); //Escanean las redes wifi disponibles
  
  server.begin();

  while (true) {
      server.handleClient();
  }
}
//---------------------GUARDAR CONFIGURACION-------------------------
void guardar_conf() {
Separador s;
  Serial.println(server.arg("ssid"));//Recibimos los valores que envia por GET el formulario web
  grabar(0,server.arg("ssid"));
  Serial.println(server.arg("pass"));
  grabar(20,server.arg("pass"));
  Serial.println(server.arg("dir_ip"));
  Serial.println(server.arg("gateway"));
  for(byte i=0;i<4; i++){
    EEPROM.write(40+i,(s.separa(server.arg("dir_ip"),'.',i)).toInt());
  }
  for(byte i=0;i<4; i++){
    EEPROM.write(44+i,(s.separa(server.arg("gateway"),'.',i)).toInt());
  }
  //Memorias Zona1
  EEPROM.write(50,0); //memoria de Activado
  EEPROM.write(51,0); //memoria de Atencion
  EEPROM.write(52,1); //memoria de desactivado
  //Memorias Zona2
  EEPROM.write(53,0); //memoria de Activado
  EEPROM.write(54,0); //memoria de Atencion
  EEPROM.write(55,1); //memoria de desactivado 
  //Memorias Zona3
  EEPROM.write(56,0); //memoria de Activado
  EEPROM.write(57,0); //memoria de Atencion
  EEPROM.write(58,1); //memoria de desactivado
  EEPROM.write(59,0); //memoria de Activado
  EEPROM.commit();
  
  mensaje = "Configuracion Guardada...";
  paginaconf();
}

//----------------Función para grabar en la EEPROM-------------------
void grabar(int addr, String a) {
  int tamano = a.length(); 
  char inchar[20]; 
  a.toCharArray(inchar, tamano+1);
  for (int i = 0; i < tamano; i++) {
    EEPROM.write(addr+i, inchar[i]);
  }
  for (int i = tamano; i < 20; i++) {
    EEPROM.write(addr+i, 255);
  }
  EEPROM.commit();
}

//-----------------Función para leer la EEPROM------------------------
String leer(int addr) {
   byte lectura;
   String strlectura;
   for (int i = addr; i < addr+20; i++) {
      lectura = EEPROM.read(i);
      if (lectura != 255) {
        strlectura += (char)lectura;
      }
   }
   return strlectura;
}
//---------------------------ESCANEAR----------------------------
void escanear() {  
  int n = WiFi.scanNetworks(); //devuelve el número de redes encontradas
  Serial.println("escaneo terminado");
  if (n == 0) { //si no encuentra ninguna red
    Serial.println("no se encontraron redes");
    mensaje = "no se encontraron redes";
  }  
  else
  {
    Serial.print(n);
    Serial.println(" redes encontradas");
    mensaje = "";
    for (int i = 0; i < n; ++i)
    {
      // agrega al STRING "mensaje" la información de las redes encontradas 
      mensaje = (mensaje) + "<p>" + String(i + 1) + ": " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ") Ch: " + WiFi.channel(i) + " Enc: " + WiFi.encryptionType(i) + " </p>\r\n";
      //WiFi.encryptionType 5:WEP 2:WPA/PSK 4:WPA2/PSK 7:open network 8:WPA/WPA2/PSK
      delay(10);
    }
    Serial.println(mensaje);
    paginaconf();
  }
}

//---------------------------Funcion WEBSOCKET----------------------------

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
  
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("Disconneted");
      Serial.printf("[%u] Disconnected!\n", num);  
      break;
      
    case WStype_CONNECTED:{
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connectado com: %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      webSocket.sendTXT(num, "CN",2);
      estado = digitalRead(Pin_Alarma);   //lee el estado del de la Alarma
      if(estado==LOW)webSocket.sendTXT(num, "AD", 2);  
      else webSocket.sendTXT(num, "AA", 2);
      
      if (Alarma1)webSocket.sendTXT(num, "AA1", 3);
      else webSocket.sendTXT(num, "AD1", 3);
      
      if(EEPROM.read(59)){      //memoria de SISTEMA ACTIVADO
        webSocket.sendTXT(num, "1A", 2);
        webSocket.sendTXT(num, "2A", 2);
        webSocket.sendTXT(num, "3A", 2);
      }
      else{
        if (EEPROM.read(50))webSocket.sendTXT(num, "1A", 2);
        if (EEPROM.read(51))webSocket.sendTXT(num, "1T", 2);
        if (EEPROM.read(52))webSocket.sendTXT(num, "1D", 2);  
        if (EEPROM.read(53))webSocket.sendTXT(num, "2A", 2);
        if (EEPROM.read(54))webSocket.sendTXT(num, "2T", 2);
        if (EEPROM.read(55))webSocket.sendTXT(num, "2D", 2);
        if (EEPROM.read(56))webSocket.sendTXT(num, "3A", 2);
        if (EEPROM.read(57))webSocket.sendTXT(num, "3T", 2);
        if (EEPROM.read(58))webSocket.sendTXT(num, "3D", 2);
      }  
      if(Zona1)webSocket.sendTXT(num, "Z1", 2); 
      if(Zona2)webSocket.sendTXT(num, "Z2", 2);
      if(Zona3)webSocket.sendTXT(num, "Z3", 2);  
      if(EEPROM.read(59))webSocket.sendTXT(num, "SA", 2);   //memoria de SISTEMA ACTIVADO
      else webSocket.sendTXT(num, "SD", 2);

      
    }
      
      break;
      
    case WStype_TEXT:{
      String text = String((char *) &payload[0]);      
// **** Activacion del Sistema **********//      
        if (text=="SA"){
          EEPROM.write(59,1); //Memoria se SISTEMA ACTIVADO
          EEPROM.commit();
          webSocket.sendTXT(num, "1A", 2);
          webSocket.sendTXT(num, "2A", 2);
          webSocket.sendTXT(num, "3A", 2);
          webSocket.sendTXT(num, "SA", 2);
          Serial.println("Sis. ACTIVADO");
        }
        else {
          if (text=="SD"){
            EEPROM.write(59,0);  //Memoria se SISTEMA ACTIVADO
            EEPROM.commit();
            if (EEPROM.read(50))webSocket.sendTXT(num, "1A", 2);
            if (EEPROM.read(51))webSocket.sendTXT(num, "1T", 2);
            if (EEPROM.read(52))webSocket.sendTXT(num, "1D", 2);  
            if (EEPROM.read(53))webSocket.sendTXT(num, "2A", 2);
            if (EEPROM.read(54))webSocket.sendTXT(num, "2T", 2);
            if (EEPROM.read(55))webSocket.sendTXT(num, "2D", 2);
            if (EEPROM.read(56))webSocket.sendTXT(num, "3A", 2);
            if (EEPROM.read(57))webSocket.sendTXT(num, "3T", 2);
            if (EEPROM.read(58))webSocket.sendTXT(num, "3D", 2);
            webSocket.sendTXT(num, "SD", 2);
            Serial.println("Sis. DESACTIVADO");
          }
          else {  
            

// **** Desactivacion de Alarma Sileciosa **********// 
      
            if(text=="AD1"){
              webSocket.sendTXT(num, "AD1", 3);
              Alarma1=false;   
            }
      
// **** Activacion de Alarma **********//      
      
            else {
              if (text=="AA") {
                Alarma(true);
                Serial.println("ALARMA ACTIVADA");
              }
              else {     
                if(text=="AD"){
                  Alarma(false);
                  if(Zona1){
                    webSocket.sendTXT(num, "D1", 2);
                    Zona1=false;
                  }
                  if(Zona2){
                    webSocket.sendTXT(num, "D2", 2);
                    Zona2=false;
                  }
                  if(Zona3){
                    webSocket.sendTXT(num, "D3", 2);
                    Zona3=false;
                  }  
                  Serial.println("ALARMA DESACTIVADO");
                }
                else {    
      
//****  Zona1  ******//

                  if(text=="1A"){
                    EEPROM.write(50,1); //memoria de Activada
                    EEPROM.write(52,0); //memoria de Desactivada
                    EEPROM.commit();
                    webSocket.sendTXT(num, "1A", 2);
                    Serial.println("Zona1 Activada");
                  }
                  else {   
                    if(text=="1T"){
                      EEPROM.write(50,0); //memoria de Activada
                      EEPROM.write(51,1); //memoria de Atencion
                      EEPROM.commit();
                      webSocket.sendTXT(num, "1T", 2);
                      Serial.println("Zona1 Atencion");
                    }
                    else {   
                      if(text=="1D"){
                        EEPROM.write(51,0); //memoria de Atencion
                        EEPROM.write(52,1); //memoria de Desactivada
                        EEPROM.commit();
                        webSocket.sendTXT(num, "1D", 2);
                        Serial.println("Zona1 Desactivada");
                      }
                      else {
//****  Zona2  ******//       
                        if(text=="2A"){
                          EEPROM.write(53,1); //memoria de Activada
                          EEPROM.write(55,0); //memoria de Desactivada
                          EEPROM.commit();
                          webSocket.sendTXT(num, "2A", 2);
                          Serial.println("Zona2 Activada");
                        }
                        else {
                          if(text=="2T"){
                            EEPROM.write(53,0); //memoria de Activada
                            EEPROM.write(54,1); //memoria de Atencion
                            EEPROM.commit();
                            webSocket.sendTXT(num, "2T", 2);
                            Serial.println("Zona2 Atencion");
                          }
                          else {
                            if(text=="2D"){
                              EEPROM.write(54,0); //memoria de Atencion
                              EEPROM.write(55,1); //memoria de Desactivada
                              EEPROM.commit();
                              webSocket.sendTXT(num, "2D", 2);
                              Serial.println("Zona2 Desactivada");
                            }
                            else {
        
//****  Zona3  ******//       
                              if(text=="3A"){
                                EEPROM.write(56,1); //memoria de Activada
                                EEPROM.write(58,0); //memoria de Desactivada
                                EEPROM.commit();
                                webSocket.sendTXT(num, "3A", 2);
                                Serial.println("Zona3 Activada");
                              }
                              else {
                                if(text=="3T"){
                                  EEPROM.write(56,0); //memoria de Activada
                                  EEPROM.write(57,1); //memoria de Atencion
                                  EEPROM.commit();
                                  webSocket.sendTXT(num, "3T", 2);
                                  Serial.println("Zona3 Atencion");
                                }
                                else {
                                  if(text=="3D"){
                                    EEPROM.write(57,0); //memoria de Atencion
                                    EEPROM.write(58,1); //memoria de Desactivada
                                    EEPROM.commit();
                                    webSocket.sendTXT(num, "3D",2);
                                    Serial.println("Zona3 Desactivada");
                                  }
                                }
                              }
                            }                     
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }                                       
      break;
        
    case WStype_BIN:
      hexdump(payload, lenght);
      webSocket.sendBIN(num, payload, lenght);
      break;
  }
   
}

//------------------------SETUP-----------------------------
void setup() {
  Serial.begin(57600);
  pinMode(Sensor_Z1,INPUT);
  pinMode(Sensor_Z2,INPUT);
  pinMode(Sensor_Z3,INPUT);
  pinMode(Pin_Alarma,OUTPUT);
  pinMode(Config,OUTPUT);
  // Inicia Serial
  Serial.println("");
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);
  
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();
    

  if (!Rtc.IsDateTimeValid()) 
  {
    
      // Common Cuases:
      //    1) first time you ran and the device wasn't running yet
      //    2) the battery on the device is low or even missing

      Serial.println("RTC lost confidence in the DateTime!");

      // following line sets the RTC to the date & time this sketch was compiled
      // it will also reset the valid flag internally unless the Rtc device is
      // having an issue

      Rtc.SetDateTime(compiled);
  }

  if (!Rtc.GetIsRunning())
  {
      Serial.println("RTC was not actively running, starting now");
      Rtc.SetIsRunning(true);
  }

  now = Rtc.GetDateTime();
  
  if (now < compiled) 
  {
      Serial.println("RTC is older than compile time!  (Updating DateTime)");
      Rtc.SetDateTime(compiled);
  }
  else if (now > compiled) 
  {
      Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled) 
  {
      Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 

  EEPROM.begin(512);

  if (digitalRead(Sensor_Z1) == 0) {
    modoconf();
  }

  leer(0).toCharArray(ssid, 20);
  leer(20).toCharArray(pass, 20);
  for(byte i=0;i<8; i++){
    dir[i]=EEPROM.read(40+i);
  } 
   setup_wifi();
}
//---------------------------PROGRAMA PRINCIPAL----------------------------
void loop() {
    webSocket.loop();
    Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
    String subject = "ATENCION ALARMA";
//********** Sensor Zona1 ***********///
   if(!digitalRead(Sensor_Z1)){
    delay(100);
    bitSet(puerto,0);
   }
   else bitClear(puerto,0);
   if(!digitalRead(Sensor_Z2)){
    delay(100);
    bitSet(puerto,1);
   }
   else bitClear(puerto,1);
   if(!digitalRead(Sensor_Z3)){
    delay(100);
    bitSet(puerto,2);
   }
   else bitClear(puerto,2);
   
  
  val = puerto;
  if ((val-val_p))
  {
  bandera = false;
  val_p = val;  
  //Serial.print(val);
  //Serial.println();
  enviar(val);
  }
  if (!(val-val_p)) bandera = true;
  
   
   /*pulsador1 = digitalRead(Sensor_Z1);   
   if ((pulsador1==LOW)&&(bandera1)){
        bandera1=false;
        if(EEPROM.read(50)||EEPROM.read(59)){// Memoria de Zona1 Activada || Memoria se SISTEMA ACTIVADO
          Alarma(true);
          webSocket.sendTXT(num,"Z1",2);
          Zona1=true;
          gsender->Subject(subject)->Send("globalredr@gmail.com", "Alarma en Zona1");   
        }
        else {
          if (EEPROM.read(52))webSocket.sendTXT(num,"S1",2);//Memoria de Zona1 Desactivada
          else {
          webSocket.sendTXT(num,"AA1",3);
          webSocket.sendTXT(num,"Z1",2);
          Alarma1=true;
          Zona1=true;
          }
        }
        Serial.println("ALARMA Zona1");  
    }
  
  if ((pulsador1==HIGH)&&(!bandera1)) {
     if (EEPROM.read(52)&!EEPROM.read(59))webSocket.sendTXT(num,"N1",2);  //Memoria de Zona1 Desactivada &!Memoria se SISTEMA ACTIVADO
     bandera1=true;
     Serial.println("NORMAL Zona1");
     
  } 

//********** Sensor Zona2 ***********///
    
 /*  pulsador2 = digitalRead(Sensor_Z2); 
   if ((pulsador2==LOW)&&(bandera2)){ 
      bandera2=false;
      if(EEPROM.read(53)||EEPROM.read(59)){      //Memoria de Zona2 Activada||Memoria se SISTEMA ACTIVADO
        Alarma(true);
        webSocket.sendTXT(num,"Z2",2);
        Zona2=true;
        gsender->Subject(subject)->Send("globalredr@gmail.com", "Alarma en Zona2");
        }
        else {
          if (EEPROM.read(55))webSocket.sendTXT(num,"S2",2);//Memoria de Zona2 Desactivada
          else {
          webSocket.sendTXT(num,"AA1",3);
          webSocket.sendTXT(num,"Z2",2);
          Alarma1=true;
          Zona2=true;
          }
        } 
      Serial.println("ALARMA Zona2");   
   }
  if ((pulsador2==HIGH)&&(!bandera2)) {
     if (EEPROM.read(55)&!EEPROM.read(59))webSocket.sendTXT(num,"N2",2);//Memoria de Zona2 Desactivada&!Memoria se SISTEMA ACTIVADO
     bandera2=true;
     Serial.println("NORMAL Zona2");
  }   

//********** Sensor Zona3 ***********///
    
  /* pulsador3 = digitalRead(Sensor_Z3); 
   if ((pulsador3==LOW)&&(bandera3)){ 
        bandera3=false;
        if(EEPROM.read(56)||EEPROM.read(59)){//Memoria de Zona3 Activada||Memoria se SISTEMA ACTIVADO
          Alarma(true);
          webSocket.sendTXT(num,"Z3",2);
          Zona3=true;
          gsender->Subject(subject)->Send("globalredr@gmail.com", "Alarma en Zona3");
        }
        else {
          
          if (EEPROM.read(58))webSocket.sendTXT(num,"S3",2);//memoria de Zona3 Desactivada
          else {
          webSocket.sendTXT(num,"AA1",3);
          webSocket.sendTXT(num,"Z3",2);
          Alarma1=true;
          Zona3=true;
          }
        }
        Serial.println("ALARMA Zona3"); 
    }
    
    if ((pulsador3==HIGH)&&(!bandera3)) {
     if (EEPROM.read(58)&!EEPROM.read(59))webSocket.sendTXT(num,"N3",2);//memoria de Zona3 Desactivada&!Memoria se SISTEMA ACTIVADO
     bandera3=true;
     Serial.println("NORMAL Zona3");    
    }*/
    if (FueActivada){ 
      now = Rtc.GetDateTime();
      /*printDateTime(now);
      Serial.println();
      printDateTime(alarmTime);
      Serial.println(); */
      if ((alarmTime.Second()==now.Second())&&(alarmTime.Minute()==now.Minute()))
          {
              Serial.println("Alarma Desactivada");
              Alarma(false);
          }
    }   
}
