# M5Stack-HA-Alarm-panel
M5stack alarm panel with RFID face to control Home Assistant MQTT alarm panel

This M5Stack Alarm panel uses RFID to disarm the alarm and buttons to arm (away or home) the alarm system.

## Introduction
Communication to Home Assistant is done via MQTT (TLS with authentication).
The M5Stack alarm panel is build to be used in combination with the [Home Assistant MQTT Alarm Panel](https://www.home-assistant.io/integrations/alarm_control_panel.mqtt)

It uses a [M5Stack Core](https://m5stack.com/collections/m5-core/products/basic-core-iot-development-kit) , a [M5Stack baseplate](https://m5stack.com/products/m5-faces-bottom-board?_pos=5&_sid=7d6c6ec02&_ss=r) and a [M5Stack RFID Face](https://m5stack.com/products/rfid-rc522-panel-for-m5-faces?_pos=2&_sid=120cb46b5&_ss=r)

## Screenshots
![Alarm Off](/images/alarm_off.png "Alarm Off") ![Alarm_pending](/images/alarm_pending.png "Alarm pending") ![Alarm_Fully On](/images/alarm_full.png "Alarm fully on")

## Configuration
To compile and use the code for your own purpose/HA configuration, just change the following settings in the Arduino code:

    const char ssid[] = "WiFi SSID";
    const char pass[] = "WiFi password"; 
    const char* mqtt_host = "mqtt.hostname (FQDN)";
    const int mqtt_port = 8883;
    const char* mqtt_user = "MQTT-userID";
    const char* mqtt_pass = "MQTT-password";
    const char* mqtt_clientId = "M5stack-alarm1";
    const char* mqtt_state_topic = "home/alarm";
    const char* mqtt_command_topic = "home/alarm/set";
    const char* mqtt_state_full = "armed_away";
    const char* mqtt_state_part = "armed_home";
    const char* mqtt_state_off = "disarmed";
    const char* mqtt_state_pending = "pending";
    const char* mqtt_command_full = "ARM_AWAY";
    const char* mqtt_command_part = "ARM_HOME";
    const char* mqtt_command_off = "DISARM";
    const char* display_state_full ="Fully Armed";
    const char* display_state_part ="Partially Armed";
    const char* display_state_off ="Switched Off";
    const char* display_state_pending ="Pending";
    const char* display_present_card = "Please present Card!";
    const char* button_a_text = "Off";
    const char* button_b_text = "Home";
    const char* button_c_text = "Away";
    
The valid RFID UID's have to be inserted into the follwing place in the Arduino Code:

    //Array with valid RFID UID's
      byte uidArray[3][4]={
      {0x11,0x22,0x33,0x44},
      {0x12,0x22,0x33,0x45},
      {0x13,0x22,0x33,0x46},  
     };

You can initially read UID from your RFID tokens by connecting a serial consiole from the Arduino IDE. Whenever you present a RFID token, the UID is logged in the serial console. On of the ToDo items is add a master token to add/delete valid tokens from the system interactive.  

If you additionally want to use readable names for the cards, define these in:

    //Array with accompanying user names 
    String nameArray[3]={
      "Card1","Card2","Card3"
    };

The names are now displayed in the header briefly and are being dumped to the serial console. But you could sent the names via MQTT and use it for displaying/Text To Speech or precense detection.

## Uploading images to SPIFFS
The pictures on the screen are PNG files loaded from the filesystem (SPIFFS) of the m5Stack. Through the Arduino IDE choose for *Tools > ESP32 Sketch Data Upload* from the menu to upload the pictures to the SPIFFS filesystem. The PNG picture files are in the [data folder](/data) of this github project. 

## Used Arduino Libraries
I use the MFRC522 I2C library from M5Stack and the [M5ez library from Rob Gonggrijp](https://github.com/ropg/M5ez) to create the button and header text fields. Due to a bug, the time does not (yet) show in the righthand corner, next to the wifi strentgh icon.

## Security 
This code uses a secured authenticated TLS connection to an MQTT server. My MQTT broker is secured using a Lets Encrypt certificate. So the Lets encrypted CA is added to the code to verify the validity of the MQTT broker certificate.

**So the communication is secured by WPA2, TLS and UserID/password. That should be sufficient for us ordinary people. But bear in mind that it is relative easy to clone an existing RFID token. https://www.getkisi.com/blog/how-to-copy-access-cards-and-keyfobs**

## M5Stack alarm panel state machine
The screens follow the **MQTT_Status_topic**, whatever the status topic is **(armed_away, armed_home, pending or disarmed)**, that screen is being showed om the M5Stack.
If a button is pressed or a card is presented, the according Command **(ARM_AWAY, ARM_HOME or DISARM** is send to the **mqtt_command_topic**. The MQTT broker dispatches this to the alarm (via HA) and the alarm changes the status through the status topiuc, again via MQTT.

## M5Stack sound bug
The I2C RFID redaer is interfering in a very annoyoing way with the onboard amplifier/speaker, resulting in a hisshing/whining sound sound. This is a known problem with the M5stack platform. To overcome this, I connected the enable amplifier input with GPIO 5 and first enable the amplifier before beeping and disabling the amplifier directly again after the beep. Read http://community.m5stack.com/topic/367/mod-to-programmatically-disable-speaker-whine-hiss to get more information.

## ToDo
* Make it more secure by using more advanced features from the RFID tokens then just the UID.
* Read valid UID's and names from a textfile of the SPIFFS filesystem.
* Add Mastercard and RFID learning mode to interactivally add and remove tokens.
* Clean up the Code and make it more robust .
* Add Availablilty topic (LWT and birth messages) so Host can see whenever panels are not online.

*I am not a programmer in any way and I have very limited time free, so please feel free to improve or extend my code with additional features.*
