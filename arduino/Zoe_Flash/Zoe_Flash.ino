#include <avr/wdt.h>
#include <limits.h>

#define LEDA_OUT_PIN     2
#define LEDB_OUT_PIN     3


#define HALL_SENSE_PIN   8      // Also ICP1 pin in case we want to use the Input Capture Unit for more precision
#define HALL_GND_PIN     9
#define HALL_VCC_PIN    10

// We will use the 16 bit Timer1 as a stopwatch

// Set prescaller to /64. With a 16Mhz clock, this gives us a tick clock of 250KHz, and a tick time of 4uS
// This is a 16 bit timer, so we will overflow at 65536*4us = ~250ms. This is a bout 4Hz, way longer than We need since we will stop blinking at like 10Hz

#define FRAMES_PER_ROTATION (24)        // Depends on the actual lamp shade

/*
unsigned timerHigh =0;

ISR(TIMER1_OVF_vect) {

  timerHigh++;
  TIFR1 |= _BV( TOV1);      // Clear overflow flag
  
}

void initTicks(void) {

  TCCR1A = 0;   // No outputs, normal mode
  
  TCCR1B = _BV( CS12) ;        // Set prescaller to /256 = 16Mhz/256 = 62Khz  /65536 (one full timer cycle) = ~1Hz

  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt  
  
}


#define TICK_PER_SECOND (F_CPU/256)   // Tick freqency after prescaller

// On Arduino clock is 16Mhz so...
// TICKS_PER_SECOND = 16000000 / 256 = 62500
// One tick = 16us
// Maximum time before 16 bit timer overflows is 16us * 65536 = ~1 second which is more than long enough for our purposes. 


static inline void resetTicks(void) {
    cli();
    TCNT1 = 0;        
    timerHigh=0;        // No race here becuase TCNT1 will only be at like 2 by here
    TIFR1 |= _BV( TOV1);      // Clear overflow flag    
    sei();
}


static inline unsigned long ticksNow(void) {

  cli();
  // Stop timer...
  TCCR1B = 0 ;                 // Set prescaller to OFF


  unsigned h=timerHigh;     // Snap values
  unsigned l=TCNT1;

  if (  TIFR1 |= _BV( TOV1) ) {      // check overflow flag - would be set if we overflowed between cli() and turning the timer off. Unlikely, but possible!
      h+=1;
  }

  TCNT1+=10;                  // Account for the cycles we missed whist looking

  // Restart timer...
  TCCR1B = _BV( CS12) ;        // Set prescaller to /256
  sei();

  return ((unsigned long) h << 16) | l;
 
}

*/

void setup() {

  // LED drivers. High to turn on LED modules.
  pinMode( LEDA_OUT_PIN , OUTPUT );
 //DDRD |= _BV(2);

  pinMode( LEDB_OUT_PIN , OUTPUT );
  //DDRD |= _BV(3);

  // US1881 hall effect latch connected to pins 8,9,10. 
  
  pinMode( HALL_GND_PIN , OUTPUT );

  pinMode( HALL_SENSE_PIN , INPUT_PULLUP );
  
  pinMode( HALL_VCC_PIN , OUTPUT );
  digitalWrite( HALL_VCC_PIN , HIGH );    // Power up the hall latch

  //initTicks();
}


void reset(void) {      // TODO: Could just set this to timeout value before looping and then WDT inside the loop to have reset happen naturally. 

  wdt_enable( WDTO_60MS );
  while (1);
  
}


#define A_BUFFER_LEN 100
float a_buffer[A_BUFFER_LEN];
unsigned a_buffer_count=0;

#define MICROS_PER_S      (1000000UL)

#define FRAMES_PER_R 24    // Number of flashes per rotation

#define INITIAL_A (-3.7E-11)    // Acceleration in rotations/tick^2 - emperically derived

#define MAX_MICROS_PER_R (1UL*MICROS_PER_S)      // If we take more than ! second between clicks, that is 24 flashes/sec, which is pretty harsh

void loop() {

  // Loop is one spin cycle. 

  // We will start flashing when the rotation speed gets fast enough as measured by the tick time not overflowing in one revolution

  uint8_t clickState =  digitalRead( HALL_SENSE_PIN);

  uint8_t currentState = clickState;    // We are currently in the click state. We will wait until the next transition into clickState to start working...
                                   // This prevents a spurrious flash on the first transision.
    
  unsigned long startOfCurrentRotation=0;      // We just woke up so we dont know when it started!
  
  float Vstart_R_PER_U  =0;                      // The instant speed at the start of the current rotation 


  float Vavg_R_PER_U =0;        // Average speed durring previous rotation
  
  unsigned long nextFlash = UINT32_MAX;     // Flash never. 


  float x=0;    // The location (in rotations since last click) where next flash will happen
  unsigned long startOfTime =0;
 
  unsigned long lengthOfLastRotation_U= UINT32_MAX;    // How many ticks did the last rotation take? 

  float a = 0;              // Acceleration but with a running average trimed to the actual data to try to stay cenetred. 
    
  Serial.begin(9600);
 
  unsigned long giveupTime = micros() + MAX_MICROS_PER_R;

  Serial.write('S');

  while (1) {

    unsigned long timeNow = micros();

    if (  currentState != digitalRead( HALL_SENSE_PIN) ) {     // Change of state?

      currentState = !currentState;           // Update currentState to match what it really is (avoid doign another slow digitalRead()

      if (currentState == clickState) {          // Got next click....

         Serial.write('C');

        //Serial.print( "Click time " );
        //Serial.println( timeNow );

        if (startOfCurrentRotation>0) {      // if this is the First rotation then we can't compute last rotation time yet
                   
          lengthOfLastRotation_U = timeNow - startOfCurrentRotation;          // Remember how long that last rotation took 
  
/*
          Serial.print( "timeNow" );
          Serial.println( timeNow );
          
          Serial.print( "startOfCurrentRotation" );
          Serial.println( startOfCurrentRotation );
          
          Serial.print( "lastRotationTime" );
          Serial.println( lengthOfLastRotation_U );              
*/
          // Acceleration is difference in v over time

          //a = (prevous_v - current_v) / timedifference

          float Vavg_New_R_PER_U = 1.0 / lengthOfLastRotation_U;               // The average speed over the last rotation. Becuase of constant rotation this happened exactly in the middle. (in Rotations/Tick)
          
          if (a==0) {     // a never assigned, so First flash should happen now that we have all data!

            nextFlash = timeNow;                // If so, then trigger the first flash 
            x=0;
            startOfTime= timeNow;

            Serial.write('F');
            
          }       


          // Acceleration = differnece in speed between last two averages / time it took 
          
          a = (Vavg_New_R_PER_U - Vavg_R_PER_U) / lengthOfLastRotation_U;

          if (a_buffer_count<A_BUFFER_LEN) {
            a_buffer[a_buffer_count++] = a;
          }
          

          // Don't bother to try to get a interpolated here becuase it is not linear

          Vstart_R_PER_U = Vavg_R_PER_U + ( 0.5 * a * (lengthOfLastRotation_U ) );      // The instantious speed at the end of the last rotation, by adding the acceleration durring the second half or the rotation. (in Rotations/Tick)            

          Vavg_R_PER_U = Vavg_New_R_PER_U;      // Save this avgerage speed for next time

          /*
          Serial.print("lengthOfLastRotation_U:");
          Serial.println(lengthOfLastRotation_U);
    
    
          Serial.print(" Vstart 100R/S:");      // Convert up becuase arduino only prints floats to 2 decimals!!!!
          Serial.println(Vstart_R_PER_U * MICROS_PER_S * 100);

          */
          
         //Serial.print("a=");      // Convert up becuase arduino only prints floats to 2 decimals!!!!
         //Serial.println(a , 20 );

  
        }
        
        startOfCurrentRotation = timeNow;
        
        //frame=0;

        giveupTime = timeNow + MAX_MICROS_PER_R;        // We clicked, so we are still spinning and if we didn't give up yet then we should reset when. we will 

        //Serial.println( lengthOfLastRotation_U);        
        
      }
              
    }

    if ( timeNow >= nextFlash) {     // Time for next flash? Not do not use >+ or else we will catch the TICKS_FOREVER case
       
      // Flash!!!!
      digitalWrite( LEDA_OUT_PIN , HIGH );
      digitalWrite( LEDB_OUT_PIN , HIGH );

      for(unsigned i=20;i>0; i--) {     // Strech the flash longer for slower speeds so we can consistant brightness. 
        _delay_us(10);     // One tick
      }
      digitalWrite( LEDA_OUT_PIN , LOW );
      digitalWrite( LEDB_OUT_PIN , LOW );

      // Now that the flash is done, we have time to calculate when to flash again based on the last rotation period and the current frame


/*
      float Vavg_R_PER_U = 1.0 / lengthOfLastRotation_U;               // The average speed over the last rotation. Becuase of constant rotation this happened exactly in the middle. (in Rotations/Tick)

      Serial.print("lengthOfLastRotation_U:");
      Serial.println(lengthOfLastRotation_U);


      Serial.print(" Vavg 100R/S:");      // Convert up becuase arduino only prints floats to 2 decimals!!!!
      Serial.println(Vavg_R_PER_U * MICROS_PER_S * 100);
*/

      x += (1.0/FRAMES_PER_R);        // New time we should flash on next frame....

      // Now we have to use the quadratic equaltion to solve for time (in Ticks) when position will be x (in Rotations)

      nextFlash =  startOfTime + ( (-1.0 * Vstart_R_PER_U) + sqrt( ( Vstart_R_PER_U * Vstart_R_PER_U ) + ( 2.0 * a * x ))) / a;
             
      //Serial.print(" df:");
      //Serial.println( nextFlash - timeNow );
      
  //    Serial.print(" frame:");
  //    Serial.println(frame);

      // TODO: Compute how long the flash durration should be to light for, say, 100th of the frame. 

      if (micros()>=nextFlash) {
        //Serial.println("EXPIRE!!!!!!!!!");
        
      }
  
    }

    
    if (timeNow>giveupTime) {

      Serial.println("a buffer");

      for(int i=0; i<a_buffer_count;i++ ){
          Serial.println(a_buffer[i],20);        
      }
//        Serial.println("GIVE UP!!!!!!!!!");
       
      return;                   // This will enter the loop() again and We will wait to for a full rotation to start again.
    }
    
    //Serial.print("T:");
    //Serial.println(timeNow);

  } // while (1) 
} // loop()

