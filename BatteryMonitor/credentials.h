/* Login Information For WiFi and SMTP Mail Accounts */

/* Update with your wifi login credentials */
#define WIFI_SSID     "YOUR WIFI SSID"
#define WIFI_PASSWORD "YOUR WIFI PASSWORD"

/*
 *	EMAIL CONFIGURATION AND CREDENTIALS
 *	 Note: To send Email using Gmail use port 465 (SSL) and SMTP Server smtp.gmail.com
 *	 YOU MUST ENABLE less secure app option at https://myaccount.google.com/lesssecureapps?pli=1
 */
#define SMTP_HOST                "YOUR SMTP HOST" // e.g., "smtp.gmail.com"
#define SMTP_PORT                465              // your smtp port integer value, no quotes e.g 465
#define SMPT_SEND_FROM_NAME      "YOUR SEND FROM NAME"            // e.g., "Battery Monitor"
#define SMPT_SEND_FROM_EMAIL     "YOUR SEND FROM EMAIL ADDR"      // e.g., "xxx@gmail.com
#define SMPT_SEND_FROM_PASSWORD  "YOUR SEND FROM EMAIL PASSWORD"  // e.g., "123passw%rd"

 /* Notification Addresses */
#define SMPT_SEND_TO_EMAIL1      "YOUR PRIMARY EMAIL NOTIFICATION"		// Primary email notification address e.g., notify@gmail.com
#define SMPT_SEND_TO_EMAIL2      ""	// Second email notification address if desired (e.g, use NpaNnxXxxx@txt.att.net to send text message via email)

/*
 *	FYI - Email to Text Methods (For Text Alerts)
 *		AT&T: phonenumber@txt.att.net
 *		T-Mobile: phonenumber@tmomail.net
 *		Sprint: phonenumber@messaging.sprintpcs.com
 *		Verizon: phonenumber@vtext.com or phonenumber@vzwpix.com
 *		Virgin Mobile: phonenumber@vmobl.com
 */
