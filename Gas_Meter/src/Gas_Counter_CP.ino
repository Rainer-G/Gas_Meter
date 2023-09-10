
/**************************************************************************/
/*
  Use RTC as counter and read results with TWI
  Store last value in RAM
*/
/**************************************************************************/
//#define THINGSPEAK                                 // if you want send data to the Internet

#include <Wire.h>
#include "Gas_WiFi.h"
#include "Gas_MQTT.h"
#include "Gas_Time.h"
#ifdef THINGSPEAK
  #include "Gas_Thingspeak.h"
#endif  
//
#define RESET_PIN            D7                             // LOW will reset the counter and clear old data
#define UPDATE_PIN           D6                             // LOW will prevent going to sleep, needed for MQTT Commands & OTA updates
//
#define COUNTER_MODE         0x20                           // counter mode 0x20  
#define COUNTER_ADDRESS      0x50                           // 7 I²C bit address for RTC Chip
// address locations
#define CONTROL_ADDRESS      0x00
#define ADDRESS_COUNTER      0x01                           // only 3 bytes with 6 BCD digits
// starting from 0x10, we have 240 bytes of RAM for free use
#define RAM_OFFSET           0x10                           // start after reserved RAM locations
#define ADDRESS_OLD_COUNTER  RAM_OFFSET                     // provide 4 bytes
#define ADDRESS_OLD_TIME     ADDRESS_OLD_COUNTER   + 4      // provide 4 bytes
#define ADDRESS_OLD_DATE     ADDRESS_OLD_TIME      + 4      // provide 4 bytes
#define ADDRESS_START_METER  ADDRESS_OLD_DATE      + 4      // provide 4  bytes for meter data at first start 
#define ADDRESS_START_PERIOD ADDRESS_START_METER   + 4      // provide 4  bytes for meter data for start of period
#define ADDRESS_MONTH_01     ADDRESS_START_PERIOD  + 4      // provide 4  bytes for meter data at month end of first month of period
#define NEXT_FREE_ADDRESS    ADDRESS_MONTH_01      + ( 4 * 12 )  
//
//**********************************************************************************
// connect D0 to RST if you want sleep
// using D6 for controlling Sleep Mode
//**********************************************************************************
//
bool      sleep                = false;//true;             // allow sleep
bool      error                = false;            // report error to MQTT
bool      log_msg              = true;             // for Test only, set to false when in operation
uint32_t  old_time;                                // in seconds
uint8_t   old_date[ 3 ];                           // year, month, day
uint32_t  new_time;                                // in seconds
uint8_t   new_date[ 3 ];                           // year, month, day
uint32_t  old_count            = 0;
uint32_t  new_count            = 0;
uint32_t  delta_count          = 0;
uint32_t  liter_per_count      = 10;               // see meter label
float     mexp3_per_count      = 0.01;             // see meter label
double    brennwert            = 11.53800;
double    zustandszahl         = 0.93940;
uint16_t  year_offset          = 2000;
float     energy1              = 0;                // current load in kWh  : Gas m³ * Brennwert * Zustandszahl
double    total_energy         = 0;                // total usage  in kWh  : Gas m³ * Brennwert * Zustandszahl
uint32_t  start_consumption    = 0;                // set after first start or battery change
double    consumption          = 0;                // Gas in liter
uint32_t  start_period         = 0;                // Gas in liter
uint32_t  period               = 0;                // in seconds
uint32_t  consumption_data     = 0;                // value for a monthly consumption
String    consumption_string   = "";               // temp string for a monthly consumption
uint8_t   month_data           = 0;                // month for above
uint32_t  temp_data            = 0;
//char      valueString[ 80 ];
char      valueString1[ 80 ];
char      valueString2[ 80 ];
char      valueString3[ 80 ];
char      valueString4[ 80 ];
char      valueString5[ 80 ];
char      valueString6[ 80 ];
char      valueString7[ 80 ];
char      valueString8[ 80 ];
//char      valueString9[ 80 ];
//char      valueString10[ 80 ];
//char      valueString11[ 80 ];
uint8_t   old_year;
uint8_t   old_day;
uint8_t   old_month;
uint8_t   old_hour;
uint8_t   old_min;
uint8_t   old_sec;
//
//  for data transfer
//
union     long_t
{
  uint32_t long_data;
  uint8_t  byte_data[ 4 ];
};
union     int_t
{
  uint16_t int_data;
  uint8_t  byte_data[ 2 ];
};
//
//
uint8_t bcd2byte( uint8_t value )
{
  return (( value >> 4 ) * 10 ) + ( value & 0x0f );
}

uint8_t byte2bcd( uint8_t value )
{
  return (( value / 10) << 4 ) + ( value % 10 );
}


bool set_Counter_Mode( )
{
  Wire.beginTransmission( COUNTER_ADDRESS );

  Wire.write( CONTROL_ADDRESS );
  Wire.write( COUNTER_MODE );
  return ( Wire.endTransmission( true ) == 0 );
}

uint8_t get_Mode( )
{
  return get_Byte( CONTROL_ADDRESS );
}

bool set_Byte( int8_t address, int8_t value )
{
  Wire.beginTransmission( COUNTER_ADDRESS );
  Wire.write( address );
  Wire.write( value );
  return ( Wire.endTransmission( true ) == 0 );
}

int8_t get_Byte( int8_t address )
{
  Wire.beginTransmission( COUNTER_ADDRESS );
  Wire.write( address );
  Wire.endTransmission( );
  Wire.requestFrom( COUNTER_ADDRESS, 1 );
  return Wire.read( );
}

bool set_Integer( int8_t address, int16_t value )
{
  int_t  temp_data;
  temp_data.int_data = value;
  Wire.beginTransmission( COUNTER_ADDRESS );
  Wire.write( address );
  Wire.write( temp_data.byte_data[ 0 ] );
  Wire.write( temp_data.byte_data[ 1 ] );
  return ( Wire.endTransmission( true ) == 0 );
}

bool get_Integer( int8_t address, int16_t &value )
{
  int_t  temp_data;
  
  value = 0;
  Wire.beginTransmission( COUNTER_ADDRESS );
  Wire.write( address );
  Wire.endTransmission( );
  byte len = Wire.requestFrom( COUNTER_ADDRESS, 2 );
  if ( len == 0 )
  {
    Serial.println("");
    Serial.println("***************** Error occured when reading 2 bytes ******************");
    return false;
  }
  else
  {
    temp_data.byte_data[ 0 ] = Wire.read( );
    temp_data.byte_data[ 1 ] = Wire.read( );
    value                    = temp_data.int_data;
    return true;
  }
}

bool set_My_4_bytes( int8_t address, int32_t value )
{
  union long_t  temp_data;
  int8_t  result = 9;
  temp_data.long_data = value;
  Wire.beginTransmission( COUNTER_ADDRESS );
  Wire.write(  address );
  Wire.write(  temp_data.byte_data[ 0 ] );
  Wire.write(  temp_data.byte_data[ 1 ] );
  Wire.write(  temp_data.byte_data[ 2 ] );
  Wire.write(  temp_data.byte_data[ 3 ] );
  return ( Wire.endTransmission( true ) == 0 );
}


bool get_My_4_bytes( int8_t address, uint32_t &value )
{

  long_t  temp_data;

  value = 0;

  Wire.beginTransmission( COUNTER_ADDRESS );
  Wire.write( address );
  Wire.endTransmission( );
  byte len = Wire.requestFrom( COUNTER_ADDRESS, 4 );
  if ( len == 0 )
  {
    Serial.println("");
    Serial.println("********************* Error occured when reading My 4 bytes **************");
    return false;
  }
  else
  {
    temp_data.byte_data[ 0 ] = Wire.read( );
    temp_data.byte_data[ 1 ] = Wire.read( );
    temp_data.byte_data[ 2 ] = Wire.read( );
    temp_data.byte_data[ 3 ] = Wire.read( );
    value            = temp_data.long_data;
    return true;
  }
}

bool get_Counter( int8_t address, uint32_t &count  )
{
  count = 0;
  Wire.beginTransmission( COUNTER_ADDRESS );               // stored as BCD digits in 3 bytes
  Wire.write( address );
  Wire.endTransmission( );
  byte len = Wire.requestFrom( COUNTER_ADDRESS, 3);

  if (len == 0)
  {
    Serial.println("");
    Serial.println("********************* Error occured when reading Counter *****************");
    return false;
  }
  else
  {
    count =         bcd2byte( Wire.read( ));
    count = count + bcd2byte( Wire.read( )) * 100L;
    count = count + bcd2byte( Wire.read( )) * 10000L;
    return true;
  }
}

bool set_Counter( int8_t address, int32_t value )            // store as BCD digits in 3 bytes
{
  Wire.beginTransmission( COUNTER_ADDRESS );
  Wire.write( address );
  Wire.write( byte2bcd(   value % 100L ));
  Wire.write( byte2bcd( ( value / 100L ) % 100L ));
  Wire.write( byte2bcd( ( value / 10000L ) % 100L ));
  return ( Wire.endTransmission( true ) == 0 );
}

void get_Time( int8_t address, uint8_t &o_hour, uint8_t &o_min, uint8_t &o_sec )    // return 3 hex bytes
{

  Wire.beginTransmission( COUNTER_ADDRESS );
  Wire.write( address );
  Wire.endTransmission( );
  Wire.requestFrom( COUNTER_ADDRESS, 3 );

  o_hour   = Wire.read( );         // get Hour
  o_min    = Wire.read( );         // get minute
  o_sec    = Wire.read( );         // get second

}

bool set_Time( int8_t address, uint8_t o_hour, uint8_t o_min, uint8_t o_sec )            // store as 3 hex bytes
{
  Wire.beginTransmission( COUNTER_ADDRESS );
  Wire.write( address );
  Wire.write( o_hour );
  Wire.write( o_min  );
  Wire.write( o_sec  );
  return ( Wire.endTransmission( true ) == 0 );
}

void get_Date( int8_t address, uint8_t &day, uint8_t &month, uint8_t &year )
{
  uint8_t value;

  Wire.beginTransmission( COUNTER_ADDRESS );
  Wire.write( address  );
  Wire.endTransmission();
  Wire.requestFrom( COUNTER_ADDRESS, 3 );

  day   = Wire.read( );                       // get day
  month = Wire.read( );                       // get month
  year  = Wire.read( ) + year_offset;         // get year

}

bool set_Date( int8_t address, uint8_t day, uint8_t month, int year )
{
  uint8_t value;

  Wire.beginTransmission( COUNTER_ADDRESS );
  Wire.write( address  );
  Wire.write( day ) ;
  Wire.write( month );
  year = year - year_offset ;
  Wire.write( year );
  return ( Wire.endTransmission( true ) == 0 );

}

bool set_month( int8_t month, int32_t new_count ) 
{
   uint8_t  month_address     = 0;
   uint32_t month_consumption = 0;
    
   month_address      = ADDRESS_MONTH_01  + ( month - 1 ) * 4;            // calculate address for current month
   month_consumption  = start_consumption + new_count * liter_per_count ; // up to date consumptiion
   return set_My_4_bytes( month_address, month_consumption );
}

int32_t get_month( int8_t month ) 
{
   uint8_t  month_address     = 0;

   month_address      = ADDRESS_MONTH_01  + ( month - 1 ) * 4;            // calculate address for current month
   if ( get_My_4_bytes( month_address, temp_data ) != true ) temp_data = 0 ;
   return temp_data;
}

//
//**************************************************************************************************
//                                          L O G P R I N T                                        *
//**************************************************************************************************
// Log messages if log_msg is true                                                                 *
// Uses vsnprint for format strings, needs /r/n for CR/LF at end                                   *
//**************************************************************************************************
//
char* logprint ( const char* format, ... )
{
  static char sbuf[ 128 ] ;                            // For debug lines
  va_list     varArgs ;                                // For variable number of params

  va_start ( varArgs, format ) ;                       // Prepare parameters
  vsnprintf ( sbuf, sizeof(sbuf), format, varArgs ) ;  // Format the message
  va_end ( varArgs ) ;                                 // End of using parameters
  if ( log_msg )                                       // Log Messages on ?
  {
    Serial.print ( sbuf ) ;                            // and the info
  }
  return sbuf ;                                        // Return stored string
}

//
// Init RTC Counter
//
void Init_counter( )
{
  logprint( "***  Set Counter Mode *******" );
  if ( set_Counter_Mode( ) == false )
     logprint( "***  failed to set Counter Mode *******");        // only needed for first init
  logprint( "***  Clear counter data *******" );
  if ( set_Counter    ( ADDRESS_COUNTER, 0 ) == false )
     logprint( "***  Failed to clear counter *******");           // do not clear counter after reset
  if ( set_My_4_bytes ( ADDRESS_OLD_COUNTER, 0 ) == false )
     logprint( "***  Failed to clear old counter data *******");  // do not clear counter after reset
  //set_My_4_bytes ( ADDRESS_START_METER, 0 );                   // do not clear start consumption after reset
  logprint( "***  Clear Time *******" );
  set_Time       ( ADDRESS_OLD_TIME, 0, 0, 0 );
  //
  // init old time
  //
  logprint( "***  Set Time *******" );
  set_Time( ADDRESS_OLD_TIME, Gas_hour, Gas_min, Gas_sec ); //
  //
}
//
// MQTT commnands
//

void publish_month( )                        // send consumption data for selectetd month

{
   uint8_t  cmd_month = 0;
   uint8_t  cmd_value = 0;
   uint8_t  cmd_start = 0;
   uint8_t  cmd_end   = 0;
   uint32_t cmd_data  = 0;
   String   value3;

   consumption_string = MQTT_command.substring( MQTT_command.indexOf(',') + 1); 
   cmd_value          = consumption_string.toInt();
   cmd_month          = constrain( cmd_value, 1, 12 );
   value3             = ""; 

   if ( cmd_value  <= 12 )            // inside the 1 to 12 limit?
   {
      cmd_start = cmd_month;
      cmd_end   = cmd_month;
   }
   else
   {
      cmd_start = 1;
      cmd_end   = 12;
   }
   for ( int i = cmd_start; i <= cmd_end; i++ )
   {
      get_My_4_bytes( ( ADDRESS_MONTH_01  + ( i - 1 ) * 4 ), cmd_data );
      value3 = value3  + ';' + String( i ) + ';' + String( cmd_data );
   }
   publish_topic( "month_data", value3 );
   
   MQTT_command  = "";
}   
//
void set_period( )                           // set start value for heating period

{
   uint32_t cmd_data  = 0;
   String   value3;
   
   if ( MQTT_command.indexOf(',') > 0 )
   {
      value3    = MQTT_command.substring( MQTT_command.indexOf(',') + 1); 
      cmd_data  = value3.toInt();
   }
   if ( set_My_4_bytes( ADDRESS_START_PERIOD, cmd_data ) == false )
   {
      logprint( "Failed to save Start Period %10d\n\r", MQTT_command );
   }
 
   MQTT_command = "";

}

void set_start( )                           // set start value for heating period

{
   uint32_t cmd_data  = 0;
   String   value3;
   
   if ( MQTT_command.indexOf(',') > 0 )
   {
      value3    = MQTT_command.substring( MQTT_command.indexOf(',') + 1); 
      cmd_data  = value3.toInt();
   }
   if ( set_My_4_bytes( ADDRESS_START_METER, cmd_data ) == false )
   {
      logprint( "Failed to save Start Period %10d\n\r", MQTT_command );
   }
 
   MQTT_command = "";

}

void init_counter_and_data( )                           // set start value for counting and set count = 0

{
   uint32_t cmd_data  = 0;
   String   value3;
   
   Init_counter( );
  
   if ( MQTT_command.indexOf(',') > 0 )
   {
      value3    = MQTT_command.substring( MQTT_command.indexOf(',') + 1); 
      cmd_data  = value3.toInt();
   }
  //
  //   save start consumption frm MQTT to RAM
  //
  if ( cmd_data >= 0 )
  {
     if ( set_My_4_bytes( ADDRESS_START_METER, cmd_data ) == false )
     {
        logprint( "Failed to save Start Consumption %10d\n\r", MQTT_command );
     }
  }
 
   MQTT_command = "";

}
void set_month_data( )                          // set data for a comsumption month

{
   uint32_t cmd_data    = 0;
   uint8_t  cmd_month   = 0;
   uint8_t  cmd_address = 0;
   String   value2;
   String   value3;
 
   value2    = MQTT_command.substring( MQTT_command.indexOf(',') + 1); 
   cmd_month = value2.toInt();
  
   if (( MQTT_command.indexOf(',') > 0   ) &&
       ( cmd_month                       > 0   ) &&
       ( cmd_month                       <= 12 ))
   {
      cmd_data     = 0; 
      if ( MQTT_command.indexOf(';') > 0 )
      {
         value3    = MQTT_command.substring( MQTT_command.indexOf(';') + 1); 
         cmd_data  = value3.toInt();
      }  
      cmd_address  = ADDRESS_MONTH_01  + ( cmd_month - 1 ) * 4;            // calculate address for current month
      set_My_4_bytes( cmd_address, cmd_data );
//      logprint( "Saving %14d to Month %02d at address %08d\n\r", cmd_data, cmd_month, cmd_address );
  }  
   //
   // 
   publish_month( );
   delay( 3000 );
   // 
   MQTT_command = "";

}

void command_parser()

{
  //********* Publish consumption of a single month or month 1 to 12 ***********************
  // SYNTAX  : "Energy/Test/command"
  // PAYLOAD : "Month, x"
  // value for x : either 1 to 12 for a single month, x > 12 shows all 12 months
  //****************************************************************************************
   if ( MQTT_command.indexOf( "Month" ) >= 0 ) 
   {
      publish_month( );          // publish monthly comsumption
   }   
  
  //********* Set consumption for the start of a heating period ****************************
  // SYNTAX  : "Energy/Test/command"
  // PAYLOAD : "InitP, x"
  // value for x : consumption in liter
  //****************************************************************************************
  if ( MQTT_command.indexOf( "InitP" ) >= 0 ) 
   {
      set_period( );             // set period start value
   }   

  //********* Set initial consumption value for count  value of zero ***********************
  // SYNTAX  : "Energy/Test/command"
  // PAYLOAD : "InitS, x"
  // value for x : consumption in liter
  //****************************************************************************************
  if ( MQTT_command.indexOf( "InitS" ) >= 0 ) 
   {
      set_start( );             // set period start value
   }   

    //********* Set consumption for a single month to init data   ****************************
  // SYNTAX  : "Energy/Test/command"
  // PAYLOAD : "InitM, x ; y"
  // value for x : selected month
  // value for y : consumption in liter
  //****************************************************************************************
    if ( MQTT_command.indexOf( "InitM" ) >= 0 ) 
   {
      set_month_data( );        // set data for a comsumption month
   }   

  //********* Set consumption for the strat of a counting period ***************************
  // Initialyse RTC chip and set initial consumption value for count = 0of a counting period 
  // SYNTAX  : "Energy/Test/command"
  // PAYLOAD : "InitC, x"
  // value for x : consumption in liter
  //****************************************************************************************
     if ( MQTT_command.indexOf( "InitC" ) >= 0 ) 
   {
      init_counter_and_data( );        // init counter
   }   
//
// enter more commands here
//
}


void setup()
{
  Serial.begin( 115200 );
  delay(1000);
  pinMode( RESET_PIN, INPUT_PULLUP );             // pull to ground if you want to clear the RTC Data
  pinMode( UPDATE_PIN, INPUT_PULLUP );            // pull to ground if you want prevent ESP going to sleep
  logprint( "\r\nStarting Test!\r\n");
  //
  Wire.setClock( 50000 );                        // reduce SCL speed to 50kHz
  Wire.begin( );
  //
  // connect to WiFi
  //
  Gas_WiFi_connect();
  delay (1000 );
  logprint( "\r\nConnected to WiFi!\r\n");
  //
  // read time from NTP server
  //
  begin_time( );
  //
  delay (1000 );
  //
  // Init Thingsspeak
  //
  #ifdef THINGSPEAK 
    ThingSpeak.begin( client1 );  // Initialize ThingSpeak
  #endif  
  //
  // Init RTC Counter if Pin set
  //
  if ( digitalRead( RESET_PIN ) == 0 )
  {
    logprint( "\r\nClear RTC Data in RAM!\r\n");
    Init_counter( );
  }
  //
  // seup MQTT Client
  //
  setup_MQTT( );
  //
  //   get start consumption
  //
  if ( get_My_4_bytes( ADDRESS_START_METER, start_consumption ) != true ) start_consumption = 0 ;
  //
  //   get period consumption
  //
  if ( get_My_4_bytes( ADDRESS_START_PERIOD, start_period )     != true ) start_period      = 0 ;
  //
  //
}

void loop()
{
   mqtt.loop();
   error = false;           // reset error flag
  //
  // check WiFi connection
  //
  if ( WiFi.status() != WL_CONNECTED )
    Gas_WiFi_reconnect();
  //
  // parse MQTT commands
  //
  if ( MQTT_command != "" ) command_parser ();
  //
  // read current time from ESP
  //
  get_LocalTime( );
  logprint( "Datum %02d.%02d.%04d Uhrzeit  %02d:%02d:%02d\n\r", Gas_day, Gas_month, Gas_year, Gas_hour, Gas_min, Gas_sec );
  //
  //   read current counter from counter location
  //
  if ( get_Counter( ADDRESS_COUNTER, new_count ) != true ) 
  {
     error     = true;
     new_count = 0;                  // to do
  }   
  //
  //   read old counter from RAM
  //
  if ( get_My_4_bytes( ADDRESS_OLD_COUNTER, old_count ) != true ) old_count = 0 ;
  //
  logprint( "***  New Count %10d Old Count %10d\n\r", new_count, old_count );

  //
  //   get start consumption
  //
  if ( get_My_4_bytes( ADDRESS_START_METER, start_consumption ) != true ) start_consumption = 0 ;
  //
  //   get period consumption
  //
  if ( get_My_4_bytes( ADDRESS_START_PERIOD, start_period ) != true ) start_period = 0 ;
  //
  //   save current counter to RAM
  //
  if ( set_My_4_bytes( ADDRESS_OLD_COUNTER, new_count ) == false )
  {
    logprint( "Failed to save New to Old Count ***  New Count %10d Old Count %10d\n\r", new_count, old_count );
  }
  //
  // read last time
  //
  get_Time( ADDRESS_OLD_TIME, old_hour, old_min, old_sec );
  //
  // save Date & time of current read  in RAM
  // TO DO check whether period per read or per xmitt success
  if ( set_Time( ADDRESS_OLD_TIME, Gas_hour, Gas_min, Gas_sec ) == false )
  {
    logprint( "Failed to save New to Time ***  New Time %02d:%02d:$02d\n\r", Gas_hour, Gas_min, Gas_sec );
  }
  //
  // read date of last readoute
  get_Date( ADDRESS_OLD_DATE, old_day, old_month, old_year );
  //
  // save data for current month
  //
  if ( set_month( Gas_month, new_count) == false )
  {
    logprint( "Failed to save mmonthly data Gas_month %02d New Count:%06d\n\r", Gas_month, new_count );
  }
  //
  //
  set_Date( ADDRESS_OLD_DATE, Gas_day, Gas_month, Gas_year );
  // calculate period between 2 reads
  period =   ( Gas_hour - old_hour ) * 3600L +
             ( Gas_min  - old_min  ) *   60  +
             ( Gas_sec  - old_sec );
  new_time = ( Gas_hour ) * 3600L +
             ( Gas_min  ) *   60  +
             ( Gas_sec  );
  old_time = ( old_hour ) * 3600L +
             ( old_min  ) *   60  +
             ( old_sec );
  //
  logprint( "Old Time %02d:%02d:%02d New Time %02d:%02d:%02d Period %10d\n\r",
             old_hour, old_min, old_sec, Gas_hour, Gas_min, Gas_sec, period );
  logprint( "Old Time %08d New time %06d  in secondes\n\r", old_time, new_time );
  // calculate counts between two read outs
  delta_count       = new_count - old_count;
  //
  // calculate absolute consumption bewteen two reads
  //
  energy1      = ( mexp3_per_count * brennwert * zustandszahl * delta_count )      ; // in kWh  : Gas m³ * Brennwert * Zustandszahl
  total_energy = ( mexp3_per_count * brennwert * zustandszahl * new_count )        ; // in kWh  : Gas m³ * Brennwert * Zustandszahl
  consumption  = ( start_consumption + new_count * liter_per_count ) / 1000.0      ; // up to date consumptiion
  
  dtostrf( total_energy,     12, 3, valueString1 );
  dtostrf( energy1,          12, 3, valueString2 );
  dtostrf( consumption,      12, 3, valueString3 );
  dtostrf( new_count,        10, 0, valueString4 );
  //dtostrf( new_time,         10, 0, valueString6 );
  //dtostrf( old_time,         10, 0, valueString7 );
  dtostrf( ( 3600L / ( new_time - old_time ) * energy1 ),  12, 3, valueString5  );
  dtostrf( ( start_consumption / 1000.0 ),                 12, 3, valueString6  );
  dtostrf( ( start_period / 1000.0 ),                      12, 3, valueString7 );
  sprintf( valueString8, "%02d:%02d:%02d", Gas_hour, Gas_min, Gas_sec );
  //
  // calculate consumption per hour or minute
  //
#ifdef THINGSPEAK 
{
   //
   // write data to the ThingSpeak channel
   //
   ThingSpeak.setField( Thingspeak_1, valueString1 );            // total consumption
   ThingSpeak.setField( Thingspeak_2, valueString2  );           // consumption between two readouts
   ThingSpeak.setField( Thingspeak_4, valueString4  );           // total counts 
   ThingSpeak.setField( Thingspeak_5, valueString3  );           // Gas m³
   // send data to Thingspeak fields
   Send_data( );
}
#endif
 
  //if ( old_min != Gas_min )
  {  
  //
  // publish to MQTT
  //
    publish_topic( "Time", valueString8 );
    publish_topic( "Total_kWh", valueString1 );
    publish_topic( "Power_kW", valueString5 );
    publish_topic( "Counts", valueString4 );
    publish_topic( "Volume", valueString3 );
    publish_topic( "Start", valueString6 );
    publish_topic( "Period_Start", valueString7 );
    if ( error == true )
      publish_topic( "Status", "Error" );
    else
      publish_topic( "Status", "OK" );
  //
  }  
  //
  //  check if Sleep or wait
  //
  if ( digitalRead( UPDATE_PIN ) == 1 )      // pull UPDATE ptin LOW to prevent sleep
  {
    logprint( "Going to sleep\n" );
    delay( 1000 );
    ESP.deepSleep( 111 * 1000 * 1000 );
  }
  else
  {
    logprint( "Waiting 30 seconds\n" );
    delay( 30000 );
  }
}
