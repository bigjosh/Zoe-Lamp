
#define FETPIN 9     // Must be a PWM pin
#define LEDPIN 13
#define HALLOPIN 2
#define HALLGPIN 3
#define HALLVPIN 4

#define STEADY_ON_A_VALUE 25   // Empreically found to match the brightness level while spinning


// Don't flash less than this freeqncy to avoid seisures
#define MIN_FLASH_FREQ_HZ  20

#define US_PER_S  1000000

// maximum time between flashes to avoid strobe
#define MAX_FLASH_SPACING_US (US_PER_S/MIN_FLASH_FREQ_HZ)

#define FPS_STEPS 3

byte fps_steps[FPS_STEPS] = { 24 , 16 , 12 };

byte current_fps_step = 0; 

#define FLASH_PER_REV   12      // Total flashes per revolution

#define TRIGGER_PER_REV 4       // Number of magents mounted

unsigned flash_per_trigger;

void next_flash_per_trigger() {

  current_fps_step++;

  if (current_fps_step>=FPS_STEPS) current_fps_step=0; 

  flash_per_trigger = fps_steps[current_fps_step] / TRIGGER_PER_REV;  
/*
  Serial.println("step:");
  Serial.println(current_fps_step);  
  Serial.println(fps_steps[current_fps_step]);  
  Serial.println(flash_per_trigger);  
*/
}



void steady_light_on() {

  analogWrite( FETPIN , STEADY_ON_A_VALUE );
  
}

void steady_light_off() {
  
}


void setup() {

  Serial.begin(9600);

  Serial.println("start");

  steady_light_on();
  
  // set the digital pin as output:
  pinMode( FETPIN  , OUTPUT);
  pinMode( HALLVPIN , OUTPUT);
  pinMode( HALLGPIN  , OUTPUT);
  pinMode( HALLOPIN , INPUT_PULLUP );

  pinMode( LEDPIN  , OUTPUT);

  digitalWrite( HALLVPIN , 1 );

  
  
    
}

#define VSAMPLE_COUNT (TRIGGER_PER_REV*2)   // Filter average of last 2 rotations

unsigned long vsamples[VSAMPLE_COUNT];

byte nextvsampleslot=0;

unsigned long vsampletotal=0;

// Whenthis gets to 0 then we know we have not seen a rising count in the sample buffer
byte fallingvsamplecountdown =VSAMPLE_COUNT; 

void addvsample( unsigned long s ) {

 // Serial.println("remove:");
 // Serial.println(vsamples[ nextvsampleslot ]);
  
  //Serial.println("add");
  //Serial.println(s);
  
  vsampletotal-= vsamples[ nextvsampleslot ];

  vsampletotal += s;
  
  //Serial.println("total:");
  //Serial.println(vsampletotal / VSAMPLE_COUNT);

  vsamples[ nextvsampleslot ] = s;

  nextvsampleslot++;

  if (nextvsampleslot==VSAMPLE_COUNT) nextvsampleslot = 0;

}




// Speed in deg/sec

float v=0;

unsigned long last_micros=0;

unsigned long int nextflash= UINT32_MAX;

unsigned long int lastflash= UINT32_MAX;

unsigned long int lasttrigger= UINT32_MAX;

unsigned long pace;   // how long between flashes? in us

unsigned long lastdiff=0;

unsigned d;   // deceleration per flash 

unsigned long giveuptime = 0;   // When we stop flashing (catches if the spin suddently stopped) 

byte flip=0;


long push=0;

bool flashing_flag  = false;    // Are we currently animating? If no, then show steady state


void loop() {

/*

  digitalWrite( FETPIN , 1 );

  delay(100);
  
  digitalWrite( FETPIN , 0 );

  delay(100);

  return;

  */

    unsigned long now = micros();   

    if ( digitalRead( HALLOPIN ) ) {
      if (flip==0) {
        flip=1;

        unsigned long diff = now - last_micros; 

        addvsample( diff ); 

        v = (lastdiff - diff);

        // V is how much less this diff is from the last one, so represents the deceleration


        lastdiff = diff;

        //v = (360.0/4) / ( diff  / 1000.0 ); 

 //       Serial.println(now);
//        Serial.println(v);

        last_micros = now; 

        if ( now > giveuptime) {
        
          nextflash=now;
          push=0;

          next_flash_per_trigger();

        } 

        pace = ((vsampletotal / VSAMPLE_COUNT) / flash_per_trigger);


        giveuptime = now + ( 2 * MAX_FLASH_SPACING_US * flash_per_trigger ) ;   // If we miss miss a trigger then stop flashing. 

        /*
        Serial.println("trigger:");
        Serial.println(now);
        Serial.println(giveuptime);
        */
               
       
      }
      digitalWrite( LEDPIN , 1 );
    } else {
      digitalWrite( LEDPIN , 0);
      flip=0;
    }

    if ( (now>=nextflash) && (pace < MAX_FLASH_SPACING_US) ) {    // Don't flash slower than once per 50ms (20Hz) 

        if (flashing_flag) {
       
          digitalWrite(FETPIN , 1);
          _delay_ms(1);
          digitalWrite(FETPIN , 0);

        } else {
          
          flashing_flag = true; 
          
        }
        lastflash = now; 

        nextflash += pace  ;
       
        //Serial.println("flash:");
        //Serial.println(nextflash);
        
            
    }

    if  (now >  giveuptime ) {

      if (flashing_flag) {

        steady_light_on();
        flashing_flag = false; 

      }


       nextflash= UINT32_MAX;

    }
    
    
    
}
