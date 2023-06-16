//Librerias
#include "FS.h"
#include "SD.h"
#include <esp_now.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//Definicion pines
#define led 2

//Variables de conexion WiFi
const char *ssid = "Tenda_DF630";
const char *password = "12345678";

//Contador
int conta;

// Definir el NTP Client para obtener la hora y fecha
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables para guardar fecha y hora
String formattedDate;
String dayStamp;
String timeStamp;

//Estructura para enviar por ESP-NOW
typedef struct struct_message {
  char a[32];
  int b;
} struct_message;

//Variable para guardar dato UID
String dato = "";

// myData e la que se recibe
struct_message myData;

// callback función que se ejecutará cuando se reciban los datos
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {

  memcpy(&myData, incomingData, sizeof(myData));
  if (myData.b == 1) {
    digitalWrite(led, HIGH);
    delay(1000);
    digitalWrite(led, LOW);
  }
  Serial.print("UID: ");
  dato = String(myData.a);
  Serial.println(dato);
}

//Metodo para listar directorios
void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

//Metodo para crear directorios
void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

//Metodo para eliminar directorios
void removeDir(fs::FS &fs, const char *path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

//Metodo para leer archivo
void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

//Metodo para escribir en un archivo
void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

//Metodo para escribir en un archivo con salto de linea
void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}


void setup() {
  //Iniciar comunicaion serial
  Serial.begin(115200);

  //Mensajes de conexion
  Serial.print("Connecting to ");
  Serial.println(ssid);
  //Ponemos el modo Wi-Fi Acces Point y Station
  WiFi.mode(WIFI_AP_STA);
  //Iniciamos la comunicacion Wi-Fi
  WiFi.begin(ssid, password);
  //Permitimos la auto reconexion
  WiFi.setAutoReconnect(true);
  //Inicializamos ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // iniciamos la recepción
  esp_now_register_recv_cb(OnDataRecv);

  //Iniciamos el NTPClient para obtener tiempo
  timeClient.begin();

  //GMT-6: México (hora centro),
  timeClient.setTimeOffset(-6 * 3600);

  //Inidicar el modo de los pines de los leds
  pinMode(led, OUTPUT);

  //Iniciamos el modulo SD
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }

  //Variable String que sera puesta en los encabezados
  String Header = "Num,UID,Fecha,Hora";
  Header += "\r\n";
  //Escribimos los encabezados en el archivo datos.csv
  appendFile(SD, "/datos.csv", Header.c_str());
}

void loop() {

  //Obtenemos la hora y fecha
  timeClient.update();

  //Guardamos la fecha y hora en la variable
  formattedDate = timeClient.getFormattedDate();

  // Extraemos la fecha
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  // Extraemos la hora
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);

  //Incrementamos en +1 por cada regitro el contador
  conta++;

  //Inicamos a contatenar en una cadena los valores para ser almacenados en el csv
  String dataString = "";
  dataString += conta;
  dataString += ",";
  dataString += dato;
  dataString += ",";
  dataString += dayStamp;
  dataString += ",";
  dataString += timeStamp;
  dataString += "\r\n";
  //Escribimos la cadena en el archivo datos.csv
  appendFile(SD, "/datos.csv", dataString.c_str());
  //Tiempo de espera para nuevo registro
  delay(10000);
}