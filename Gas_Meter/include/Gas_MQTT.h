#include <PicoMQTT.h>


PicoMQTT::Client mqtt( "192.168.2.213" );
uint32_t  MQTT_start_consumption ;                 // set after MQTT message
uint32_t  MQTT_start_period ;                      // set after MQTT message


void setup_MQTT( ) 
{
    // Subscribe to start of counter data at start and attach a callback
   mqtt.subscribe("Energy/Gas/Init", [](const char * topic, const char * payload ) 
   {
       // payload might be binary, but PicoMQTT guarantees that it's zero-terminated
       Serial.printf("Received message in topic '%s': %s\n", topic, payload);
       MQTT_start_consumption = atoi( payload );
   });
   // Subscribe to start of mearuring period and attach a callback
   mqtt.subscribe("Energy/Gas/Init1", [](const char * topic, const char * payload ) 
   {
       // payload might be binary, but PicoMQTT guarantees that it's zero-terminated
       Serial.printf("Received message in topic '%s': %s\n", topic, payload);
       MQTT_start_period = atoi( payload );
   });
   mqtt.begin();
}

void publish_topic( String subtopic, String message ) 
{
   String        topic;
      
   topic   = "Energy/" + subtopic;
//
   Serial.printf( "Publishing message in topic '%30s': %15s\n", topic.c_str( ), message.c_str( ) );
   mqtt.publish( topic, message );
}
