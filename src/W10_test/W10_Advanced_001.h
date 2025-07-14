
// W10_Advanced_001.h
/**
 * WiFiManager advanced demo, contains advanced configurartion options
 * Implements TRIGGEN_PIN button press, press for ondemand configportal, hold for 3 seconds for reset settings.
 */
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager

#define G_W10_TRIGGER_PIN 0

// wifimanager can run in a blocking mode or a non blocking mode
// Be sure to know how to process loops with no delay() if using non blocking
bool				 g_W10_wm_nonblocking = false;  // change to true to use non blocking

WiFiManager			 g_W10_WifiManager;			// global g_W10_WifiManager instance
WiFiManagerParameter g_W10_custom_field;	// global param ( for non blocking w params )




void	W10_checkButton();
String	W10_getParam(String name);
void	W10_saveParamCallback();


void W10_init() {

	WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
	
	// Serial.begin(115200);
	// Serial.setDebugOutput(true);
	// delay(3000);
	// Serial.println("\n Starting");

	pinMode(G_W10_TRIGGER_PIN, INPUT);

	// g_W10_WifiManager.resetSettings(); // wipe settings

	if (g_W10_wm_nonblocking) {
		g_W10_WifiManager.setConfigPortalBlocking(false);
	}

	// add a custom input field
	int			customFieldLength = 40;

	// new (&g_W10_custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");

	// test custom html input type(checkbox)
	// new (&g_W10_custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type

	// test custom html(radio)
	const char* custom_radio_str  = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
	new (&g_W10_custom_field) WiFiManagerParameter(custom_radio_str);	 // custom html input

	g_W10_WifiManager.addParameter(&g_W10_custom_field);
	g_W10_WifiManager.setSaveParamsCallback(W10_saveParamCallback);

	// custom menu via array or vector
	//
	// menu tokens, "wifi","wifinoscan","info","param","close","sep","erase","restart","exit" (sep is seperator) (if param is in menu, params will not show up in wifi page!)
	// const char* menu[] = {"wifi","info","param","sep","restart","exit"};
	// g_W10_WifiManager.setMenu(menu,6);
	std::vector<const char*> menu = {"wifi", "info", "param", "sep", "restart", "exit"};
	g_W10_WifiManager.setMenu(menu);

	// set dark theme
	g_W10_WifiManager.setClass("invert");

	// set static ip
	//  g_W10_WifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
	//  g_W10_WifiManager.setShowStaticFields(true); // force show static ip fields
	//  g_W10_WifiManager.setShowDnsFields(true);    // force show dns field always

	// g_W10_WifiManager.setConnectTimeout(20); // how long to try to connect for before continuing
	g_W10_WifiManager.setConfigPortalTimeout(30);	// auto close configportal after n seconds
	// g_W10_WifiManager.setCaptivePortalEnable(false); // disable captive portal redirection
	// g_W10_WifiManager.setAPClientCheck(true); // avoid timeout if client connected to softap

	// wifi scan settings
	// g_W10_WifiManager.setRemoveDuplicateAPs(false); // do not remove duplicate ap names (true)
	// g_W10_WifiManager.setMinimumSignalQuality(20);  // set min RSSI (percentage) to show in scans, null = 8%
	// g_W10_WifiManager.setShowInfoErase(false);      // do not show erase button on info page
	// g_W10_WifiManager.setScanDispPerc(true);       // show RSSI as percentage not graph icons

	// g_W10_WifiManager.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails

	bool res;
	// res = g_W10_WifiManager.autoConnect(); // auto generated AP name from chipid
	// res = g_W10_WifiManager.autoConnect("AutoConnectAP"); // anonymous ap
	res = g_W10_WifiManager.autoConnect("AutoConnectAP", "password");	// password protected ap

	if (!res) {
		Serial.println("Failed to connect or hit timeout");
		// ESP.restart();
	} else {
		// if you get here you have connected to the WiFi
		Serial.println("connected...yeey :)");
	}
}

void W10_checkButton() {
	// check for button press
	if (digitalRead(G_W10_TRIGGER_PIN) == LOW) {
		// poor mans debounce/press-hold, code not ideal for production
		delay(50);
		if (digitalRead(G_W10_TRIGGER_PIN) == LOW) {
			Serial.println("Button Pressed");
			// still holding button for 3000 ms, reset settings, code not ideaa for production
			delay(3000);  // reset delay hold
			if (digitalRead(G_W10_TRIGGER_PIN) == LOW) {
				Serial.println("Button Held");
				Serial.println("Erasing Config, restarting");
				g_W10_WifiManager.resetSettings();
				ESP.restart();
			}

			// start portal w delay
			Serial.println("Starting config portal");
			g_W10_WifiManager.setConfigPortalTimeout(120);

			if (!g_W10_WifiManager.startConfigPortal("OnDemandAP", "password")) {
				Serial.println("failed to connect or hit timeout");
				delay(3000);
				// ESP.restart();
			} else {
				// if you get here you have connected to the WiFi
				Serial.println("connected...yeey :)");
			}
		}
	}
}


String W10_getParam(String name) {
	// read parameter from server, for customhmtl input
	String value;
	if (g_W10_WifiManager.server->hasArg(name)) {
		value = g_W10_WifiManager.server->arg(name);
	}
	return value;
}


void W10_saveParamCallback() {
	Serial.println("[CALLBACK] W10_saveParamCallback fired");
	Serial.println("PARAM customfieldid = " + W10_getParam("customfieldid"));
}

void W10_run() {
	if (g_W10_wm_nonblocking) g_W10_WifiManager.process();  // avoid delays() in loop when non-blocking and other long running code
	W10_checkButton();
	// put your main code here, to run repeatedly:
}
