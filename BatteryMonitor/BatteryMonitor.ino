/*
 Name:		BatteryMonitor.ino
 Author:	STS89 (John Robertson)
 Dates:      Started ~2/10/2021;  Working Prototype: ~2/20/2021

 Function:  Device to monitor up to two 12 volt batteries and alert via email when voltage drops below predefined thresholds.
			Based upon an ESP32 Dev Module
*/

/*
  ESP32 Dev Module microboard used to build 12 volt battery monitor

  Hardware:
   - ESP32 DevKit1 Module - https://www.amazon.com/gp/product/B08FR5Y85W/
   - DC/DC Buck Converter - https://www.amazon.com/gp/product/B08GSGYRGW/
		Notes:  Learned about the concept of dropout voltage for the commonly used LM1117 voltage regulator used internal to the ESP32 board.
		Had trouble getting wifi to work when powered by the battery (through my DC/DC converter) instead of the USB port used during development.
		Turns out the Vin voltage (e.g., from the DC/DC converter) must be at least 1.2v higher than the 3.3 volts output by the internal LM1117
		to power the ESP32 board.  This concept is called "dropout voltage" in the voltage regulator datasheets.  Once my DC/DC convert was set to
		output greater than 4.5v the circuit operated properbly when powered from the 12v battery.

  WiFi / eMail Tutorial - details at
   - https://RandomNerdTutorials.com/esp32-send-email-smtp-server-arduino-ide/
   - https://raw.githubusercontent.com/RuiSantosdotme/Random-Nerd-Tutorials/master/Projects/ESP32/ESP32_Email/ESP32_Email_Simple.ino
   - NOTE:  did not actually use ESP32 mail library in the example since it was deprecated.   https://github.com/mobizt/ESP32-Mail-Client
   - Choose a newer more general purpose (and more complicated!) version:  https://github.com/mobizt/ESP-Mail-Client

  DeepSleep/Wake-up Tutorial
   - details at https://raw.githubusercontent.com/RuiSantosdotme/ESP32-Course/master/code/DeepSleep/TimerWakeUp/TimerWakeUp.ino
*/

/*
 * CONFIGURATION and AUTHENTICATION
 *
 * Make all changes within those files, not the program source below.
 *
 */
#include "config.h"			// Configuration Parameters
#include "credentials.h"	// WiFi and SMTP Email Login Information


 /* PROGRAM CODE FOLLOWS */
#include <Arduino.h>
#include <ESP_Mail_Client.h>
#include <WiFi.h>

// CONSTANTS
#define uS_TO_S_FACTOR 1000000  // Conversion factor for micro seconds to seconds
#define LED 2                   // GPIO2 controls onboard blue LED on the ESP32 board

// GLOBAL VARIABLES
float batteryA_Volts = -1.0; // engine battery
float batteryB_Volts = -1.0; // house (deep cycle) batteries
boolean batteryA_Alarm = false;
boolean batteryB_Alarm = false;
boolean batteryA_Warn = false;
boolean batteryB_Warn = false;
boolean alertNeeded = false;
char alertSubjectA[BUF_SIZE / 2];  // preallocated buffers storage for building subject and message strings
char alertSubjectB[BUF_SIZE / 2];
char alertSubject[BUF_SIZE];
char alertMsgA[BUF_SIZE / 2];
char alertMsgB[BUF_SIZE / 2];
char alertMsg[BUF_SIZE];
uint64_t sleepMicroSec;
ESP_Mail_Session session;
SMTPSession smtp;
SMTP_Message message;
RTC_DATA_ATTR int wakeupCount = 0; // RTC_DATA_ATTR label defines r/w variable stored in deep sleep, low power memory

/*
 * C++ FUNCTION PROTOTYPES DEFINATIONS
 *
 * These are actually not required for Arduino because they get automatically created by the Arduino complier;
 * however, its good practice and elminates warning in VisualStudio IDE
 *
 */
void displayRunningSketch();	        // Displays information (via serial port) about complilation date of sketch
void getWakeUpReason();			        // Returns reason that ESP32 awoke from deep sleep
void sendAlertMail();			        // Sends mail alerts
void smtpCallback(SMTP_Status status);	// SMTP Email callback function declaration to get send status 
boolean composeAlerts();	            // Determines if an alert should be sent based upon voltage measurements
boolean startWiFi();		            // Initializes WiFi connection - returns true if successful
float measureVolts(int v, float f);  	// Reads average voltage on the identified pin and scales to the target max voltage


/* Initialization done on power-up and waking from deep sleep */
void setup() {
	// Start-up housekeeping tasks
	Serial.begin(115200);   // Start port for serial output
	displayRunningSketch(); // Send info (via serial port) about the program version loaded in the flash memory
	++wakeupCount;          // Increment boot number and print it every wake-up

	if (DEBUG) {
		Serial.printf("Wakeup Count: %i \n", wakeupCount);  // Note "printf" function is unique to ESP32 hardware implementation
		getWakeUpReason();
	}

	// show BLUE LED long/short flash to indicate device is awake and starting measurements
	pinMode(LED, OUTPUT);
	digitalWrite(LED, HIGH);
	delay(200);
	digitalWrite(LED, LOW);
	delay(50);
	digitalWrite(LED, HIGH);
	delay(50);
	digitalWrite(LED, LOW);

	// Measure battery volts
	batteryA_Volts = measureVolts(ADC_PIN_A, ADC_A_MULTIPLIER);
	Serial.printf("Battery A Volts: %f\n", batteryA_Volts);
	if (BATTERY_B_MONITORED) {
		batteryB_Volts = measureVolts(ADC_PIN_B, ADC_B_MULTIPLIER);
		Serial.printf("Battery B Volts: %f\n", batteryB_Volts);
	}

	// Evaluate need to alert and (when appropriate) form the email subject and msg body text. Returns true when alert needed.
	alertNeeded = composeAlerts();
	Serial.printf("After composeAlerts -- alertNeeded: %d  Subject: %s \n", alertNeeded, alertSubject);

	// Send email(s) if alert appropriate
	// Note: SEND_MAIL is flag used to prevent excessive emails during development testing
	if (SEND_MAIL && alertNeeded && startWiFi()) {
		Serial.println("WiFi connected and preparing to send alert.");
		sendAlertMail();
	}
	else {
		Serial.println("No mail sent.");
	}

	// Gracefully shutdown wifi and serial port before and going into deep sleep   
	WiFi.disconnect();
	digitalWrite(LED, LOW);
	Serial.printf("Going to deep sleep for %i seconds\n", TIME_TO_SLEEP);
	Serial.flush();
	sleepMicroSec = TIME_TO_SLEEP * uS_TO_S_FACTOR;
	Serial.printf("Going to sleep for %lu microsecs\n", sleepMicroSec);
	esp_deep_sleep(sleepMicroSec);

	// Should be asleep - the following should never be executed
	Serial.println("Hey, what happened?  You're supposed to have gone to sleep!");
}

void loop() {
	/* loop never entered - should be nothing happening here */
	Serial.println("Hey, what happened?  You're never supposed to have started looping!");
}


/*
  Attempt NUM_WIFI_ATTEMPTS to establish the WiFi connnection.
  Returns true if connected.
*/
bool startWiFi() {
	int retry = NUM_WIFI_ATTEMPTS;

	if (DEBUG) Serial.print("Attempting to connect to wifi ");
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED && retry > 0) {
		if (DEBUG) Serial.print(". ");
		digitalWrite(LED, HIGH);	// Flash onboard LED to show WiFi trying to connect
		delay(WIFI_RETRY_INTERVAL / 2);
		digitalWrite(LED, LOW);
		delay(WIFI_RETRY_INTERVAL / 2);
		retry--;
	}
	if (DEBUG) Serial.printf("Wifi status: %d\n", WiFi.status());

	// Return WiFi Connected Status and set LED indicator 
	if (WiFi.status() == WL_CONNECTED) {
		digitalWrite(LED, HIGH);
		return true;
	}
	else {
		//digitalWrite(LED, LOW);
		digitalWrite(LED, HIGH);
		return false;
	}
}


/*
 * Measure average voltage on the designated pin
 */
float measureVolts(int adcPin, float adcCorrectionFactor) {
	int rawValue = 0;
	float rawValueAvg = 0;
	float vAvg = 0;

	pinMode(adcPin, INPUT);  // sets pin to input with no pullup/pulldown resistor
	analogSetPinAttenuation(ADC_PIN_A, ADC_11db);  // sets the target range of the ADC
	analogSetPinAttenuation(ADC_PIN_B, ADC_11db);

	// averaging numerous readings is probably unncecessary but won't hurt.
	for (int n = 0; n < NUM_READINGS; n++) {
		rawValue = analogRead(adcPin); // individual ADC reading
		if (DEBUG) Serial.printf("Raw ADC Value: %i\n", rawValue);
		rawValueAvg += rawValue; // sum of n samples
		delay(DELAY_BETWEEN_READINGS);
	}
	// Convert to battery volts using adc resolution (12 bits) and external resistor network voltage divider factor
	if (DEBUG) Serial.printf("rawValueAvg: %f\n", rawValueAvg / NUM_READINGS);
	vAvg = (rawValueAvg / NUM_READINGS / adcCorrectionFactor);
	if (DEBUG) Serial.printf("vAvg: %f\n", vAvg);
	return (vAvg);
}


/*
 * Apply rules to decide if an alert should be sent
 */
boolean composeAlerts() {
	alertNeeded = false;
	batteryA_Alarm = false;
	batteryA_Warn = false;

	// strategy:   1) send warnings & alerts every time below threshold.  2) send an "I'm working" status email every "IM_OK_INTERVAL" wakeups (if no other alarm).
	// battery A measurements
	if (batteryA_Volts <= BATTERY_A_ALARM_LOW) {
		batteryA_Alarm = true;
		alertNeeded = true;
		snprintf(alertSubjectA, sizeof(alertSubjectA), "%s", SUBJECT_BATTERY_A_ALARM_LOW);
		snprintf(alertMsgA, sizeof(alertMsgA), "Battery A at %f volts (%d).  Alarm level is %f volts]", batteryA_Volts, batteryA_Volts * ADC_A_MULTIPLIER, BATTERY_A_ALARM_LOW);
	}
	else if (batteryA_Volts <= BATTERY_A_WARN_LOW) {
		batteryA_Warn = true;
		alertNeeded = true;
		snprintf(alertSubjectA, sizeof(alertSubjectA), "%s", SUBJECT_BATTERY_A_WARN_LOW);
		snprintf(alertMsgA, sizeof(alertMsgA), "Battery A at %f volts (%d).  Warning level is %f volts]", batteryA_Volts, batteryA_Volts * ADC_A_MULTIPLIER, BATTERY_A_WARN_LOW);
	}

	// battery B measurements
	if (BATTERY_B_MONITORED) {
		batteryB_Alarm = false;
		batteryB_Warn = false;

		if (batteryB_Volts <= BATTERY_B_ALARM_LOW) {
			batteryB_Alarm = true;
			alertNeeded = true;
			snprintf(alertSubjectB, sizeof(alertSubjectB), "%s", SUBJECT_BATTERY_B_ALARM_LOW);
			snprintf(alertMsgB, sizeof(alertMsgB), "Battery B at %f volts (%d).  Alarm level is %f volts]", batteryB_Volts, batteryB_Volts * ADC_B_MULTIPLIER, BATTERY_A_ALARM_LOW);

		}
		else if (batteryB_Volts <= BATTERY_A_WARN_LOW) {
			batteryB_Warn = true;
			alertNeeded = true;
			snprintf(alertSubjectB, sizeof(alertSubjectB), "%s", SUBJECT_BATTERY_B_WARN_LOW);
			snprintf(alertMsgB, sizeof(alertMsgB), "Battery B at %f volts (%d).  Warning level is %f volts]", batteryB_Volts, batteryB_Volts * ADC_B_MULTIPLIER, BATTERY_B_WARN_LOW);
		}
	}

	// combine info into designated buffers to send required alerts for both batteries in one email
	if (alertNeeded) {
		if (BATTERY_B_MONITORED) {
			snprintf(alertSubject, sizeof(alertSubject), "%s / %s", alertSubjectA, alertSubjectB);  // forms subject string for email
			snprintf(alertMsg, sizeof(alertMsg), "%s \n \n  %s", alertMsgA, alertMsgB);             // forms msg body string for email
		}
		else {
			snprintf(alertSubject, sizeof(alertSubject), "%s", alertSubjectA);
			snprintf(alertMsg, sizeof(alertMsg), "%s", alertMsgA);
		}
	}

	// Periodically send an "I'm still working" message even if no warning or alarm is needed.
	if (DEBUG) { Serial.printf("Monitor I'm OK - wakeup: %d\n", (wakeupCount % IM_OK_INTERVAL)); }
	if (!alertNeeded && ((wakeupCount % IM_OK_INTERVAL) >= IM_OK_INTERVAL - 1)) {
		alertNeeded = true;
		snprintf(alertSubject, sizeof(alertSubject), "%s", SUBJECT_BATTERY_MONITOR_WORKING);
		if (BATTERY_B_MONITORED) {
			snprintf(alertMsg, sizeof(alertMsg), "Battery A: %f (%d)\n Battery B: %f (%d)", batteryA_Volts, (int)(batteryA_Volts * ADC_A_MULTIPLIER), batteryB_Volts, (int)(batteryB_Volts * ADC_B_MULTIPLIER));
		}
		else {
			snprintf(alertMsg, sizeof(alertMsg), "Battery A: %f (%d)", batteryA_Volts, (int)(batteryA_Volts * ADC_A_MULTIPLIER));
		}
	}
	return alertNeeded;
}


/*"
 * Function to prepare and send email if warning or alarm criteria are met.
 */
void sendAlertMail() {
	smtp.debug(DEBUG);

	/* Set the callback function to get the sending results */
	smtp.callback(smtpCallback);

	/* Set the session config */
	session.server.host_name = SMTP_HOST;
	session.server.port = SMTP_PORT;
	session.login.email = SMPT_SEND_FROM_EMAIL;
	session.login.password = SMPT_SEND_FROM_PASSWORD;

	/* Set the message headers */
	message.sender.name = SMPT_SEND_FROM_NAME;
	message.sender.email = SMPT_SEND_FROM_EMAIL;
	message.addRecipient(SMPT_SEND_TO_EMAIL1, SMPT_SEND_TO_EMAIL1);
	if (SMPT_SEND_TO_EMAIL2 != "") {
		message.addRecipient(SMPT_SEND_TO_EMAIL2, SMPT_SEND_TO_EMAIL2);
	}

	message.subject = alertSubject;
	message.text.content = alertMsg;
	message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

	/* Connect to server with the session config */
	if (!smtp.connect(&session))
		return;

	/* Start sending Email and close the session */
	if (!MailClient.sendMail(&smtp, &message))
		Serial.println("Error sending Email, " + smtp.errorReason());
}


/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{
	/* Print the current status */
	Serial.println(status.info());

	/* Print the sending result */
	if (status.success())
	{
		Serial.println("----------------");
		Serial.printf("Message sent success: %d\n", status.completedCount());
		Serial.printf("Message sent failled: %d\n", status.failedCount());
		Serial.println("----------------\n");
		struct tm dt;

		for (size_t i = 0; i < smtp.sendingResult.size(); i++)
		{
			/* Get the result item */
			SMTP_Result result = smtp.sendingResult.getItem(i);
			localtime_r(&result.timesstamp, &dt);

			Serial.printf("Message No: %d\n", i + 1);
			Serial.printf("Status: %s\n", result.completed ? "success" : "failed");
			Serial.printf("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
			Serial.printf("Recipient: %s\n", result.recipients);
			Serial.printf("Subject: %s\n", result.subject);
		}
		Serial.println("----------------\n");
	}
}

/*
  Method to print the reason by which ESP32
  has been awaken from sleep
*/
void getWakeUpReason() {
	esp_sleep_wakeup_cause_t wakeup_reason;
	String msg;
	wakeup_reason = esp_sleep_get_wakeup_cause();
	switch (wakeup_reason)
	{
	case ESP_SLEEP_WAKEUP_EXT0: msg = "Wakeup caused by external signal using RTC_IO"; break;
	case ESP_SLEEP_WAKEUP_EXT1: msg = "Wakeup caused by external signal using RTC_CNTL"; break;
	case ESP_SLEEP_WAKEUP_TIMER: msg = "Wakeup caused by timer"; break;
	case ESP_SLEEP_WAKEUP_TOUCHPAD: msg = "Wakeup caused by touchpad"; break;
	case ESP_SLEEP_WAKEUP_ULP: msg = "Wakeup caused by ULP program"; break;
	default: msg = "Wakeup was not caused by deep sleep -- wakeup code: " + wakeup_reason; break;
	}
	Serial.println(msg);
}


/* Displays sketch file and compilation info over the serial monitor port */
void displayRunningSketch(void) {
	String the_path = __FILE__;
	int slash_loc = the_path.lastIndexOf('/');
	String the_cpp_name = the_path.substring(slash_loc + 1);
	int dot_loc = the_cpp_name.lastIndexOf('.');
	String the_sketchname = the_cpp_name.substring(0, dot_loc);

	Serial.print("\nArduino is running Sketch: ");
	Serial.println(the_sketchname);
	Serial.print("File: ");
	Serial.println(__FILE__);
	Serial.print("Compiled on: ");
	Serial.print(__DATE__);
	Serial.print(" at ");
	Serial.println(__TIME__);
	Serial.printf("File last changed at: %s\n", __TIMESTAMP__);
	Serial.print("\n");
}
