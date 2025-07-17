/**
 * OnDemandNonBlocking.ino
 * example of running the webportal or configportal manually and non blocking
 * trigger pin will start a webportal for 120 seconds then turn it off.
 * g_W30_startAP = true will start both the configportal AP and webportal
 */
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager

// include MDNS
#ifdef ESP8266
	#include <ESP8266mDNS.h>
#elif defined(ESP32)
	#include <ESPmDNS.h>
#endif

// select which pin will trigger the configuration portal when set to LOW
#define G_W30_TRIGGER_PIN 0

WiFiManager	 g_W30_WifiManager;

unsigned int g_W30_timeout	   = 120;  // seconds to run for
unsigned int g_W30_startTime	   = millis();
bool		 g_W30_portalRunning = false;
bool		 g_W30_startAP	   = false;	 // start AP and webserver if true, else start only webserver


void W30_doWiFiManager();


void W30_init() {
	WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
	
	// // put your setup code here, to run once
	// Serial.begin(115200);
	// Serial.setDebugOutput(true);
	// delay(1000);
	// Serial.println("\n Starting");


	pinMode(G_W30_TRIGGER_PIN, INPUT_PULLUP);

	// g_W30_WifiManager.resetSettings();
	g_W30_WifiManager.setHostname("MDNSEXAMPLE");
	// g_W30_WifiManager.setEnableConfigPortal(false);
	// g_W30_WifiManager.setConfigPortalBlocking(false);
	g_W30_WifiManager.autoConnect();
}

void W30_run() {
	#ifdef ESP8266
		MDNS.update();
	#endif

	W30_doWiFiManager();
	// put your main code here, to run repeatedly:
}

void W30_doWiFiManager() {
	// is auto timeout portal running
	if (g_W30_portalRunning) {
		g_W30_WifiManager.process();  // do processing

		// check for timeout
		if ((millis() - g_W30_startTime) > (g_W30_timeout * 1000)) {
			Serial.println("portaltimeout");
			g_W30_portalRunning = false;
			if (g_W30_startAP) {
				g_W30_WifiManager.stopConfigPortal();
			} else {
				g_W30_WifiManager.stopWebPortal();
			}
		}
	}

	// is configuration portal requested?
	if (digitalRead(G_W30_TRIGGER_PIN) == LOW && (!g_W30_portalRunning)) {
		if (g_W30_startAP) {
			Serial.println("Button Pressed, Starting Config Portal");
			g_W30_WifiManager.setConfigPortalBlocking(false);
			g_W30_WifiManager.startConfigPortal();
		} else {
			Serial.println("Button Pressed, Starting Web Portal");
			g_W30_WifiManager.startWebPortal();
		}
		g_W30_portalRunning = true;
		g_W30_startTime	  = millis();
	}
}
