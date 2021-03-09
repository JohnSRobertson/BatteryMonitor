/*
 * Configuration Parameters For Battery Monitor Using an ESP32 Development Board
 */

/* Battery Voltage Warning / Alarm Values */
#define BATTERY_A_WARN_LOW	11.5	// Floating point values in volts (no quotes)
#define BATTERY_B_WARN_LOW	11.5
#define BATTERY_A_ALARM_LOW	10.5
#define BATTERY_B_ALARM_LOW	10.5
#define SUBJECT_BATTERY_MONITOR_WORKING	"OK: Batteries Charged"		// Email subject line text strings.  Keep less than 1/2 BUF_SIZE in length.
#define SUBJECT_BATTERY_A_ALARM_LOW		"Alarm: Battery A (Engine) "
#define SUBJECT_BATTERY_B_ALARM_LOW		"Alarm: Battery B (DeepCycle) "
#define SUBJECT_BATTERY_A_WARN_LOW		"Warn: Battery A (Engine) "
#define SUBJECT_BATTERY_B_WARN_LOW		"Warn: Battery B (DeepCycle) "

/* ESP32 DEEP SLEEP TIMER */
#define	TIME_TO_SLEEP	1800	// Time the ESP32 will go into deep sleep (Integer seconds)
#define	IM_OK_INTERVAL	1		// Number of wakeups before sending an "OK: Batteries Charged" message (when no other alert occured) (Integer)

/* ANALOG TO DIGITAL VOLTAGE MEASUREMENT */
#define	ADC_PIN_A	34				// ADC Pin used to sample #1 Battery (Engine)
#define	ADC_PIN_B	35				// ADC Pin used to sample #2 Battery (House Batteries)
#define ADC_A_MULTIPLIER 189		// Factor (for specific voltage divider resistors) to convert ADC A value to volts (determined by voltage measurements)
#define ADC_B_MULTIPLIER 179		// Factor to convert adc B returned value to volts
#define	NUM_READINGS	25			// number of voltage samples used to compute average
#define	DELAY_BETWEEN_READINGS	50	// mSec delay between ADC samples
#define BATTERY_B_MONITORED	true  // Set true if a second battery is monitored

/* PROGRAM PARAMETERS */
#define NUM_WIFI_ATTEMPTS 25	// Number of retry attempts to log onto the wifi network before abandonment.
#define WIFI_RETRY_INTERVAL 100 // Milliseconds between wifi retry attempt
#define BUF_SIZE 128			// Size of utility buffers used for strings

/* DEVELOPMENT AND DEBUGGING PARAMETERS */
#define SEND_MAIL true  // Prevents the sending of email messages during dev testing.  Set true for production.
#define DEBUG     true  // Enables serial port output of detailed debugging messages.  Set false for production.