#include <Arduino.h>

#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <SPI.h>
#include "SdFat.h"
#include "notes.h"
#include "beeps.h"
#include "const.h"

SdFat SD;

char sprintf_buffer [50];

const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
char fileName[] = FILE_BASE_NAME "00.txt";

volatile int interrupt_flag = 0;

int pass_counter = 0;
unsigned long wdt_trigger_counter = 0;
unsigned long last_wdt_counter_measurement = 0;
unsigned long last_wdt_counter_pass = 0;
unsigned long last_wdt_counter_save = 0;
short save_loops = 0;
bool led_state = false;
int entries = 0;
bool last_battery_status = true;

byte data_arr[ENTRY_COUNT];

bool battery_discharged = false;

int fills = 0;

int timing_table[4] = {
  10,
  7,
  4,
  2
};

int stage = 0;


// print to serial
void serial_log(char* log){
  DEBUG_SERIAL.begin(SERIAL_BAUD);
  DEBUG_SERIAL.print(log);
  DEBUG_SERIAL.flush();
  DEBUG_SERIAL.end();
}
void serial_log(char* log, int a){
  DEBUG_SERIAL.begin(SERIAL_BAUD);
  DEBUG_SERIAL.print(log);
  DEBUG_SERIAL.print(a);
  DEBUG_SERIAL.flush();
  DEBUG_SERIAL.end();
}
void serial_log_n(char* log){
  DEBUG_SERIAL.begin(SERIAL_BAUD);
  DEBUG_SERIAL.println(log);
  DEBUG_SERIAL.flush();
  DEBUG_SERIAL.end();
}
void serial_log_n(char* log, int a){
  DEBUG_SERIAL.begin(SERIAL_BAUD);
  DEBUG_SERIAL.print(log);
  DEBUG_SERIAL.println(a);
  DEBUG_SERIAL.flush();
  DEBUG_SERIAL.end();
}
void serial_log_f(int a){
  DEBUG_SERIAL.begin(SERIAL_BAUD);
  DEBUG_SERIAL.write(sprintf_buffer, a);
  DEBUG_SERIAL.flush();
  DEBUG_SERIAL.end();
}

// print all entries to serial
void dump_data_to_serial(){
    DEBUG_SERIAL.begin(SERIAL_BAUD);
    DEBUG_SERIAL.print("\nData dump call: ");
    for(int i = 0; i < entries; i++){
      DEBUG_SERIAL.print('\n');
      DEBUG_SERIAL.print(data_arr[i]);
    }
    DEBUG_SERIAL.flush();
    DEBUG_SERIAL.end();
}

// save all entries to sd card
bool dump_data_to_sd(){

  if(fills == 0){
    while (SD.exists(fileName)) {
      if (fileName[BASE_NAME_SIZE + 1] != '9') {
        fileName[BASE_NAME_SIZE + 1]++;
      } else if (fileName[BASE_NAME_SIZE] != '9') {
        fileName[BASE_NAME_SIZE + 1] = '0';
        fileName[BASE_NAME_SIZE]++;
      } else {
        beep_sad();

        serial_log_n("Can't create file name");

        return false;
      }
    }
  }

  File dataFile = SD.open(fileName, FILE_WRITE);
  
  // if the file is available, write to it:
  if (dataFile) {
    if(fills > 0){

      dataFile.print("C");
      dataFile.print(fills);

      if(battery_discharged){
        dataFile.print("   -- SAVED AT DISCHARGE");
      }
      dataFile.print('\n');

    }else{
      // print header
      if(battery_discharged){
        dataFile.print("-- SAVED AT DISCHARGE --\n");
      }

      dataFile.print("Battery status: ");
      dataFile.print(last_battery_status ? "high" : "low");

      dataFile.print("\nEntry target: ");
      dataFile.print(ENTRY_COUNT);

      dataFile.printf("\nEntry count: ");
      dataFile.print(entries);

      dataFile.print("\nMeasurment duration: ");
      dataFile.print(MEASUREMENT_DURATION);
      dataFile.print("ms\n");

      dataFile.print("C");
      dataFile.print(fills);

      dataFile.println();
    }
    
    for(int i = 0; i < entries; i++){
      dataFile.println(data_arr[i]);
    }
    
    dataFile.close();

    return true;
  }else
  {
    serial_log_n("no data file");

    return false;
  }
}

// configure watchdog timer
void configure_wdt(void)
{
  cli();                           // disable interrupts for changing the registers

  MCUSR = 0;                       // reset status register flags

                                   // Put timer in interrupt-only mode:                                       
  WDTCSR |= 0b00011000;            // Set WDCE (5th from left) and WDE (4th from left) to enter config mode,
                                   // using bitwise OR assignment (leaves other bits unchanged).
  WDTCSR =  0b01000000 | 0b000010; // set WDIE: interrupt enabled
                                   // clr WDE: reset disabled
                                   // and set delay interval (right side of bar) to 8 seconds

  sei();                           // re-enable interrupts
}

// magnet pass interrupt
void Magnet_INT()
{
  interrupt_flag = 1;
  sleep_disable();

  detachInterrupt(digitalPinToInterrupt(MAGNET_INPUT_PIN));
}

// save button press interrupt
void Save_INT()
{
  interrupt_flag = 2;
  sleep_disable();

  detachInterrupt(digitalPinToInterrupt(SAVE_INPUT_PIN));
}

// enable sleep
void sleepNow ()
{
  cli();                                                                       //disable interrupts
  sleep_enable ();                                                      // enables the sleep bit in the mcucr register

  if(!battery_discharged)
    attachInterrupt (digitalPinToInterrupt(MAGNET_INPUT_PIN), Magnet_INT, FALLING);          // wake up on RISING level on D2
  attachInterrupt (digitalPinToInterrupt(SAVE_INPUT_PIN), Save_INT, RISING);          // wake up on RISING level on D2

  set_sleep_mode (SLEEP_MODE_PWR_DOWN); 

  ADCSRA = 0;                                                           //disable the ADC

  sleep_bod_disable();                                                //save power                                             
  sei();                                                                      //enable interrupts
  sleep_cpu ();                                                           // here the device is put to sleep
}

// watchdog interrupt
ISR (WDT_vect) 
{
  interrupt_flag = 0;
  sleep_disable();
}

// flip LED state
void switchLED(){
  led_state = !led_state;
  digitalWrite(LED_PIN, led_state ? HIGH : LOW);
}

// switch LED state
void switchLED(bool state){
  led_state = state;
  digitalWrite(LED_PIN, led_state ? HIGH : LOW);
}

// check battery status
bool battery_status(){
  bool res = digitalRead(VREF_PIN) == HIGH;
  last_battery_status = res;
  return res;
}

void setup() {
  wdt_disable(); //Datasheet recommends disabling WDT right away in case of low probabibliy event

  pinMode(BUZZER_PIN, OUTPUT); // Set buzzer - pin 9 as an output
  pinMode(LED_PIN, OUTPUT); //set up the LED pin to output
  pinMode(MAGNET_INPUT_PIN, INPUT_PULLUP); //setup pin 2 for input since we will be using it with our button  
  pinMode(SAVE_INPUT_PIN, INPUT_PULLUP); //setup pin 2 for input since we will be using it with our button  
  pinMode(VREF_PIN, INPUT_PULLUP);

  delay(1);

  if(!battery_status())
  {
    // save and power-off
    battery_discharged = true;
    dump_data_to_sd();
    
    serial_log_n("Discharged");

    // disable watchdog
    wdt_reset();
    MCUSR=0;
    WDTCSR|=_BV(WDCE) | _BV(WDE);
    WDTCSR=0;

    beep_discharge(20, 2000);
  }
  else{

    // SD Card Initialization
    if (!SD.begin(SD_CS_PIN))
    {
      beep_sad();
      
      serial_log("SD card initialization failed");
        
      // disable watchdog
      wdt_reset();
      MCUSR=0;
      WDTCSR|=_BV(WDCE) | _BV(WDE);
      WDTCSR=0;
      
      return;
    }else{
      beep_short();
      configure_wdt();
    }
    
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    
    sleep_mode();
  }
}

void loop() {
  sleepNow();
  
  delay(1);
  
  //serial_log_n("Woken up, interrupt flag: ", interrupt_flag);

  switch(interrupt_flag){


    // wdt trigger
    case 0:
      if(entries < ENTRY_COUNT){
        if(wdt_trigger_counter - last_wdt_counter_measurement == (int)(MEASUREMENT_DURATION/WDT_AVG_64MS_CYCLE)){
          switchLED(false);

          save_loops++;
          // check voltage
          if(save_loops >= ENTRIES_UNTIL_BATT_CHECK){
            if(!battery_status() && !last_battery_status)
            {
              battery_discharged = true;

              // save and power-off
              dump_data_to_sd();
              
              serial_log_n("Battery discharged");

              beep_discharge(10, 30000);

              // disable watchdog
              wdt_reset();
              MCUSR = 0;
              WDTCSR |= _BV(WDCE) | _BV(WDE);
              WDTCSR = 0;
              goto end;
            }

            //serial_log_n("Battery voltage sufficent");
            
            save_loops = 0;
          }

          //serial_log_n("saving entry ", entries);

          // log entry and reset counter
          data_arr[entries] = pass_counter;
          entries++;
          pass_counter = 0;

          last_wdt_counter_measurement = wdt_trigger_counter;
        } 
      }
      else
      {
        serial_log_f(sprintf(sprintf_buffer, "dumping %d entries\n", entries));
        
        // entries full - save to sd
        if(!dump_data_to_sd())
          beep_sad();

        entries = 0;
        fills++;
      }

      
      wdt_trigger_counter++;
    break;



    // magnet pass
    case 1:{
      int dt = wdt_trigger_counter - last_wdt_counter_pass;
      
      if(dt >= timing_table[stage])
      //if(dt > 2)
      {
        int last_s = stage;

        if(dt <= 2 * timing_table[stage])
          stage = min(stage + 1, 3); // stage++
        else if(dt > 3 * timing_table[stage])
          stage = max(0, stage - 1); // stage--


        serial_log_f(sprintf(sprintf_buffer, "pass detected, dt = %d, stage = %d - %d\n", dt, last_s, stage));
        
        pass_counter++;

        switchLED();

        last_wdt_counter_pass = wdt_trigger_counter;
      }
    }
    break;



    // save button press
    case 2:
      if(battery_discharged){
            beep_discharge(2, 1000);
            serial_log_n("battery discharged");
            break;
      }
      else if((wdt_trigger_counter - last_wdt_counter_save > (int)(500/WDT_AVG_64MS_CYCLE))){
        
        if(dump_data_to_sd()){
          fills++;
          entries = 0;
          beep_short();
          serial_log_n("sd data save successful");
        }
        else{
          beep_sad();
          serial_log_n("sd data save unsuccessful");
        }

        last_wdt_counter_save = wdt_trigger_counter;
      }
    break;
  }
end:{}
}
