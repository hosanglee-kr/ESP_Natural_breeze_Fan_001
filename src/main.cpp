#include <Arduino.h>


#define F10
#ifdef F10
	#include "F10/F10_fan_004.h"
#endif


#define W10
#ifdef W10
	#include "W10_test/W10_Advanced2_005.h"
	// #include "W10_test/W10_Advanced_004.h"
#endif

#define W20
#ifdef W20
	#include "W10_test/W20_AutoConnectNonBlockingwParams_001.h"
#endif

#define W30
#ifdef W30
	#include "W10_test/W30_onDemandNonBlocking_001.h"
#endif

#define W40
#ifdef W40
	#include "W10_test/W40_Super_OnDemandConfigPortal_001.h"
#endif


void setup() {

	Serial.begin(115200);
	// delay(5000);


    #ifdef F10
	    F10_init();
    #endif

	
	#ifdef W10
		W10_init();
	#endif

	#ifdef W20
		W20_init();
	#endif

	#ifdef W30
		W30_init();
	#endif

	#ifdef W40
		W40_init();
	#endif

	//Serial.println("11111");
}

void loop() {
    #ifdef F10
	    F10_run();
    #endif

	#ifdef W10
		W10_run();
	#endif

	#ifdef W20
		W20_run();
	#endif

	#ifdef W30
		W30_run();
	#endif

	#ifdef W40
		W40_run();
	#endif
}
