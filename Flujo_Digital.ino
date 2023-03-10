#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ESP32Time.h>
//Librerias para tiempo
#include "time.h"

#define FIREBASE_HOST "" // --> link de realtime database
#define FIREBASE_Authorization_key ""  //--> secreto de la base de datos en config

FirebaseData firebaseData;

// NTP server to request epoch time
const char* ntpServer = "co.pool.ntp.org";
const long  gmtOffset_sec = -18000;
const int daylightOffset_sec = 0;
// Variable to save current epoch time
unsigned long epochTime;

// Declaración de pines y variables
const int sensorPin = 18;   // Pin digital al que está conectado el sensor
volatile int pulseCount;   // Contador de pulsos
float flowRate;            // Flujo de agua en LPM
unsigned long currentTime; // Tiempo actual
unsigned long lastTime;    // Tiempo de la última lectura
String time_str;
String hora;
String dia;
String mes;
String ano;
String fecha;
bool banderaWifi = true;
bool configured = true;
int contador_reset_wifi = 0;
int contadorfl = 0;
// Establece el límite máximo de flujo (en litros por minuto)
const float maxFlowRate = 180.5; // Ajusta esto según tus necesidad

// Configura la conexión WiFi
const char* ssid = "Soluciones Verticales SAS";
const char* pass = "Z24cBn9p4L";
const char* red;
const char* contra;

String printTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "Time Error";
  }
  //See http://www.cplusplus.com/reference/ctime/strftime/
  char output[80];
   // strftime(output, 80, "%H:%M:%S", &timeinfo);
   strftime(output, 80, "%d-%b-%y, %H:%M:%S", &timeinfo);
  time_str = String(output);

  return time_str;
}

void wifi_setup(String nomid, String pass){
  // Set device as a Wi-Fi Station
   WiFi.mode(WIFI_AP_STA);
     Serial.print("ssid: ");
  Serial.println(nomid);
  Serial.print("password: ");
  Serial.println(pass);
   red = nomid.c_str();
   contra = pass.c_str();
   WiFi.begin(red, contra);
   while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.println("Setting as a Wi-Fi Station..");
    if (contador_reset_wifi < 5){
      contador_reset_wifi++;
      Serial.println(contador_reset_wifi);
    }else{
      ESP.restart();
      contador_reset_wifi = 0;
    }
  }
  Serial.print("WiFi connected with ip ");  
  Serial.println(WiFi.localIP());
 
  // color Azul
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  banderaWifi = false;
  configured = false;
  configTime(0, 0, "pool.ntp.org");  
}

void setup() {
  Serial.begin(115200);
  // Configuración del pin de entrada
  pinMode(sensorPin, INPUT_PULLUP);
  wifi_setup(ssid,pass);  
  Firebase.begin(FIREBASE_HOST,FIREBASE_Authorization_key);
  // Inicialización de variables
  pulseCount = 0;
  flowRate = 0.0;
  currentTime = 0;
  lastTime = 0;

  // Interrupción externa en el pin digital 2
  attachInterrupt(digitalPinToInterrupt(sensorPin), pulseCounter, FALLING);
}

String get_time(){ 
   setenv("TZ", "UTC+05:00", 1);
   String tiempo = printTime();
   hora = tiempo.substring(11,20); 
  //  Serial.println("  COLOMBIAN Time = "+ hora);    
  //  Serial.println(hora);             
  //  Serial.println("----HORA LOCAL DE COLOMBIA SUMERCÉ-----");
   return hora;
}

void get_date(){ 
   setenv("TZ", "UTC+05:00", 1);
   String tiempo = printTime();
   ano = tiempo.substring(7,9);
   mes = tiempo.substring(3,6);
   dia = tiempo.substring(0,2);
  //  Serial.println("  COLOMBIAN year = "+ ano);
  //  Serial.println("  COLOMBIAN Month = "+ mes);
  //  Serial.println("  COLOMBIAN Day = "+ dia);
   fecha = dia+"-"+mes+"-20"+ano;    
  //  Serial.println(fecha);             
  //  Serial.println("----FECHA LOCAL DE COLOMBIA SUMERCÉ-----");
}

void loop() {

 // Lee el valor del sensor de flujo
  int flowPulses = pulseIn(sensorPin, HIGH);
  flowRate = flowPulses / 7.5; // Cada pulso representa 7.5 ml de flujo
  // Lectura de pulsos y cálculo del flujo de agua

  // Si el flujo es mayor que el límite establecido, apaga el flujo de agua
  if (flowRate > maxFlowRate) {
    get_time();
    get_date();
    Serial.println("Flujo de agua excedido");
    
  // Imprime el valor de flujo en el puerto serie
  Serial.print("Flujo: ");
  Serial.print(flowRate);
  Serial.println(" L/min");
  Firebase.setFloat(firebaseData, "Flujo/Datos/20"+ano+"/"+dia+"/"+hora+"/Flujo/", flowRate);
  contadorfl++;
  Serial.print("Tiempo de llenado: ");
  Serial.println(contadorfl);
    if(contadorfl >= 15){
      Serial.println("Tiempo de flujo de agua excedido");
      String mss = "Flujo excedido";
  Firebase.setString(firebaseData, "Flujo/Alerta/20"+ano+"/"+dia+"/"+hora+"/Mensaje/", mss);
    // Aquí debes agregar código para apagar el suministro de agua
    lastTime = currentTime;
    attachInterrupt(digitalPinToInterrupt(sensorPin), pulseCounter, FALLING);
    }
  }else{
    contadorfl = 0;
  }
 
  // Espera un segundo antes de volver a leer el sensor
  delay(1000);

    // detachInterrupt(digitalPinToInterrupt(sensorPin));

  
}

void pulseCounter() {
  // Incrementa el contador de pulsos cada vez que se detecta un pulso
  pulseCount++;
}
