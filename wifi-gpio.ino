/*
 * Copyright (c) 2020, Luc Yriarte
 * License: BSD <http://www.opensource.org/licenses/bsd-license.php>
 */

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>


/* **** **** **** **** **** ****
 * Constants
 * **** **** **** **** **** ****/

#define serverPORT 80
#define WIFI_CLIENT_DELAY_MS 500
#define WIFI_CLIENT_CONNECTED_DELAY_MS 100
#define WIFI_SERVER_DELAY_MS 200
#define WIFI_CONNECT_DELAY_MS 3000
#define WIFI_CONNECT_RETRY_DELAY_MS 1000
#define WIFI_CONNECT_RETRY 5

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

typedef struct {
  char * SSID;
  char * passwd;
  IPAddress address;
  IPAddress gateway;
  IPAddress netmask;
} wifiNetInfo;

wifiNetInfo networks[] = {
  {
    "network1",
    "password",
    IPAddress(192,168,0,2),
    IPAddress(192,168,0,1),
    IPAddress(255,255,255,0)
  },
  {
    "network2",
    "password",
    IPAddress(0,0,0,0),
    IPAddress(0,0,0,0),
    IPAddress(0,0,0,0)
  }
};


int wifiStatus = WL_IDLE_STATUS;
char hostnameSSID[] = "ESP_XXXXXX";
char wifiMacStr[] = "00:00:00:00:00:00";
byte wifiMacBuf[6];

/*
 * gpio
 */

int pinState[] = {0,0,0,0};


/* **** **** **** **** **** ****
 * Functions
 * **** **** **** **** **** ****/

/*
 * WiFi
 */

void wifiMacInit() {
	WiFi.macAddress(wifiMacBuf);
	int i,j,k;
	j=0; k=4;
	for (i=0; i<6; i++) {
		wifiMacStr[j] = '0' + (wifiMacBuf[i] >> 4);
		if (wifiMacStr[j] > '9')
			wifiMacStr[j] += 7;
		if (i>2)
			hostnameSSID[k++] = wifiMacStr[j];
		++j;
		wifiMacStr[j] = '0' + (wifiMacBuf[i] & 0x0f);
		if (wifiMacStr[j] > '9')
			wifiMacStr[j] += 7;
		if (i>2)
			hostnameSSID[k++] = wifiMacStr[j];
		j+=2;
	}
}

bool wifiConnect(int retry) {
	int i,n;
	wifiStatus = WiFi.status();
	if (wifiStatus == WL_CONNECTED)
		return true;
	n = sizeof(networks) / sizeof(wifiNetInfo);
	for (i=0; i<n; i++) {
		if (wifiNetConnect(&networks[i], retry))
			return true;
	}
	WiFi.disconnect();
	wifiStatus = WiFi.status();
	return false;
}

bool wifiNetConnect(wifiNetInfo *net, int retry) {
	WiFi.config(net->address, net->gateway, net->netmask);  
	wifiStatus = WiFi.begin(net->SSID, net->passwd);
	while (wifiStatus != WL_CONNECTED && retry > 0) {
		retry--;
		delay(WIFI_CONNECT_DELAY_MS);
		wifiStatus = WiFi.status();
	}
	if (wifiStatus == WL_CONNECTED) {
		MDNS.begin(hostnameSSID);
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
}

void loop() {
	int i, j, nread, readstate;
	while (!wifiConnect(WIFI_CONNECT_RETRY))
		delay(WIFI_CONNECT_RETRY_DELAY_MS);
	wifiServer.begin();
	delay(WIFI_SERVER_DELAY_MS);
	while (wifiStatus == WL_CONNECTED) {
		wifiClient = wifiServer.available();
		if (wifiClient && wifiClient.connected()) {
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
			delay(WIFI_CLIENT_DELAY_MS);
			wifiClient.stop();
		}
	}
}
