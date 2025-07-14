#include <Arduino.h>


#define F10
#ifdef F10
	#include "F10/F10_fan_003.h"
#endif

void setup() {
	// delay(5000);

	

    #ifdef F10
	    F10_init();
    #endif

	Serial.println("11111");
}

void loop() {
    #ifdef F10
	    F10_run();
    #endif
}
