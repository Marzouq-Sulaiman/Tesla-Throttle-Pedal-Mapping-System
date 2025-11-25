#include <WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "LAPTOP-UHMN8MIP 4335";
const char* password = "46Qh8]68";

WiFiUDP sendUDP;
WiFiUDP recvUDP;

// CHANGE TO LAPTOP'S IP ADDRESS (ON HOTSPOT NETWORK)
const char* laptop_ip = "192.168.137.1";  
//const char* laptop_ip = "192.168.2.1";
//const char* laptop_ip = "172.20.10.1";


const int sendPort = 5005;
const int recvPort = 6000;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Entered Bootloader");

  WiFi.begin(ssid, password);
  Serial.println("*Connecting*");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n*Connected*");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("Sending to laptop IP: ");
  Serial.println(laptop_ip);

  sendUDP.begin(sendPort);
  recvUDP.begin(recvPort);
}

void loop() {
  
  //SEND
  int value = random(0, 100); //PUT TORQUE COMMAND HERE
  sendUDP.beginPacket(laptop_ip, sendPort);
  sendUDP.print(value);
  sendUDP.endPacket();

  //RECEIVE
  int packetSize = recvUDP.parsePacket();
  if (packetSize != 0) {
    char buffer[32];
    int len = recvUDP.read(buffer, 32);
    buffer[len] = 0;
    Serial.print("Received from laptop: ");
    Serial.println(buffer);
  }
//   int packetSize = recvUDP.parsePacket();
// Serial.print("packetSize = ");
// Serial.println(packetSize);

// if (packetSize > 0) {
//     char buffer[50];
//     int len = recvUDP.read(buffer, 50);
//     buffer[len] = 0;

//     Serial.print("RECEIVED RAW: '");
//     Serial.print(buffer);
//     Serial.println("'");
// }

  Serial.print("Sent: ");
  Serial.println(value);

  delay(1000); // send every second
}
