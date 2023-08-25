//
//
#include <ESP8266WiFi.h>
//#include <WiFi.h>
//

const char* ssid               = "your_SSID";
const char* password           = "your_password";

WiFiClient        Thingspeak_client ;


void Gas_WiFi_connect()

{
    //connect to WiFi
   Serial.printf( "Connecting to %s ", ssid );
   int loop_count      = 0;
   WiFi.begin(ssid, password);
   while ( WiFi.status() != WL_CONNECTED ) 
   {
      delay(500);
      Serial.print(".");
      loop_count++;
      if ( loop_count > 60 ) break;
   }
   if ( (WiFi.status() == WL_CONNECTED) )
     Serial.printf( "\r\nCONNECTED" );
   else
     Serial.printf( "\r\nNo WiFi\r\n" );
     
//  WiFi.disconnect( false);
}

void Gas_WiFi_reconnect()

{

   Serial.printf( "Re-Connecting to %s ", ssid );
   int loop_count      = 0;
   WiFi.begin( ssid, password );
  
   while (WiFi.status() != WL_CONNECTED) 
   {
      delay(500);
      Serial.print( "." );
      loop_count++;
      if ( loop_count > 60 ) break;
   }
   if ( (WiFi.status() == WL_CONNECTED) )
     Serial.printf("\r\nWiFi re-connected\r\n" );
   else
     Serial.printf( "\r\nReconnect failed\r\n" );
}
