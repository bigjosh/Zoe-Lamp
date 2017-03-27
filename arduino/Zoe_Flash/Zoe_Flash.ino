
#define LEDA_OUT_PIN     2
#define LEDB_OUT_PIN     3

#define HALL_GND_PIN     8
#define HALL_SENSE_PIN   9
#define HALL_VCC_PIN    10


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
  
}

void loop() {
  // put your main code here, to run repeatedly:

  while (1) {

    while ( digitalRead( HALL_SENSE_PIN) );
    digitalWrite( LEDA_OUT_PIN , HIGH );
    digitalWrite( LEDB_OUT_PIN , HIGH );
    
    PORTD = _BV(2) | _BV(3);    // Both sets of LEDs on 
    _delay_us(3000);
    //_delay_ms( (1000/60) );
    while ( !digitalRead( HALL_SENSE_PIN) );
    digitalWrite( LEDA_OUT_PIN , LOW );
    digitalWrite( LEDB_OUT_PIN , LOW );
       
  }
}
