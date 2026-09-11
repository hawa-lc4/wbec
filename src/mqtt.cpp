// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <stdarg.h>
#include <ESP8266WiFi.h>
#include <globalConfig.h>
#include <goEmulator.h>
#include <inverter.h>
#include <logger.h>
#include <loadManager.h>
#include <mbComm.h>
#include <PubSubClient.h>
#include <pvAlgo.h>
#include <rfid.h>

const uint8_t m = 2;
const char*   lastWillTopic  = "wbec/connection";
const char*   lastWillMsgOff = "offline";
const char*   lastWillMsgOn  = "online";
const uint8_t lastWillQos    = 1;
const bool	  lastWillRetain = true;

// prefixes/suffixes used to identify + parse incoming topics
#define PFX_OPENWB_LP      "openWB/lp/"
#define SFX_ACONFIGURED    "/AConfigured"
#define PFX_OPENWB_CP_OLD  "openWB/chargepoint/"          // legacy topic (openWB < 2.1.8)
#define PFX_OPENWB_CP_NEW  "openWB/mqtt/chargepoint/"      // topic since openWB >= 2.1.8 (#178)
#define SFX_SET_CURRENT    "/set/current"
#define PFX_WBEC_LP        "wbec/lp/"
#define SFX_MAXCURRENT     "/maxcurrent"
#define SFX_ENABLE         "/enable"

WiFiClient      espClient;
PubSubClient    client(espClient);

uint32_t 	lastReconnect = 0;
uint8_t   maxcurrent[WB_CNT];
boolean   callbackActive = false;


// search which box index is assigned to the given loadpoint nr.; -1 if none found
// (used to be duplicated inline 4x in callback(), also fixes the out-of-bounds
//  read that happened previously when a loadpoint had no assigned box)
int8_t findBoxForLp(uint8_t lp) {
	for (uint8_t i = 0; i < cfgCntWb; i++) {
		if (cfgMqttLp[i] == lp) {
			return i;
		}
	}
	return -1;
}


// topics for openWB
static void handleOpenWbV1(const char* topic, const char* buffer) {
	const char* pos = strstr_P(topic, PSTR(PFX_OPENWB_LP));
	if (pos && strstr_P(topic, PSTR(SFX_ACONFIGURED))) {
		uint16_t val = atoi(buffer);
		uint8_t  lp  = atoi(pos + strlen(PFX_OPENWB_LP)); 	// loadpoint nr.
		int8_t   i   = findBoxForLp(lp);
		if (i >= 0) {
			// openWB has 1A resolution, wbec has 0.1A resolution
			val = val * 10;
			// set current
			if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
				LOG(0, ", Write to box: %d Value: %d", i, val)
				lm_storeRequest(i, val);
			}
		} else {
			LOG(0, ", no box assigned", "");
		}
	}
}

// topics for openWB 2.0 (#75); "prefix" allows this to be reused for both the
// legacy path and the "mqtt/" path introduced in openWB >= 2.1.8 (#178)
static void handleOpenWbV2(const char* topic, const char* buffer, const char* prefix) {
	const char* pos = strstr_P(topic, prefix);
	if (pos && strstr_P(topic, PSTR(SFX_SET_CURRENT))) {
		float   val = atof(buffer);
		uint8_t lp  = atoi(pos + strlen(prefix)); 	// loadpoint nr.
		int8_t  i   = findBoxForLp(lp);
		if (i >= 0) {
			// openWB resolution is unclear (float with example value 12.34), wbec has 0.1A resolution
			val = val * 10;
			// set current
			if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
				LOG(0, ", Write to box: %d Value: %d", i, (uint8_t)val)
				lm_storeRequest(i, (uint8_t)val);
			}
		} else {
			LOG(0, ", no box assigned", "");
		}
	}
}

// topics for EVCC
static void handleEvccMaxCurrent(const char* topic, const char* buffer) {
	const char* pos = strstr_P(topic, PSTR(PFX_WBEC_LP));
	if (pos && strstr_P(topic, PSTR(SFX_MAXCURRENT))) {
		float   val = atof(buffer);
		uint8_t lp  = atoi(pos + strlen(PFX_WBEC_LP)); 	// loadpoint nr.
		int8_t  i   = findBoxForLp(lp);
		if (i >= 0) {
			// EVCC has 1A resolution, wbec has 0.1A resolution
			val = val * 10;
			// set current
			if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
				LOG(0, ", Write to box: %d Value: %d", i, (uint8_t)val)
				maxcurrent[i] = (uint8_t)val;
				lm_storeRequest(i, (uint8_t)val);
			}
		} else {
			LOG(0, ", no box assigned", "");
		}
	}
}

static void handleEvccEnable(const char* topic, const char* buffer) {
	const char* pos = strstr_P(topic, PSTR(PFX_WBEC_LP));
	if (pos && strstr_P(topic, PSTR(SFX_ENABLE))) {
		uint8_t lp = atoi(pos + strlen(PFX_WBEC_LP)); 	// loadpoint nr.
		int8_t  i  = findBoxForLp(lp);
		if (i >= 0) {
			if (strstr_P(buffer, PSTR("true"))) {
				LOG(0, ", Enable box: %d", i)
				lm_storeRequest(i, maxcurrent[i]);
			} else {
				LOG(0, ", Disable box: %d", i)
				lm_storeRequest(i, 0);
			}
		} else {
			LOG(0, ", no box assigned", "");
		}
	}
}

// set the watt value via MQTT (#54)
static void handleWattTopic(const char* topic, char* buffer) {
	if (strcmp(topic, cfgMqttWattTopic) == 0) {
		if (strcmp(cfgMqttWattJson, "") == 0) {
			// directly take the value from the buffer
			pv_setWatt(atol(buffer));
		} else {
			// extract the value from a JSON string (only 1st occurence)
			// Example: {"Time":"2022-12-10T17:25:46","Main":{"power":-123,"from_grid":441.231,"to_grid":9578.253}}
			// cfgMqttWattJson = power\":                      |      |------>
			// the slash \ will escape the quote " sign
			char *pch = strstr(buffer, cfgMqttWattJson); // search the index of cfgMqttWattJson, then add it's length
			if (pch != NULL) {	// avoid dereferencing NULL if the pattern was not found
				pch += strlen(cfgMqttWattJson);
				pv_setWatt(atol(pch));
			}
		}
	}
}


void callback(char* topic, byte* payload, unsigned int length) {
	callbackActive = true;
	// handle received message
	char buffer[256];	// fixed size instead of a variable-length stack array
	if (length >= sizeof(buffer)) {
		length = sizeof(buffer) - 1;	// truncate oversized payloads to fit the buffer
	}
	for (unsigned int i = 0; i < length; i++) {
		buffer[i] = (char)payload[i];		
	}
	buffer[length] = '\0';			// add string termination
	LOGEXT(m, "Received: %s, Payload: %s", topic, buffer)

	handleOpenWbV1(topic, buffer);
	handleOpenWbV2(topic, buffer, PSTR(PFX_OPENWB_CP_OLD));	// legacy path (openWB < 2.1.8)
	handleOpenWbV2(topic, buffer, PSTR(PFX_OPENWB_CP_NEW));	// path since openWB >= 2.1.8 (#178)
	handleEvccMaxCurrent(topic, buffer);
	handleEvccEnable(topic, buffer);
	handleWattTopic(topic, buffer);

	callbackActive = false;
}


void mqtt_begin() {
	if (strcmp(cfgMqttIp, "") != 0) {
  	client.setServer(cfgMqttIp, cfgMqttPort);
		client.setCallback(callback);
	}
	for (uint8_t i = 0; i < cfgCntWb; i++) {
		maxcurrent[i] = CURR_ABS_MIN;
	}
}

void reconnect() {
	LOGN(m, "Attempting MQTT connection...", "");
	// Create a random client ID
	char clientId[10];
	if (cfgMqttClientId) {
		snprintf_P(clientId, sizeof(clientId), PSTR("wbec-%d"), cfgMqttClientId);
	} else {
		snprintf_P(clientId, sizeof(clientId), PSTR("wbec-%d"), (uint8_t)random(255));
	}
	

	// Attempt to connect
	boolean con = false;
	if (strcmp(cfgMqttUser, "") != 0 && strcmp(cfgMqttPass, "") != 0) {
		con = client.connect(clientId, cfgMqttUser, cfgMqttPass, lastWillTopic, lastWillQos, lastWillRetain, lastWillMsgOff);
	} else {
		con = client.connect(clientId, lastWillTopic, lastWillQos, lastWillRetain, lastWillMsgOff);
	}
	if (con)
	{
		LOG(0, "connected", "");
		//once connected to MQTT broker, subscribe command if any
		for (uint8_t i = 0; i < cfgCntWb; i++) {
			char topic[48];
			if (cfgMqttLp[i] != 0) {
				snprintf_P(topic, sizeof(topic), PSTR("openWB/lp/%d/AConfigured"), cfgMqttLp[i]);
				client.subscribe(topic);
				snprintf_P(topic, sizeof(topic), PSTR("openWB/chargepoint/%d/set/current"), cfgMqttLp[i]);	// was missing entirely before
				client.subscribe(topic);
				snprintf_P(topic, sizeof(topic), PSTR("openWB/mqtt/chargepoint/%d/set/current"), cfgMqttLp[i]);	// #178
				client.subscribe(topic);
				snprintf_P(topic, sizeof(topic), PSTR("wbec/lp/%d/enable"), cfgMqttLp[i]);
				client.subscribe(topic);
				snprintf_P(topic, sizeof(topic), PSTR("wbec/lp/%d/maxcurrent"), cfgMqttLp[i]);
				client.subscribe(topic);
			}
		}
		client.subscribe(cfgMqttWattTopic);
	} else {
		LOG(m, "failed, rc=%d try again in 5 seconds", client.state())
	}
}

void mqtt_handle() {
	if (strcmp(cfgMqttIp, "") != 0) {
		uint32_t now = millis();

		if (!client.connected()) {
			if (now - lastReconnect > 5000 || lastReconnect == 0) {
				reconnect();
				lastReconnect = now;
			}
		}

		client.loop();
	}
}


// publish a single value under "<header>/<suffix>" (retain=true), used by mqtt_publish()
// to avoid repeating the same snprintf_P + client.publish pattern ~35 times
static void mqttPublish(const char* header, const char* suffix, const char* fmt, ...) {
	char topic[60];
	char value[20];
	va_list args;

	snprintf_P(topic, sizeof(topic), PSTR("%s/%s"), header, suffix);

	va_start(args, fmt);
	vsnprintf_P(value, sizeof(value), fmt, args);
	va_end(args);

	client.publish(topic, value, true);
}


// publishes the "openWB 2.0" (#75) parameter set under the given header; called once
// for the legacy header and once for the "mqtt/" header used since openWB >= 2.1.8 (#178)
static void publishOpenWbV2(const char* header, uint8_t ps, uint8_t cs, uint8_t i) {
	mqttPublish(header, "get/plug_state",    PSTR("%s"), ps?"true":"false");
	mqttPublish(header, "get/charge_state",  PSTR("%s"), cs?"true":"false");
	mqttPublish(header, "get/power",         PSTR("%d"), content[i][10]);
	mqttPublish(header, "get/imported",      PSTR("%ld"), ((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]));
	mqttPublish(header, "get/exported",      PSTR("%d"), 0);	// WBEC cannot measure fed-back energy (#178)
	mqttPublish(header, "get/voltages",      PSTR("[%d,%d,%d]"), content[i][6], content[i][7], content[i][8]);	// L1 = 6, L2 = 7, L3 = 8
	mqttPublish(header, "get/currents",      PSTR("[%.1f,%.1f,%.1f]"), (float)content[i][2]/10.0, (float)content[i][3]/10.0, (float)content[i][4]/10.0);	// L1 = 2, L2 = 3, L3 = 4
	mqttPublish(header, "get/phases_in_use", PSTR("%d"), cfgPvPhFactor / 23);
	mqttPublish(header, "get/rfid_tag",      PSTR("%s"), rfid_getLastID());	// legacy field name, kept for compatibility
	mqttPublish(header, "get/rfid",          PSTR("%s"), rfid_getLastID());	// field name used since openWB >= 2.1.8 (#178)
}


void mqtt_publish(uint8_t i) {
	if (strcmp(cfgMqttIp, "") == 0 || cfgMqttLp[i] == 0) {
		return;	// do nothing, when Mqtt is not configured, or box has no loadpoint assigned
	}
	
	uint8_t ps = 0;
	uint8_t cs = 0;
	char status;

	switch(content[i][1]) {
		case 0:  ps = 0; cs = 0; status = 'A'; break; // e.g. wallbox offline (#120)
		case 2:  ps = 0; cs = 0; status = 'A'; break;
		case 3:  ps = 0; cs = 0; status = 'A'; break;
		case 4:  ps = 1; cs = 0; status = 'B'; break;
		case 5:  ps = 1; cs = 0; status = 'B'; break;
		case 6:  ps = 1; cs = 0; status = 'C'; break;
		case 7:  ps = 1; cs = 1; status = 'C'; break;
		default: ps = 0; cs = 0; status = 'F'; break; 
	}

	char header[30];
	
	// topics for openWB
	snprintf_P(header, sizeof(header), PSTR("openWB/set/lp/%d"), cfgMqttLp[i]);

	mqttPublish(header, "plugStat",   PSTR("%d"), ps);
	mqttPublish(header, "chargeStat", PSTR("%d"), cs);
	mqttPublish(header, "W",          PSTR("%d"), content[i][10]);
	mqttPublish(header, "kWhCounter", PSTR("%.3f"), (float)((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) / 1000.0);

	for (uint8_t ph = 1; ph <= 3; ph++) {
		char suffix[10];
		snprintf_P(suffix, sizeof(suffix), PSTR("VPhase%d"), ph);
		mqttPublish(header, suffix, PSTR("%d"), content[i][ph+5]);	// L1 = 6, L2 = 7, L3 = 8
	}

	for (uint8_t ph = 1; ph <= 3; ph++) {
		char suffix[10];
		snprintf_P(suffix, sizeof(suffix), PSTR("APhase%d"), ph);
		mqttPublish(header, suffix, PSTR("%.1f"), (float)content[i][ph+1]/10.0);	// L1 = 2, L2 = 3, L3 = 4
	}

	LOG(m, "Publish to %s", header)

	// topics for openWB 2.0 (#75), published under both the legacy path and the
	// "mqtt/" path used since openWB >= 2.1.8 (#178), to stay compatible with both
	char headerCpLegacy[30];
	char headerCpMqtt[40];
	snprintf_P(headerCpLegacy, sizeof(headerCpLegacy), PSTR("openWB/set/chargepoint/%d"), cfgMqttLp[i]);
	snprintf_P(headerCpMqtt,   sizeof(headerCpMqtt),   PSTR("openWB/set/mqtt/chargepoint/%d"), cfgMqttLp[i]);
	publishOpenWbV2(headerCpLegacy, ps, cs, i);
	publishOpenWbV2(headerCpMqtt,   ps, cs, i);

	// topics for EVCC
	snprintf_P(header, sizeof(header), PSTR("wbec/lp/%d"), cfgMqttLp[i]);

	mqttPublish(header, "status", PSTR("%c"), status);

	boolean enabled = (content[i][53] > 0);
	if (enabled) {
		maxcurrent[i] = content[i][53];       // memorize the current limit if not 0
	}
	mqttPublish(header, "enabled", PSTR("%s"), enabled ? "true" : "false");

	mqttPublish(header, "power",   PSTR("%d"), content[i][10]);
	mqttPublish(header, "energy",  PSTR("%.3f"), (float)((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) / 1000.0);
	mqttPublish(header, "energyC", PSTR("%.3f"), (float)goE_getEnergySincePlugged(i) / 1000.0);

	for (uint8_t ph = 1; ph <= 3; ph++) {
		char suffix[10];
		snprintf_P(suffix, sizeof(suffix), PSTR("currL%d"), ph);
		mqttPublish(header, suffix, PSTR("%.1f"), (float)content[i][ph+1]/10.0);	// L1 = 2, L2 = 3, L3 = 4
	}

	for (uint8_t ph = 1; ph <= 3; ph++) {
		char suffix[10];
		snprintf_P(suffix, sizeof(suffix), PSTR("voltL%d"), ph);
		mqttPublish(header, suffix, PSTR("%d"), content[i][ph+5]);	// L1 = 6, L2 = 7, L3 = 8
	}

	mqttPublish(header, "currLimit", PSTR("%.1f"), (float)content[i][53]/10.0);
	mqttPublish(header, "pcbTemp",   PSTR("%.1f"), (float)content[i][5]/10.0);
	mqttPublish(header, "resCode",   PSTR("%x"), modbusResultCode[i]);

	int qrssi = WiFi.RSSI();
	mqttPublish(header, "wifiRssi",    PSTR("%d"), qrssi);
	mqttPublish(header, "wifiChannel", PSTR("%d"), WiFi.channel());

	mqttPublish(header, "plugState",   PSTR("%s"), ps?"true":"false");
	mqttPublish(header, "chargeState", PSTR("%s"), cs?"true":"false");

	// publish values from inverter
	if (strcmp(cfgInverterIp, "") != 0) {
		snprintf_P(header, sizeof(header), PSTR("wbec/inverter"));
		mqttPublish(header, "pwrInv", PSTR("%ld"), inverter_getPwrInv());	// was "%d" (mismatched vararg type)
		mqttPublish(header, "pwrMet", PSTR("%ld"), inverter_getPwrMet());	// was "%d" (mismatched vararg type)
	}

	// publish values from pvAlgo
	if (pv_getMode()) {
		snprintf_P(header, sizeof(header), PSTR("wbec/pv"));
		mqttPublish(header, "mode", PSTR("%d"), pv_getMode());
		mqttPublish(header, "watt", PSTR("%ld"), pv_getWatt());
	}

	// Wbec-Connection Status
	client.publish(lastWillTopic, lastWillMsgOn, lastWillRetain);
}

void mqtt_log(const char *output, const char *msg) {
	if (strcmp(cfgMqttIp, "") == 0 || callbackActive) {
		return;	// do nothing, when Mqtt is not configured OR when request comes from mqtt callback (#13)
	}

	boolean retain = true;
	char topic[10];
	char value[150];

	snprintf_P(topic, sizeof(topic), PSTR("wbec/log"), "");
	snprintf_P(value, sizeof(value), PSTR("%s%s"), output, msg);
	client.publish(topic, value, retain);
}
