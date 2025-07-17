#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager

WiFiManager			 g_W20_WifiManager;
WiFiManagerParameter g_W20_custom_mqtt_server("server", "mqtt server", "", 40);


void W20_saveParamsCallback();


void W20_init() {
	WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
	// put your setup code here, to run once:
	//Serial.begin(115200);

	// reset settings - wipe credentials for testing
	// g_W20_WifiManager.resetSettings();
	g_W20_WifiManager.addParameter(&g_W20_custom_mqtt_server);
	g_W20_WifiManager.setConfigPortalBlocking(false);
	g_W20_WifiManager.setSaveParamsCallback(W20_saveParamsCallback);

	// automatically connect using saved credentials if they exist
	// If connection fails it starts an access point with the specified name
	if (g_W20_WifiManager.autoConnect("AutoConnectAP")) {
		Serial.println("connected...yeey :)");
	} else {
		Serial.println("Configportal running");
	}
}

void W20_run() {
	g_W20_WifiManager.process();
	// put your main code here, to run repeatedly:
}

void W20_saveParamsCallback() {
	Serial.println("Get Params:");
	Serial.print(g_W20_custom_mqtt_server.getID());
	Serial.print(" : ");
	Serial.println(g_W20_custom_mqtt_server.getValue());
}
