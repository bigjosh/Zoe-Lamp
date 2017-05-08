
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


unsigned timerHigh =0;

ISR(TIMER1_OVF_vect) {

  timerHigh++;
  
}

void initTicks(void) {

  TCCR1A = 0;   // No outputs, normal mode
  
  TCCR1B = _BV( CS12) ;        // Set prescaller to /256

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
    sei();
}


static inline unsigned long ticksNow(void) {


  cli();
  // Stop timer...
  TCCR1B = 0 ;                 // Set prescaller to OFF

  unsigned h=timerHigher;     // Snap values
  unsigned l=TCNT1;

  TCNT1+=10;                  // Account for the cycles we missed whist looking

  // Restart timer...
  TCCR1B = _BV( CS12) ;        // Set prescaller to /256
  sei();

  return ((unsigned long) h << 16) | l;
 
}

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

  initTicks();
}

#define FRAME_COUNT 24    // Number of flashes per rotation

#define A (-3.7E-11)    // Acceleration in rotations/tick^2 - emperically derived

void loop() {

  // Loop is one spin cycle. 

  // We will start flashing when the rotation speed gets fast enough as measured by the tick time not overflowing in one revolution

  uint8_t clickState =  digitalRead( HALL_SENSE_PIN);

  uint8_t lastState = clickState;    // We are currently in the click state. We will wait until the next transition into clickState to start working...

  // Todo: probably ok to just skip on rotation. 

  uint8_t clickCountdown = 2;         // Wait for at least 2 full rotations for speed to settle after hand acceleracion. 

  // Ok, we just started timing the first proper roation period. We don't know anything until this ends.

  resetTicks();
  
  unsigned long startOfRoation=0;      // Just so we can offset the nextFlash - probably a better way to do this. 
  
  unsigned long nextFlash = UINT32_MAX;     // Flash never. 

  unsigned frame = 0;
 
  unsigned lastRotationTime=0;    // How many ticks did the last rotation take? 

  float trimmedA = A;              // Acceleration but with a running average trimed to the actual data to try to stay cenetred. 
    
  Serial.begin(9600);

  while (1) {

    unsigned long timeNow = ticksNow();

    if (  lastState != digitalRead( HALL_SENSE_PIN) ) {     // Change of state?

      lastState = !lastState;

      if (lastState == clickState) {          // Got next click....

        lastRotationTime = timeNow - startOfRotation;          // Remember how long that last rotation took 

      }
              
    }

    if (timeNow > nextFlash) {     // Time for next flash? Not do not use >+ or else we will catch the TICKS_FOREVER case
       
      // Flash!!!!
      digitalWrite( LEDA_OUT_PIN , HIGH );
      digitalWrite( LEDB_OUT_PIN , HIGH );

      for(unsigned i=20;i>0; i--) {     // Strech the flash longer for slower speeds so we can consistant brightness. 
        _delay_us(10);     // One tick
      }
      digitalWrite( LEDA_OUT_PIN , LOW );
      digitalWrite( LEDB_OUT_PIN , LOW );

      // Now that the flash is done, we have time to calculate when to flash again based on the last rotation period and the current frame

      frame++;

      if (frame>=FRAME_COUNT+3) {
        
        nextFlash = TICKS_FOREVER;      // Stop flashing until we get another rotation
        
      } else {

        float Vavg = 1.0 / lastRotationTicks;                   // The average speed over the last rotation. Becuase of constant rotation this happened exactly in the middle. (in Rotations/Tick)
        float Vend = Vavg + ( 0.5 * A * lastRotationTicks );  // The instantious speed at the ned of the last rotation, by adding the acceleration durring the second half or the rotation. (in Rotations/Tick)

        float x = (1.0 * frame)/FRAME_COUNT;                  // Where will the next flash happen? (in Rotations)

        // Now we have to use the quadratic equaltion to solve for time (in Ticks) when position will be x (in Rotations)

        nextFlash =  ( (-1.0 * Vend) + sqrt( ( Vend * Vend ) + ( 2.0 * A * x ))) / A;


        /*
        Serial.print("NextFlash:");
        Serial.print(nextFlash);

        Serial.print(" LastRoationTicks:");
        Serial.print( lastRotationTicks);
        Serial.print(" Vend:");
        Serial.print(Vend);


        
        Serial.print(" frame:");
        Serial.println(frame);

        */
        // TODO: Compute how long the flash durration should be to light for, say, 100th of the frame. 

      }
          
    } 
    
    
    //Serial.print("T:");
    //Serial.println(timeNow);

  } // while (1) 
} // loop()

