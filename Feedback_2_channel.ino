#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include <AceButton.h>

const char *service_name = "FEEDBACK";
const char *pop = "123456";

// define the Device Names
char deviceName_1[] = "Relay1";
char deviceName_2[] = "Relay2";

// define the GPIO connected with Relays and switches
static uint8_t RelayPin1 = 21;  //D21
static uint8_t SwitchPin1 = 22;  //D22

static uint8_t RelayPin2 = 19;  //D19
static uint8_t SwitchPin2 = 23;  //D23

static uint8_t wifiLed    = 2;   //D2
static uint8_t gpio_reset = 0;

/* Variable for reading pin status*/
bool toggleState_1 = HIGH; //Define integer to remember the toggle state for relay 1
bool toggleState_2 = HIGH; //Define integer to remember the toggle state for relay 2

// Relay State
bool switch_state_ch1 = HIGH; //control status of widget
bool switch_state_ch2 = HIGH; //control status of widget

using namespace ace_button;

//The framework provides some standard device types like switch, lightbulb, fan, temperature sensor.
static Switch my_switch1(deviceName_1, &RelayPin1);
static Switch my_switch2(deviceName_2, &RelayPin2);

ButtonConfig config1;
AceButton button1(&config1);
ButtonConfig config2;
AceButton button2(&config2);

void button1Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  Serial.println("EVENT1");
  switch (eventType) {
    case AceButton::kEventPressed:
      Serial.println("kEventPressed");
      switch_state_ch1 = true;
      my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch_state_ch1);
      break;
    case AceButton::kEventReleased:
      Serial.println("kEventReleased");
      switch_state_ch1 = false;
      my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch_state_ch1);
      break;
    }
}

void button2Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  Serial.println("EVENT2");
  switch (eventType) {
    case AceButton::kEventPressed:
      Serial.println("kEventPressed");
      switch_state_ch2 = true;
      my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch_state_ch2);
      break;
    case AceButton::kEventReleased:
      Serial.println("kEventReleased");
      switch_state_ch2 = false;
      my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch_state_ch2);
      break;
    }
}

void sysProvEvent(arduino_event_t *sys_event)
{
    switch (sys_event->event_id) {      
        case ARDUINO_EVENT_PROV_START:
#if CONFIG_IDF_TARGET_ESP32
        Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on BLE\n", service_name, pop);
        printQR(service_name, pop, "ble");
#else
        Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on SoftAP\n", service_name, pop);
        printQR(service_name, pop, "softap");
#endif        
        break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Serial.printf("\nConnected to Wi-Fi!\n");
        digitalWrite(wifiLed, true);
        break;
    }
}

void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx)
{
    const char *device_name = device->getDeviceName();
    const char *param_name = param->getParamName();

    // EVENTS FOR SWITCH 1
    if(strcmp(device_name, deviceName_1) == 0) {
      
      Serial.printf("Switch-1 Value = %s\n", val.val.b? "true" : "false");
      
      if(strcmp(param_name, "Power") == 0) {
         Serial.printf("Received value = %s for %s - %s\n", val.val.b? "true" : "false", device_name, param_name);
         bool widget_val_1 = val.val.b;
         if(widget_val_1 == true || widget_val_1 == false){
          toggleState_1 = !toggleState_1;
          digitalWrite(RelayPin1, toggleState_1);
          param->updateAndReport(val);
         }
       }
     }
    // EVENTS FOR SWITCH 2
    else if(strcmp(device_name, deviceName_2) == 0) {
      
      Serial.printf("Switch-2 Value = %s\n", val.val.b? "true" : "false");
      
      if(strcmp(param_name, "Power") == 0) {
         Serial.printf("Received value = %s for %s - %s\n", val.val.b? "true" : "false", device_name, param_name);
         bool widget_val_2 = val.val.b;
         if(widget_val_2 == true || widget_val_2 == false){
          toggleState_2 = !toggleState_2;
          digitalWrite(RelayPin2, toggleState_2);
          param->updateAndReport(val);
         }
       }
     }
    
}


void setup()
{
  uint32_t chipId = 0;
    
  Serial.begin(115200);
    
  // Set the Relays GPIOs as output mode
  pinMode(RelayPin1, OUTPUT);
  pinMode(RelayPin2, OUTPUT);
  pinMode(wifiLed, OUTPUT);

  // Configure the input GPIOs
  pinMode(SwitchPin1, INPUT_PULLUP);
  pinMode(SwitchPin2, INPUT_PULLUP);
  pinMode(gpio_reset, INPUT);
  
  // Switch off relay's by default
  digitalWrite(RelayPin1, HIGH);
  digitalWrite(RelayPin2, HIGH);

  config1.setEventHandler(button1Handler);
  config2.setEventHandler(button2Handler);
  
  button1.init(SwitchPin1);
  button2.init(SwitchPin2);

  //------------------------------------------- Declaring Node -----------------------------------------------------//
  Node my_node;
  my_node = RMaker.initNode("Dual Device");

  //Standard switch device
  my_switch1.addCb(write_callback);
  my_switch2.addCb(write_callback);

  //Add switch device to the node   
  my_node.addDevice(my_switch1);
  my_node.addDevice(my_switch2);

  //This is optional 
  RMaker.enableOTA(OTA_USING_PARAMS);
  //If you want to enable scheduling, set time zone for your region using setTimeZone(). 
  //The list of available values are provided here https://rainmaker.espressif.com/docs/time-service.html
  // RMaker.setTimeZone("Asia/Shanghai");
  // Alternatively, enable the Timezone service and let the phone apps set the appropriate timezone
  RMaker.enableTZService();
  RMaker.enableSchedule();

  //Service Name
  for(int i=0; i<17; i=i+8) {
      chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  Serial.printf("\nChip ID:  %d Service Name: %s\n", chipId, service_name);

  Serial.printf("\nStarting ESP-RainMaker\n");
  RMaker.start();

  WiFi.onEvent(sysProvEvent);
#if CONFIG_IDF_TARGET_ESP32
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name);
#else
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, pop, service_name);
#endif

    //getRelayState(); // Get the last state of Relays
    my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
    my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
}

void loop()
{
    button1.check();
    button2.check();
    
    // Read GPIO0 (external button to reset device
    if(digitalRead(gpio_reset) == LOW) { //Push button pressed
        Serial.printf("Reset Button Pressed!\n");
        // Key debounce handling
        delay(100);
        int startTime = millis();
        while(digitalRead(gpio_reset) == LOW) delay(50);
        int endTime = millis();

        if ((endTime - startTime) > 10000) {
          // If key pressed for more than 10secs, reset all
          Serial.printf("Reset to factory.\n");
          RMakerFactoryReset(2);
        } else if ((endTime - startTime) > 3000) {
          Serial.printf("Reset Wi-Fi.\n");
          // If key pressed for more than 3secs, but less than 10, reset Wi-Fi
          RMakerWiFiReset(2);
        }
    }
    delay(100);

    if (WiFi.status() != WL_CONNECTED)
    {
      //Serial.println("WiFi Not Connected");
      digitalWrite(wifiLed, LOW);
    }
    else
    {
      //Serial.println("WiFi Connected");
      digitalWrite(wifiLed, HIGH);
    }
    
}



    
