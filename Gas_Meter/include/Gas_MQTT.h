#include <PicoMQTT.h>
//
PicoMQTT::Client mqtt( "192.168.2.213" );
String    MQTT_command        = "";                // commands for Counter
String    topic               = "Energy/Test/";
//
//
//
void setup_MQTT( ) 
{
//
// Subscribe to MQTT commands
//
   mqtt.subscribe( topic + "command", [](const char * topic, const char* payload ) 
   {
       // payload might be binary, but PicoMQTT guarantees that it's zero-terminated
       Serial.printf( "Received message in topic '%s': %s\n", topic, payload );
       MQTT_command = payload ;
   });
    mqtt.begin();
}

void publish_topic( String subtopic, String message ) 
{
//
   String  topic1 = topic + subtopic;
   Serial.printf( "Publishing message in topic '%30s': %15s\n", topic1.c_str( ), message.c_str( ) );
   mqtt.publish( topic1, message );
   delay( 100 );
}
