/* NRF24L01 client
 * Pings server and waits for an ack packet
 * Uses a speaker for audio feedback on send/receive
 */

#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include <Bounce.h>

const int rfChannel = 35;             // 0 to 127 (0 to 84 in USA)
byte myAddr[]   = {"pingC"};          // address of this device
byte destAddr[] = {"pingS"};          // address of remote device
const int payloadSize = sizeof(byte); // 1 byte payload size
const int serverTimeout = 1000;       // 1000 ms server ping timeout

// these must be in byte array form, even if they're just one byte
byte pingPacket[payloadSize]    = {12};
byte confirmPacket[payloadSize] = {84};

byte recvData[payloadSize] = {0}; // incoming data buffer

const int pinSpkr    = 2; // speaker to beep on success or failure
const int pinLedSucc = 3; // green led to blink on success
const int pinLedFail = 4; // red led to blink on failure
const int pinSwPing  = 5; // ground this pin to send a ping

const float pingFreq = 246.942; // B3
const float succFreq = 329.628; // E4
const float failFreq = 164.814; // E3

const int beepLengthMs = 85;

Bounce swPing = Bounce(pinSwPing, 10); // 10 ms debounce time

void setup() {
	Serial.begin(115200);

	pinMode(pinSpkr, OUTPUT);
	pinMode(pinLedSucc, OUTPUT);
	pinMode(pinLedFail, OUTPUT);
	
	pinMode(pinSwPing, INPUT);
	digitalWrite(pinSwPing, HIGH); // pullup resistor

	Mirf.spi = &MirfHardwareSpi;
	Mirf.init();
	Mirf.setRADDR(myAddr);
	Mirf.setTADDR(destAddr);
	Mirf.payload = payloadSize;
	Mirf.channel = rfChannel;
	Mirf.config();
	Serial.println("Client is ready to go!");

	Serial.print("My address:     ");
	for (int i = 0; i < 5; i++) {
		Serial.print(myAddr[i], HEX);
		Serial.print(' ');
	}
	Serial.println();
	Serial.print("Target address: ");
	for (int i = 0; i < 5; i++) {
		Serial.print(destAddr[i], HEX);
		Serial.print(' ');
	}
	Serial.println();
	uint8_t tempRfCh[1] = {0};
	Mirf.readRegister(RF_CH, tempRfCh, 1);
	Serial.print("RF channel:     ");
	Serial.println(tempRfCh[0]);
}

void loop() {
	swPing.update();
	if (swPing.fallingEdge()) {
		ping();
	}
}

void ping() {
	// send a ping
	beep(pingFreq, beepLengthMs);

	bool pingAcked = false;
	Mirf.send(pingPacket);
	while (Mirf.isSending()) { /* nop */ }
	Serial.println("Ping sent.");

	// wait for ack
	unsigned long timeoutTime = millis() + serverTimeout;
	while (pingAcked == false && millis() < timeoutTime) {
		if (Mirf.dataReady()) {
			pingAcked = true; // breaks while loop
		}
	}
	
	// respond for user
	if (pingAcked) {
		Serial.println("Ack received!");
		Mirf.getData(recvData);
		showResult(true); // success beep
	} else {
		Serial.println("Server timeout.");
		showResult(false); // failure beep
	}
}

void showResult (bool result) {
	delay(beepLengthMs / 3);
	if (result == true) { // success
		digitalWrite(pinLedSucc, HIGH);
		beep(succFreq, beepLengthMs);
		digitalWrite(pinLedSucc, LOW);
	} else { // failure
		digitalWrite(pinLedFail, HIGH);
		for (int i = 0; i <= 2; i++) {
			beep(failFreq, beepLengthMs);
			delay(beepLengthMs / 3);
		}
		digitalWrite(pinLedFail, LOW);
	}
}

void beep (long freq, long lengthMs) {
	long delayHalfUs = 250000 / freq;
	long timeBeepEnd = millis() + lengthMs;
	while (millis() < timeBeepEnd) {
		digitalWrite(pinSpkr, HIGH);
		delayMicroseconds(delayHalfUs);
		digitalWrite(pinSpkr, LOW);
		delayMicroseconds(delayHalfUs);
	}
}