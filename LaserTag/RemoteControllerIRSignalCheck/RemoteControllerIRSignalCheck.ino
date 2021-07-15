#include <Adafruit_CircuitPlayground.h>

#if !defined(ARDUINO_SAMD_CIRCUITPLAYGROUND_EXPRESS)
  #error "Infrared support is only for the Circuit Playground Express, it doesn't work with the Classic version"
#endif

void setup() {
  CircuitPlayground.begin();
  Serial.begin(9600);

  while (!Serial); // Wait until serial console is opened
  
  CircuitPlayground.irReceiver.enableIRIn(); // Start the receiver
  Serial.println("Ready to receive IR signals");
}

void loop() {
  //Continue looping until you get a complete signal received
  if (CircuitPlayground.irReceiver.getResults()) {
    CircuitPlayground.irDecoder.decode();        //Decode it
    
    // Print IR signal value on Hex
    Serial.print("IR hex value: ");
    Serial.println(CircuitPlayground.irDecoder.valueResults(),HEX);

    // Print IR signal value on Decimal
    Serial.print("IR decimal value: ");
    Serial.println(CircuitPlayground.irDecoder.valueResults());
    CircuitPlayground.irReceiver.enableIRIn();      //Restart receiver
  }
}
