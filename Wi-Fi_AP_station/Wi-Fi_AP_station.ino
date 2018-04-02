//#define DEBUG
#define ESP_RECIVE

#ifdef ESP_RECIVE
 #include <ESP8266WiFi.h>
 #include <WiFiClient.h>
#endif

#define SERIAL_RECIVE   Serial     // прием из NEXTION
#define SERIAL_TRANSMIT Serial     // передача в NEXTION или в порт

#include "BMP280.h"
#include "Wire.h"
#define P0 1013.25
BMP280 bmp;


/* Set these to your desired credentials. */
const char *ssid = "ESPap";
const char *password = "23519980";

WiFiServer server(80);

int val = -1;
String response = "";
double T = 0, P = 0, H = 0; // температура, давление, влажность

unsigned long timeBMP     = 0;
unsigned long timeClient  = 0;
unsigned long timeNextion = 0;

const int TIME_REQUEST_BMP     = 500;
const int TIME_SERVER          = 3000;
const int TIME_DISPLAY_NEXTION = 1000;

bool flagUpdate    = false;
bool alarmT        = false;
byte incomingByte  = 0;
double limit       = 25.0;
double temperature = 24.0;    


// --------------_ Объявление функций _--------------------
void sendCommand(const char* cmd);
bool ack(void);
//----------------------------------------------------------
void setup() {
	delay(1000);
	
	SERIAL_RECIVE.begin(115200);
	#ifdef DEBUG
		SERIAL_TRANSMIT.begin(115200);
	#endif
    
    bmp.begin(2,14);
    bmp.setOversampling(4);

    #ifdef DEBUG

		SERIAL_TRANSMIT.println();
		SERIAL_TRANSMIT.print("Configuring access point...");
	#endif

	WiFi.softAP(ssid, password);

	IPAddress myIP = WiFi.softAPIP();

	#ifdef DEBUG
		SERIAL_TRANSMIT.print("AP IP address: ");
		SERIAL_TRANSMIT.println(myIP);
	#endif

	server.begin();

	#ifdef DEBUG
		SERIAL_TRANSMIT.println("HTTP server started");
	#endif

	timeNextion = millis();
	timeClient  = millis();
	timeBMP     = millis();
}

void loop() {

	if(millis() - timeBMP > TIME_REQUEST_BMP)
	{
        char result = bmp.startMeasurment();
    	delay(result);
    
    	result = bmp.getTemperatureAndPressure(T,P);
    	if(result!=0)
    	{
			response += "<H1>Temp=";
       		response += T;
       		response += "Pres=";
       		response += P;
       		response += "</H1>";

    	}
    	else{
    		response += "<p>Error Temperature</p>";
    		#ifdef DEBUG
    			SERIAL_TRANSMIT.println("Error BMP280");
    		#endif
    	}
    	timeBMP = millis();
    }

//-----------_ Проверка температуры _-------------------
    if(T > limit)
  	{
    	if(!alarmT)
   		{
      
      		#ifdef DEBAG 
        		SERIAL_TRANSMIT.print("temperature > limit ");
        		SERIAL_TRANSMIT.print("limit - ");
        		SERIAL_TRANSMIT.print(limit);
        		SERIAL_TRANSMIT.print(" SERVO - ");
        		SERIAL_TRANSMIT.println(ser);
      		#endif
      	
      		alarmT = true;
      		String n = "t4.txt=\"OTKP.\"";
     		sendCommand(n.c_str());
     		ack();
     
    	}
  	}
  	if(T < (limit - 0.5))
  	{
    	if(alarmT)
    	{
    
      		#ifdef DEBAG 
        		SERIAL_TRANSMIT.print("temperature < limit ");
        		SERIAL_TRANSMIT.print("limit - ");
        		SERIAL_TRANSMIT.print(limit);
        		SERIAL_TRANSMIT.print(" SERVO - ");
        		SERIAL_TRANSMIT.println(ser);
      		#endif
      	
      		alarmT =false;
      		String n = "t4.txt=\"3AKP.\"";
     		sendCommand(n.c_str());
     		ack();
      	}
  
  	}
//------------------------------------------------------------
//----------_ Работа с NEXTION _------------------------------
    if(millis() - timeNextion > TIME_DISPLAY_NEXTION)
    {
		if (SERIAL_RECIVE.available())
  		{
  		    incomingByte = SERIAL_RECIVE.read();
        
        	#ifdef DEBAG 
        		SERIAL_TRANSMIT.print("NEXTION");
        		SERIAL_TRANSMIT.println(incomingByte);
        	#endif
       		switch(incomingByte)
       		{
         		case 101: // Нажатие кнопки
        		{
          			int pageId      = SERIAL_RECIVE.read();
          			int componentId = SERIAL_RECIVE.read();
          			int flagPress   = SERIAL_RECIVE.read();

          			#ifdef DEBAG
            			SERIAL_TRANSMIT.print("pageId - ");
            			SERIAL_TRANSMIT.println(pageId);
            			SERIAL_TRANSMIT.print("componentId - ");
            			SERIAL_TRANSMIT.println(componentId);
            			SERIAL_TRANSMIT.print("flagPress - ");
            			SERIAL_TRANSMIT.println(flagPress);
          			#endif 
          			if (flagPress == 1)flagUpdate = true;
          			else flagUpdate = false; 
          			
          			// завершаем чтение 
          			while(SERIAL_RECIVE.available()){
              			SERIAL_RECIVE.read();
          			}
          			break;
          
              	}
        		case 113: // Передача лимита температуры
        		{
          			while(!SERIAL_RECIVE.available())
          			{delay(1);}
           
           			limit = SERIAL_RECIVE.read();
           

          			#ifdef DEBAG
            			SERIAL_TRANSMIT.print("Limit - ");
            			SERIAL_TRANSMIT.println(limit);
          			#endif 
          			SERIAL_RECIVE.read();
          			
          			while(SERIAL_RECIVE.available()){
              			SERIAL_RECIVE.read();
          			}
          
          			break;
        		}
       
        		default : break;
        
       		}
   		}// Если не было приёма, то просто обновляем показания

   		String n = "t2.txt=\"";
   		n +=  T;
   		n += "\"";
   		sendCommand(n.c_str());
   		ack();
   		String s = "t3.txt=\"";
   		s += P;
   		s += "\"";
   		sendCommand(s.c_str());
   		ack(); 
   	
   		timeNextion = millis();
	}
//------------------------------------------------------------
    if(millis() - timeClient > TIME_SERVER)
	  {
		  WiFiClient client = server.available();
  		if (!client) 
      {
    		// нет подключений
    		timeClient = millis();
    		return;
  		}
  		// Ждем данных от клиента
  		#ifdef DEBUG
  			SERIAL_TRANSMIT.println("new client");
  		#endif
  		while(!client.available())
      {
    		delay(1);
  		}
  		// Читаем данные от клиента
  		String req = client.readStringUntil('\r');
  		
  		#ifdef DEBUG
  			SERIAL_TRANSMIT.println(req);
  		#endif
  		client.flush();
  
  		// Match the request
  	  if (req.indexOf("/all") != -1)
    		val = 0;
  		else if (req.indexOf("/servo") != -1)
    		val = 1;
  		else 
      {

  			#ifdef DEBUG
    			SERIAL_TRANSMIT.println("invalid request");
    		#endif
    	  client.stop();
    	  val = -1;
    	  timeClient = millis();
    	  return;
  		}

      client.flush();	
 		  switch(val)
 		  {
 			  case 0:
 			  {
 				  String res= "<h1>Hello!</h1></br>";
 				  res += response;
 				  // Send the response to the client
  				client.print(res);
  				delay(1);
  				
  				#ifdef DEBUG
  					SERIAL_TRANSMIT.println("Client disonnected");
  				#endif		
  				// The client will actually be disconnected 
 				  break;
 			  }
 			  case 1:
 			  {
 				  if(alarmT)
 				  {
 					  String res = "OPEN \r";
 					  client.print(res);
 					  delay(1);
 				  }
 				  else
 				  {
 					  String res = "CLOSE \r";
 					  client.print(res);
 					  delay(1);
 				  }
 				  break;
 			  }
 			  case -1:
 			  {
 				   break;
 			  }
 			  default:
 			  {
 				   #ifdef DEBUG
 					  SERIAL_TRANSMIT.print("Error val recive server - ");
 					  SERIAL_TRANSMIT.println(val);
 				   #endif
 				   break;
 			  }
 		}
 		timeClient = millis();

	}
   
}

bool ack(void){
  uint8_t bytes[4] = {0};
  SERIAL_TRANSMIT.setTimeout(20);
  if (sizeof(bytes) != SERIAL_TRANSMIT.readBytes((char *)bytes, sizeof(bytes))){
    return 0;
  }//end if
  if((bytes[1]==0xFF)&&(bytes[2]==0xFF)&&(bytes[3]==0xFF)){
    switch (bytes[0]) {
  case 0x00:
    return false; break;
    //return "0"; break;      
  case 0x01:
    return true; break;
    //return "1"; break;
    /*case 0x03:
    return "3"; break;
  case 0x04:
    return "4"; break;
  case 0x05:
    return "5"; break;
  case 0x1A:
    return "1A"; break;
  case 0x1B:
    return "1B"; break;//*/
  default: 
    return false;
    }
  }
}

void sendCommand(const char* cmd)
{
  while(SERIAL_TRANSMIT.available())
  {
    SERIAL_TRANSMIT.read();
  }

  SERIAL_TRANSMIT.print(cmd);
  SERIAL_TRANSMIT.write(0xFF);
  SERIAL_TRANSMIT.write(0xFF);
  SERIAL_TRANSMIT.write(0xFF);
}
