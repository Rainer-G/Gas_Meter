#include       "ThingSpeak.h"     

WiFiClient     client1;
const char* host     = "api.thingspeak.com";
const int   httpPort = 80;
String      Gas_Thingspeak_data;

#define     ThingSpeakKEY   "your key"
#define     myChannelNumber 1234567    // put in your channel number


//
// ===================== Thingsspeak functions ==================================================================
//
boolean Send_data( )
//
{
   //
   // write data to the ThingSpeak channel
   //
   int x = ThingSpeak.writeFields( myChannelNumber, ThingSpeakKEY );
   if( x == 200 )
   {
      return true;
   }
   else
   {
      return false;
   }
}  
