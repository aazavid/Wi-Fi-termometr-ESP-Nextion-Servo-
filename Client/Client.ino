//#define DEBAG
#define ESP_RECIVE

#ifdef ESP_RECIVE
 #include <ESP8266WiFi.h>
 #include <WiFiClient.h>
#endif

#include <Servo.h>

#define SERIAL_RECIVE   Serial     // прием из NEXTION
#define SERIAL_TRANSMIT Serial     // передача в NEXTION или в порт



Servo myservo;  

int pos = 0;
int ser = 0;

const char* ssid     = "ESPap";
const char* password = "23519980";

const char* host = "192.168.4.1";

const int DELAY_REQUEST = 5000; // время обновления в мс
unsigned long timeState      = 0;  

#ifdef ESP_RECIVE
 WiFiServer server(80);
#endif

void setup() {
 // SERIAL_TRANSMIT.begin(115200);
  SERIAL_RECIVE.begin(115200);
  myservo.attach(4); 
  delay(10);

  #ifdef DEBAG
    SERIAL_TRANSMIT.println();
    SERIAL_TRANSMIT.println();
    SERIAL_TRANSMIT.print("Connecting to ");
    SERIAL_TRANSMIT.println(ssid);
  #endif

#ifdef ESP_RECIVE 
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_STA); // режим клиента
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
   
  }

   
  #ifdef DEBAG 
    SERIAL_TRANSMIT.println("");
    SERIAL_TRANSMIT.println("WiFi connected");  
    SERIAL_TRANSMIT.println("IP address: ");
    SERIAL_TRANSMIT.println(WiFi.localIP());
  #endif


    server.begin();
  #endif  
     #ifdef DEBAG 
        SERIAL_TRANSMIT.println("HTTP server started");
     #endif
     timeState = millis();
     myservo.write(180);

}


void loop() {
  
  if (millis() - timeState > DELAY_REQUEST) {

#ifdef DEBAG 
    SERIAL_TRANSMIT.print("connecting to ");
    SERIAL_TRANSMIT.println(host);
#endif 
  
 #ifdef ESP_RECIVE
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;

  
  if (!client.connect(host, httpPort)) {

   #ifdef DEBAG 
    SERIAL_TRANSMIT.println("connection failed");
   #endif 

    return;
  }
  String url = "/servo";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) 
  {
    if (millis() - timeout > 5000) 
    {
     
     #ifdef DEBAG 
        SERIAL_TRANSMIT.println(">>> Client Timeout !");
     #endif
        
      client.stop();
      return;
    }
  }
  
  // Read all the lines of the reply from server and print them to Serial
  
    String line = client.readStringUntil('\r');
    
    int posOpen  = line.indexOf("OPEN");
    int posClose = line.indexOf("CLOSE");
    if(posOpen >= 0)
    {
      myservo.write(0);
    }
    if(posClose>= 0)
    {
      myservo.write(180);
    }   
   #endif
    timeState = millis();
  }
} 
// END MAIN LOOP
  

