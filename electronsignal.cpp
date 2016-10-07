#include "Particle.h"
#include "CellularHelper.h"

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

const unsigned long STARTUP_WAIT_TIME_MS = 8000;
const unsigned long CLOUD_PRE_WAIT_TIME_MS = 5000;
const unsigned long DISCONNECT_WAIT_TIME_MS = 10000;

// Forward declarations
void runModemTests();
void runCellularTests();
void buttonHandler(system_event_t event, int data);

enum State {
	STARTUP_WAIT_STATE,
	CONNECT_STATE,
	CONNECT_WAIT_STATE,
	RUN_CELLULAR_TESTS,
	CLOUD_BUTTON_WAIT_STATE,
	CLOUD_CONNECT_STATE,
	CLOUD_CONNECT_WAIT_STATE,
	CLOUD_PRE_WAIT_STATE,
	RUN_CLOUD_TESTS,
	DISCONNECT_WAIT_STATE,
	REPEAT_TESTS_STATE,
	DONE_STATE,
	IDLE_WAIT_STATE
};
State state = STARTUP_WAIT_STATE;
unsigned long stateTime = 0;
bool buttonClicked = false;


void setup() {
	Serial.begin(9600);
	System.on(button_click, buttonHandler);
}

void loop() {
	switch(state) {
	case STARTUP_WAIT_STATE:
		if (millis() - stateTime >= STARTUP_WAIT_TIME_MS) {
			stateTime = millis();
			state = CONNECT_STATE;
		}
		break;

	case CONNECT_STATE:
		buttonClicked = false;
		Cellular.on();

		Serial.println("attempting to connect to the cellular network...");
		Cellular.connect();

		state = CONNECT_WAIT_STATE;
		stateTime = millis();
		break;

	case CONNECT_WAIT_STATE:
		if (Cellular.ready()) {
			unsigned long elapsed = millis() - stateTime;

			Serial.printlnf("connected to the cellular network in %lu milliseconds", elapsed);
			state = RUN_CELLULAR_TESTS;
			stateTime = millis();
		}
		else
		if (Cellular.listening()) {
			Serial.println("entered listening mode (blinking dark blue) - probably no SIM installed");
			state = DONE_STATE;
		}
		break;

	case RUN_CELLULAR_TESTS:
		Serial.println("running cellular tests");
		runModemTests();
		runCellularTests();

		Serial.println("cellular tests complete");
		Serial.println("press the MODE button to connect to the cloud");
		state = CLOUD_BUTTON_WAIT_STATE;
		break;

	case CLOUD_BUTTON_WAIT_STATE:
		if (buttonClicked) {
			buttonClicked = false;
			state = CLOUD_CONNECT_STATE;
		}
		break;

	case CLOUD_CONNECT_STATE:
		Serial.println("attempting to connect to the Particle cloud...");
		Particle.connect();
		state = CLOUD_CONNECT_WAIT_STATE;
		stateTime = millis();
		break;

	case CLOUD_CONNECT_WAIT_STATE:
		if (Particle.connected()) {
			unsigned long elapsed = millis() - stateTime;

			Serial.printlnf("connected to the cloud in %lu milliseconds", elapsed);


			state = CLOUD_PRE_WAIT_STATE;
			stateTime = millis();
		}
		break;

	case CLOUD_PRE_WAIT_STATE:
		if (millis() - stateTime >= CLOUD_PRE_WAIT_TIME_MS) {
			state = RUN_CLOUD_TESTS;
			stateTime = millis();
		}
		break;

	case RUN_CLOUD_TESTS:
		// Serial.println("running cloud tests");
		state = DONE_STATE;
		break;

	case DISCONNECT_WAIT_STATE:
		if (millis() - stateTime >= DISCONNECT_WAIT_TIME_MS) {
			state = REPEAT_TESTS_STATE;
		}
		break;

	case REPEAT_TESTS_STATE:
		if (Cellular.ready()) {
			state = RUN_CELLULAR_TESTS;
		}
		else {
			state = CONNECT_STATE;
		}
		break;

	case DONE_STATE:
		Serial.println("tests complete!");
		Serial.println("press the MODE button to disconnect from the cloud and repeat tests");
		buttonClicked = false;
		state = IDLE_WAIT_STATE;
		break;


	case IDLE_WAIT_STATE:
		if (buttonClicked) {
			buttonClicked = false;
			if (Particle.connected()) {
				Serial.println("disconnecting from the cloud");
				Particle.disconnect();
				state = DISCONNECT_WAIT_STATE;
				stateTime = millis();
			}
			else {
				state = REPEAT_TESTS_STATE;
			}
		}
		break;
	}
}

// For some reason. these fail when done before cellular connected. I'm not sure why.
void runModemTests() {
	Serial.printlnf("manufacturer=%s", CellularHelper.getManufacturer().c_str());

	Serial.printlnf("model=%s", CellularHelper.getModel().c_str());

	Serial.printlnf("firmware version=%s", CellularHelper.getFirmwareVersion().c_str());

	Serial.printlnf("ordering code=%s", CellularHelper.getOrderingCode().c_str());

	Serial.printlnf("IMEI=%s", CellularHelper.getIMEI().c_str());

	Serial.printlnf("IMSI=%s", CellularHelper.getIMSI().c_str());

	Serial.printlnf("ICCID=%s", CellularHelper.getICCID().c_str());

}

// Various tests to find out information about the cellular network we connected to
void runCellularTests() {

	Serial.printlnf("operator name=%s", CellularHelper.getOperatorName().c_str());

	CellularHelperRSSIQualResponse rssiQual = CellularHelper.getRSSIQual();
	Serial.printlnf("rssi=%d, qual=%d", rssiQual.rssi, rssiQual.qual);

	// First try to get info on neighboring cells. This doesn't work for me using the U260
	CellularHelperEnvironmentResponse envResp = CellularHelper.getEnvironment(5);

	if (envResp.result != RESP_OK) {
		// We couldn't get neighboring cells, so try just the receiving cell
		envResp = CellularHelper.getEnvironment(3);
	}
	envResp.serialDebug();


	Serial.printlnf("ping 8.8.8.8=%d", CellularHelper.ping("8.8.8.8"));

	Serial.printlnf("dns device.spark.io=%s", CellularHelper.dnsLookup("device.spark.io").toString().c_str());

}

void buttonHandler(system_event_t event, int param) {
	// int clicks = system_button_clicks(param);
	buttonClicked = true;
}


