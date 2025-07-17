/**
 * This is a kind of unit test for DEV for now
 * It contains many of the public methods
 *
 */
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <stdio.h>
#include <time.h>

#define USEOTA
// enable OTA
#ifdef USEOTA
	#include <ArduinoOTA.h>
	#include <WiFiUdp.h>
#endif

const char	 *g_W40_wifi_modes[] = {"NULL", "STA", "AP", "STA+AP"};

unsigned long g_W40_mtime	  = 0;

WiFiManager	  g_W40_WifiManager;

// TEST OPTION FLAGS
bool		  g_W40_TEST_CP		  = false;	// always start the configportal, even if ap found
int			  G_W40_TESP_CP_TIMEOUT = 90;		// test cp timeout

bool		  G_W40_TEST_NET		  = true;  // do a network test after connect, (gets ntp time)
bool		  G_W40_ALLOWONDEMAND	  = true;  // enable on demand
int			  G_W40_ONDDEMANDPIN	  = 0;	   // gpio for button
bool		  G_W40_WMISBLOCKING	  = true;  // use blocking or non blocking mode, non global params wont work in non blocking

uint8_t		  G_W40_BUTTONFUNC	  = 1;	// 0 resetsettings, 1 configportal, 2 autoconnect

// char ssid[] = "*************";  //  your network SSID (name)
// char pass[] = "********";       // your network password


void W40_saveWifiCallback();
void W40_configModeCallback(WiFiManager *myWiFiManager);
void W40_saveParamCallback();
void W40_bindServerCallback() ;
void W40_handleRoute();
void W40_handleNotFound();
void W40_handlePreOtaUpdateCallback();
void W40_wifiInfo() ;
void W40_getTime();
void W40_debugchipid();



// callbacks
//  called after AP mode and config portal has started
//   setAPCallback( std::function<void(WiFiManager*)> func );
//  called after webserver has started
//   setWebServerCallback( std::function<void()> func );
//  called when settings reset have been triggered
//   setConfigResetCallback( std::function<void()> func );
//  called when wifi settings have been changed and connection was successful ( or setBreakAfterConfig(true) )
//   setSaveConfigCallback( std::function<void()> func );
//  called when saving either params-in-wifi or params page
//   setSaveParamsCallback( std::function<void()> func );
//  called when saving params-in-wifi or params before anything else happens (eg wifi)
//   setPreSaveConfigCallback( std::function<void()> func );
//  called just before doing OTA update
//   setPreOtaUpdateCallback( std::function<void()> func );

void W40_saveWifiCallback() {
	 Serial.println("[CALLBACK] saveCallback fired");
}


// gets called when WiFiManager enters configuration mode
void W40_configModeCallback(WiFiManager *myWiFiManager) {
	Serial.println("[CALLBACK] W40_configModeCallback fired");
	// myWiFiManager->setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
	// Serial.println(WiFi.softAPIP());
	// if you used auto generated SSID, print it
	// Serial.println(myWiFiManager->getConfigPortalSSID());
	//
	// esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
}





void W40_saveParamCallback() {
	Serial.println("[CALLBACK] W40_saveParamCallback fired");
	// g_W40_WifiManager.stopConfigPortal();
}

void W40_bindServerCallback() {
	g_W40_WifiManager.server->on("/custom", W40_handleRoute);

	// you can override g_W40_WifiManager route endpoints, I have not found a way to remove handlers, but this would let you disable them or add auth etc.
	// g_W40_WifiManager.server->on("/info",W40_handleNotFound);
	// g_W40_WifiManager.server->on("/update",W40_handleNotFound);
	g_W40_WifiManager.server->on("/erase", W40_handleNotFound);  // disable erase
}


void W40_handleRoute() {
	Serial.println("[HTTP] handle custom route");
	g_W40_WifiManager.server->send(200, "text/plain", "hello from user code");
}

void W40_handleNotFound() {
	Serial.println("[HTTP] override handle route");
	g_W40_WifiManager.handleNotFound();
}

void W40_handlePreOtaUpdateCallback() {
	Update.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("CUSTOM Progress: %u%%\r", (progress / (total / 100)));
	});
}

void W40_init() {
	// WiFi.mode(WIFI_STA); // explicitly set mode, esp can default to STA+AP

	// put your setup code here, to run once:
	// Serial.begin(115200);
	// delay(3000);

	// Serial.setDebugOutput(true);

	// WiFi.setTxPower(WIFI_POWER_8_5dBm);

	Serial.println("\n Starting");
	// WiFi.setSleepMode(WIFI_NONE_SLEEP); // disable sleep, can improve ap stability

	Serial.println("Error - TEST");
	Serial.println("Information- - TEST");

	Serial.println("[ERROR]  TEST");
	Serial.println("[INFORMATION] TEST");

	// WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN); // wifi_scan_method_t scanMethod
	// WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL); // wifi_sort_method_t sortMethod - WIFI_CONNECT_AP_BY_SIGNAL,WIFI_CONNECT_AP_BY_SECURITY
	// WiFi.setMinSecurity(WIFI_AUTH_WPA2_PSK);

	g_W40_WifiManager.setDebugOutput(true, WM_DEBUG_DEV);
	g_W40_WifiManager.debugPlatformInfo();

	// reset settings - for testing
	//  g_W40_WifiManager.resetSettings();
	//  g_W40_WifiManager.erase();

	// setup some parameters

	WiFiManagerParameter custom_html("<p style=\"color:pink;font-weight:Bold;\">This Is Custom HTML</p>");	// only custom html
	WiFiManagerParameter custom_mqtt_server("server", "mqtt server", "", 40);
	WiFiManagerParameter custom_mqtt_port("port", "mqtt port", "", 6);
	WiFiManagerParameter custom_token("api_token", "api token", "", 16);
	WiFiManagerParameter custom_tokenb("invalid token", "invalid token", "", 0);												   // id is invalid, cannot contain spaces
	WiFiManagerParameter custom_ipaddress("input_ip", "input IP", "", 15, "pattern='\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}'");  // custom input attrs (ip mask)
	WiFiManagerParameter custom_input_type("input_pwd", "input pass", "", 15, "type='password'");								   // custom input attrs (ip mask)

	const char			 _customHtml_checkbox[] = "type=\"checkbox\"";
	WiFiManagerParameter custom_checkbox("my_checkbox", "My Checkbox", "T", 2, _customHtml_checkbox, WFM_LABEL_AFTER);

	const char			*bufferStr = R"(
  <!-- INPUT CHOICE -->
  <br/>
  <p>Select Choice</p>
  <input style='display: inline-block;' type='radio' id='choice1' name='program_selection' value='1'>
  <label for='choice1'>Choice1</label><br/>
  <input style='display: inline-block;' type='radio' id='choice2' name='program_selection' value='2'>
  <label for='choice2'>Choice2</label><br/>

  <!-- INPUT SELECT -->
  <br/>
  <label for='input_select'>Label for Input Select</label>
  <select name="input_select" id="input_select" class="button">
  <option value="0">Option 1</option>
  <option value="1" selected>Option 2</option>
  <option value="2">Option 3</option>
  <option value="3">Option 4</option>
  </select>
  )";

	WiFiManagerParameter custom_html_inputs(bufferStr);

	// callbacks
	g_W40_WifiManager.setAPCallback(W40_configModeCallback);
	g_W40_WifiManager.setWebServerCallback(W40_bindServerCallback);
	g_W40_WifiManager.setSaveConfigCallback(W40_saveWifiCallback);
	g_W40_WifiManager.setSaveParamsCallback(W40_saveParamCallback);
	g_W40_WifiManager.setPreOtaUpdateCallback(W40_handlePreOtaUpdateCallback);

	// add all your parameters here
	g_W40_WifiManager.addParameter(&custom_html);
	g_W40_WifiManager.addParameter(&custom_mqtt_server);
	g_W40_WifiManager.addParameter(&custom_mqtt_port);
	g_W40_WifiManager.addParameter(&custom_token);
	g_W40_WifiManager.addParameter(&custom_tokenb);
	g_W40_WifiManager.addParameter(&custom_ipaddress);
	g_W40_WifiManager.addParameter(&custom_checkbox);
	g_W40_WifiManager.addParameter(&custom_input_type);

	g_W40_WifiManager.addParameter(&custom_html_inputs);

	// set values later if you want
	custom_html.setValue("test", 4);
	custom_token.setValue("test", 4);

	// const char* icon = "
	// <link rel='icon' type='image/png' sizes='16x16' href='data:image/png;base64,
	// iVBORw0KGgoAAAANSUhEUgAAABAAAAAQBAMAAADt3eJSAAAAMFBMVEU0OkArMjhobHEoPUPFEBIu
	// O0L+AAC2FBZ2JyuNICOfGx7xAwTjCAlCNTvVDA1aLzQ3COjMAAAAVUlEQVQI12NgwAaCDSA0888G
	// CItjn0szWGBJTVoGSCjWs8TleQCQYV95evdxkFT8Kpe0PLDi5WfKd4LUsN5zS1sKFolt8bwAZrCa
	// GqNYJAgFDEpQAAAzmxafI4vZWwAAAABJRU5ErkJggg==' />";

	// set custom html head content , inside <head>
	// examples of favicon, or meta tags etc
	// const char* headhtml = "<link rel='icon' type='image/png' href='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAADQElEQVRoQ+2YjW0VQQyE7Q6gAkgFkAogFUAqgFQAVACpAKiAUAFQAaECQgWECggVGH1PPrRvn3dv9/YkFOksoUhhfzwz9ngvKrc89JbnLxuA/63gpsCmwCADWwkNEji8fVNgotDM7osI/x777x5l9F6JyB8R4eeVql4P0y8yNsjM7KGIPBORp558T04A+CwiH1UVUItiUQmZ2XMReSEiAFgjAPBeVS96D+sCYGaUx4cFbLfmhSpnqnrZuqEJgJnd8cQplVLciAgX//Cf0ToIeOB9wpmloLQAwpnVmAXgdf6pwjpJIz+XNoeZQQZlODV9vhc1Tuf6owrAk/8qIhFbJH7eI3eEzsvydQEICqBEkZwiALfF70HyHPpqScPV5HFjeFu476SkRA0AzOfy4hYwstj2ZkDgaphE7m6XqnoS7Q0BOPs/sw0kDROzjdXcCMFCNwzIy0EcRcOvBACfh4k0wgOmBX4xjfmk4DKTS31hgNWIKBCI8gdzogTgjYjQWFMw+o9LzJoZ63GUmjWm2wGDc7EvDDOj/1IVMIyD9SUAL0WEhpriRlXv5je5S+U1i2N88zdPuoVkeB+ls4SyxCoP3kVm9jsjpEsBLoOBNC5U9SwpGdakFkviuFP1keblATkTENTYcxkzgxTKOI3jyDxqLkQT87pMA++H3XvJBYtsNbBN6vuXq5S737WqHkW1VgMQNXJ0RshMqbbT33sJ5kpHWymzcJjNTeJIymJZtSQd9NHQHS1vodoFoTMkfbJzpRnLzB2vi6BZAJxWaCr+62BC+jzAxVJb3dmmiLzLwZhZNPE5e880Suo2AZgB8e8idxherqUPnT3brBDTlPxO3Z66rVwIwySXugdNd+5ejhqp/+NmgIwGX3Py3QBmlEi54KlwmjkOytQ+iJrLJj23S4GkOeecg8G091no737qvRRdzE+HLALQoMTBbJgBsCj5RSWUlUVJiZ4SOljb05eLFWgoJ5oY6yTyJp62D39jDANoKKcSocPJD5dQYzlFAFZJflUArgTPZKZwLXAnHmerfJquUkKZEgyzqOb5TuDt1P3nwxobqwPocZA11m4A1mBx5IxNgRH21ti7KbAGiyNn3HoF/gJ0w05A8xclpwAAAABJRU5ErkJggg==' />";
	// const char* headhtml = "<meta name='color-scheme' content='dark light'><style></style><script></script>";
	// g_W40_WifiManager.setCustomHeadElement(headhtml);

	// set custom html menu content , inside menu item "custom", see setMenu()
	const char *menuhtml = "<form action='/custom' method='get'><button>Custom</button></form><br/>\n";
	g_W40_WifiManager.setCustomMenuHTML(menuhtml);

	// invert theme, dark
	g_W40_WifiManager.setDarkMode(true);

	// show scan RSSI as percentage, instead of signal stength graphic
	// g_W40_WifiManager.setScanDispPerc(true);

	/*
	  Set cutom menu via menu[] or vector
	  const char* menu[] = {"wifi","wifinoscan","info","param","close","sep","erase","restart","exit"};
	  g_W40_WifiManager.setMenu(menu,9); // custom menu array must provide length
	*/

	std::vector<const char *> menu = {"wifi", "wifinoscan", "info", "param", "custom", "close", "sep", "erase", "update", "restart", "exit"};
	// g_W40_WifiManager.setMenu(menu); // custom menu, pass vector

	// g_W40_WifiManager.setParamsPage(true); // move params to seperate page, not wifi, do not combine with setmenu!

	// set STA static ip
	// g_W40_WifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
	// g_W40_WifiManager.setShowStaticFields(false);
	// g_W40_WifiManager.setShowDnsFields(false);

	// set AP static ip
	// g_W40_WifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
	// g_W40_WifiManager.setAPStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

	// set country
	// setting wifi country seems to improve OSX soft ap connectivity,
	// may help others as well, default is CN which has different channels

	// g_W40_WifiManager.setCountry("US"); // crashing on esp32 2.0

	// set Hostname

	// g_W40_WifiManager.setHostname(("WM_"+g_W40_WifiManager.getDefaultAPName()).c_str());
	// g_W40_WifiManager.setHostname("WM_RANDO_1234");

	// set custom channel
	// g_W40_WifiManager.setWiFiAPChannel(13);

	// set AP hidden
	// g_W40_WifiManager.setAPHidden(true);

	// show password publicly in form
	// g_W40_WifiManager.setShowPassword(true);

	// sets wether g_W40_WifiManager configportal is a blocking loop(legacy) or not, use g_W40_WifiManager.process() in loop if false
	// g_W40_WifiManager.setConfigPortalBlocking(false);

	if (!G_W40_WMISBLOCKING) {
		g_W40_WifiManager.setConfigPortalBlocking(false);
	}

	// sets timeout until configuration portal gets turned off
	// useful to make it all retry or go to sleep in seconds
	g_W40_WifiManager.setConfigPortalTimeout(G_W40_TESP_CP_TIMEOUT);

	// set min quality to show in web list, default 8%
	// g_W40_WifiManager.setMinimumSignalQuality(50);

	// set connection timeout
	// g_W40_WifiManager.setConnectTimeout(20);

	// set wifi connect retries
	// g_W40_WifiManager.setConnectRetries(2);

	// connect after portal save toggle
	// g_W40_WifiManager.setSaveConnect(false); // do not connect, only save

	// show static ip fields
	// g_W40_WifiManager.setShowStaticFields(true);

	// g_W40_WifiManager.startConfigPortal("AutoConnectAP", "password");

	// This is sometimes necessary, it is still unknown when and why this is needed but it may solve some race condition or bug in esp SDK/lib
	// g_W40_WifiManager.setCleanConnect(true); // disconnect before connect, clean connect

	g_W40_WifiManager.setBreakAfterConfig(true);  // needed to use W40_saveWifiCallback

	// set custom webserver port, automatic captive portal does not work with custom ports!
	// g_W40_WifiManager.setHttpPort(8080);

	// fetches ssid and pass and tries to connect
	// if it does not connect it starts an access point with the specified name
	// here  "AutoConnectAP"
	// and goes into a blocking loop awaiting configuration

	// use autoconnect, but prevent configportal from auto starting
	// g_W40_WifiManager.setEnableConfigPortal(false);

	W40_wifiInfo();

	// to preload autoconnect with credentials
	// g_W40_WifiManager.preloadWiFi("ssid","password");

	if (!g_W40_WifiManager.autoConnect("WM_AutoConnectAP", "12345678")) {
		Serial.println("failed to connect and hit timeout");
	} else if (g_W40_TEST_CP) {
		// start configportal always
		delay(1000);
		Serial.println("g_W40_TEST_CP ENABLED");
		g_W40_WifiManager.setConfigPortalTimeout(G_W40_TESP_CP_TIMEOUT);
		g_W40_WifiManager.startConfigPortal("WM_ConnectAP", "12345678");
	} else {
		// if you get here you have connected to the WiFi
		Serial.println("connected...yeey :)");
	}

	W40_wifiInfo();
	pinMode(G_W40_ONDDEMANDPIN, INPUT_PULLUP);

	#ifdef USEOTA
		ArduinoOTA.begin();
	#endif
}





void W40_wifiInfo() {
	// can contain gargbage on esp32 if wifi is not ready yet
	Serial.println("[WIFI] WIFI_INFO DEBUG");
	WiFi.printDiag(Serial);
	Serial.println("[WIFI] MODE: " + (String)(g_W40_WifiManager.getModeString(WiFi.getMode())));
	Serial.println("[WIFI] SAVED: " + (String)(g_W40_WifiManager.getWiFiIsSaved() ? "YES" : "NO"));
	Serial.println("[WIFI] SSID: " + (String)g_W40_WifiManager.getWiFiSSID());
	Serial.println("[WIFI] PASS: " + (String)g_W40_WifiManager.getWiFiPass());
	// Serial.println("[WIFI] HOSTNAME: " + (String)WiFi.getHostname());
}

void W40_run() {
	if (!G_W40_WMISBLOCKING) {
		g_W40_WifiManager.process();
	}

	#ifdef USEOTA
		ArduinoOTA.handle();
	#endif
	// is configuration portal requested?
	if (G_W40_ALLOWONDEMAND && digitalRead(G_W40_ONDDEMANDPIN) == LOW) {
		delay(100);
		if (digitalRead(G_W40_ONDDEMANDPIN) == LOW || G_W40_BUTTONFUNC == 2) {
			Serial.println("BUTTON PRESSED");

			// button reset/reboot
			if (G_W40_BUTTONFUNC == 0) {
				g_W40_WifiManager.resetSettings();
				g_W40_WifiManager.reboot();
				delay(200);
				return;
			}

			// start configportal
			if (G_W40_BUTTONFUNC == 1) {
				if (!g_W40_WifiManager.startConfigPortal("OnDemandAP", "12345678")) {
					Serial.println("failed to connect and hit timeout");
					delay(3000);
				}
				return;
			}

			// test autoconnect as reconnect etc.
			if (G_W40_BUTTONFUNC == 2) {
				g_W40_WifiManager.setConfigPortalTimeout(G_W40_TESP_CP_TIMEOUT);
				g_W40_WifiManager.autoConnect();
				return;
			}

		} else {
			// if you get here you have connected to the WiFi
			Serial.println("connected...yeey :)");
			W40_getTime();
		}
	}

	// every 10 seconds
	if (millis() - g_W40_mtime > 10000) {
		if (WiFi.status() == WL_CONNECTED) {
			W40_getTime();
		} else
			Serial.println("No Wifi");
		g_W40_mtime = millis();
	}
	// put your main code here, to run repeatedly:
	delay(100);
}

void W40_getTime() {
	int		 tz		 = -5;
	int		 dst	 = 0;
	time_t	 now	 = time(nullptr);
	unsigned timeout = 5000;  // try for timeout
	unsigned start	 = millis();
	configTime(tz * 3600, dst * 3600, "pool.ntp.org", "time.nist.gov");
	Serial.print("Waiting for NTP time sync: ");
	while (now < 8 * 3600 * 2) {  // what is this ?
		delay(100);
		Serial.print(".");
		now = time(nullptr);
		if ((millis() - start) > timeout) {
			Serial.println("\n[ERROR] Failed to get NTP time.");
			return;
		}
	}
	Serial.println("");
	struct tm timeinfo;
	gmtime_r(&now, &timeinfo);
	Serial.print("Current time: ");
	Serial.print(asctime(&timeinfo));
}

void W40_debugchipid() {
	// WiFi.mode(WIFI_STA);
	// WiFi.printDiag(Serial);
	// Serial.println(g_W40_wifi_modes[WiFi.getMode()]);

	// ESP.eraseConfig();
	// g_W40_WifiManager.resetSettings();
	// g_W40_WifiManager.erase(true);
	WiFi.mode(WIFI_AP);
	// WiFi.softAP();
	WiFi.enableAP(true);
	delay(500);
	// esp_wifi_start();
	delay(1000);
	WiFi.printDiag(Serial);
	delay(60000);
	ESP.restart();

	// AP esp_267751
	// 507726A4AE30
	// ESP32 Chip ID = 507726A4AE30
}
