
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

void initTicks(void) {

  TCCR1A = 0;   // No outputs, normal mode
  
  TCCR1B = _BV( CS12) ;        // Set prescaller to /256
  
}


#define TICK_PER_SECOND (F_CPU/256)   // Tick freqency after prescaller

// On Arduino clock is 16Mhz so...
// TICKS_PER_SECOND = 16000000 / 256 = 62500
// One tick = 16us
// Maximum time before 16 bit timer overflows is 16us * 65536 = ~1 second which is more than long enough for our purposes. 


static inline void resetTicks(void) {
    TCNT1 = 0; 
    TIFR1 |= _BV( TOV1);      // Clear overflow flag
}

#define TICKS_FOREVER (UINT16_MAX)      // A Tick time in the infinite future. 

static inline ticksOverflow(void) {
  return( TIFR1 & _BV( TOV1) );
}

static inline unsigned int ticksNow(void) {

  uint16_t now = TCNT1;     /// Capture time quickly...
  
  if ( TIFR1 & _BV( TOV1) ) {      // Overflow flag set
      return( TICKS_FOREVER );
  }
  
  return( now );
}

// Has the time `when` past yet?
static inline uint8_t ticksYet( unsigned int when ) {

  return( ticksNow() > when );
  
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

#define A (-2.7E-11)    // Acceleration in rotations/tick^2

void loop() {

  // Loop is one spin cycle. 

  // We will start flashing when the rotation speed gets fast enough as measured by the tick time not overflowing in one revolution

  uint8_t lastState =  digitalRead( HALL_SENSE_PIN);

  while (lastState ==  digitalRead( HALL_SENSE_PIN));     // Wait for first transision

  // Ok, we just started timing the first proper roation period. We don't know anything until this ends.

  resetTicks();
  
  lastState = !lastState;            // Changed!
  
  uint8_t clickState = lastState;    // This is the state transision that we will trigger on (we never know which of the 2 transision we might actually see first on a given spin)

  unsigned nextFlash = TICKS_FOREVER;            // First flash should happen now!  
  uint8_t frame = 0;
  
  unsigned lastRotationTicks=0;    // How many ticks did the last rotation take? 

  unsigned startOfRoation=0;      // Just so we can offset the nextFlash - probably a better way to do this. 

  //float trimmedA = A;              // Acceleration but with a running average trimed to the actual data to try to stay cenetred. 
  
  // Note that both lastflash and last clicked are normalized back down to the lowest value afer each click
  
  Serial.begin(9600);

  while (1) {

    unsigned timeNow = ticksNow();

    if (timeNow == TICKS_FOREVER) {       // Took too long, start over from beging
      return;
    }

    if (  lastState != digitalRead( HALL_SENSE_PIN) ) {     // Change of state?

      lastState = !lastState;

      if (lastState == clickState) {        // Got next click....


        // Ok, we normalize the counter down on each rotation so that it does not overflow.

        TCNT1 -= timeNow;  // TODO: This might not be atomic. 

        // This means that we have to adjust the nextFlash time down by the same ammount
        
        if ( nextFlash == TICKS_FOREVER ) {     // IF we are just starting, then do first flash!

          nextFlash = timeNow;

        } else {                                // Otherwise normalize the pending flash

          nextFlash -= timeNow;

        }
    
        lastRotationTicks = timeNow;          // Remeber how long that last rotation took 

        timeNow =0;                   
        frame=0;                              // We are at the very begining of this rotation
          
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

