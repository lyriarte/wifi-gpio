/*
 * Copyright (c) 2017, Luc Yriarte
 * License: BSD <http://www.opensource.org/licenses/bsd-license.php>
 */


#include <ESP8266WiFi.h>


/* **** **** **** **** **** ****
 * Constants
 * **** **** **** **** **** ****/

#define serverPORT 80
#define AP_SUBNET 64
#define WIFI_CLIENT_DELAY 500
#define WIFI_CONNECT_DELAY 2000
#define WIFI_CONNECT_RETRY 10

enum {
	METHOD,
	URI,
	IGNORE
};


/* **** **** **** **** **** ****
 * Global variables
 * **** **** **** **** **** ****/

/*
 * WiFi
 */

WiFiServer wifiServer(serverPORT);
WiFiClient wifiClient;

char * wifiSSIDs[] = {"SSID1", "SSID2", "SSID3"};
char * wifiPASSWDs[] = {"pwd1", "pwd2", "pwd3"};
int wifiStatus = WL_IDLE_STATUS;
bool wifiAPmode = false;
char apSSID[] = "ESP_XXXXXX";
char wifiMacStr[] = "00:00:00:00:00:00";
byte wifiMacBuf[6];

/*
 * gpio
 */

int pinState[] = {0,0,0,0};

/* **** **** **** **** **** ****
 * Functions
 * **** **** **** **** **** ****/

void wifiMacInit() {
	WiFi.macAddress(wifiMacBuf);
	int i,j,k;
	j=0; k=4;
	for (i=0; i<6; i++) {
		wifiMacStr[j] = '0' + (wifiMacBuf[i] >> 4);
		if (wifiMacStr[j] > '9')
			wifiMacStr[j] += 7;
		if (i>2)
			apSSID[k++] = wifiMacStr[j];
		++j;
		wifiMacStr[j] = '0' + (wifiMacBuf[i] & 0x0f);
		if (wifiMacStr[j] > '9')
			wifiMacStr[j] += 7;
		if (i>2)
			apSSID[k++] = wifiMacStr[j];
		j+=2;
	}
}

void wifiAPInit() {
	IPAddress ip(192, 168, AP_SUBNET, 1);
	IPAddress mask(255,255,255,0);
	WiFi.mode(WIFI_AP_STA);
	WiFi.softAPConfig(ip,ip,mask);
	WiFi.softAP(apSSID);
	delay(WIFI_CONNECT_DELAY);
	wifiAPmode = true;
}

bool wifiConnect(int retry) {
	int i,n;
	if (wifiAPmode || wifiStatus == WL_CONNECTED)
		return true;
	n = sizeof(wifiSSIDs) / sizeof(char *);
	for (i=0; i<n; i++) {
		if (wifiConnectSSID(wifiSSIDs[i], wifiPASSWDs[i], retry))
			return true;
	}
	return false;
}

bool wifiConnectSSID(char * wifiSSID, char * wifiPASSWD, int retry) {
	wifiStatus = WiFi.status();
	while (wifiStatus != WL_CONNECTED && retry != 0) {
		if (wifiStatus != WL_IDLE_STATUS) {
			wifiStatus = WiFi.begin(wifiSSID, wifiPASSWD);
			if (retry > 0)
				retry--;
		}
		delay(WIFI_CONNECT_DELAY);
	}
	return wifiStatus == WL_CONNECTED;
}


/*
 * gpio
 */

void pinUpdate() {
	int i;
	for (i=0; i<4; i++)
		digitalWrite(i, pinState[i]);
}

/*
 * setup / main loop
 */


void setup() {
	int i;
	for (i=0; i<4; i++)
		pinMode(i, OUTPUT);
	pinUpdate();
	wifiMacInit();
	if (!wifiConnect(WIFI_CONNECT_RETRY))
		wifiAPInit();
	wifiServer.begin();
}

void loop() {
	int i, j, nread, readstate;
	if (!wifiConnect(WIFI_CONNECT_RETRY))
		wifiAPInit();
	wifiClient = wifiServer.available();
	if (wifiClient && wifiClient.connected()) {
		delay(100);
		i = j = 0;
		readstate = METHOD;
		while (wifiClient.available()) {
			char c = wifiClient.read();
			switch (readstate) {
				case METHOD:
					if (c == '/')
						readstate = URI;
					else if (c == '\n')
						readstate = IGNORE;
					break;
				case URI:
					if (i > 3 || c < '0' || c > '1') {
						readstate = IGNORE;
						break;
					}
					pinState[i++] = c - '0';
					break;
				default:
					break;
			}
		}
		pinUpdate();
		wifiClient.println("HTTP/1.1 200 OK");
		wifiClient.println("Content-Type: text/plain");
		wifiClient.println("Access-Control-Allow-Origin: *");
		wifiClient.println("Connection: close");
		wifiClient.println();
		for (i=0; i<4; i++)
			wifiClient.print(pinState[i]);
		wifiClient.println();
		delay(WIFI_CLIENT_DELAY);
		wifiClient.stop();
	}
}
