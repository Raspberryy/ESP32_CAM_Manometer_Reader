#include <WiFi.h>
#include "esp_camera.h"
#include "esp_http_client.h"
#include "Arduino.h"

// *****************
// ** Definitions **
// *****************
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LAMP 4

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_LOAD  300         /* Time ESP32 will load capacitors (in seconds) */

#define LED_ERROR_SIGNAL  0     // [0|1]
#define DETAILED_HTTP_STATUS 0  // [0|1]

#define FLASH_FOR_CAM 1         // [0|1]

// ************
// ** Gobals **
// ************
  String baseURL = "http://192.168.1.1/esp32RecData.php";
  const char* ssid = "rp02";
  const char* passwd = "6fe0e47522afd48aed047686e305186c";
  uint64_t TIME_TO_SLEEP = 30;
  
// *************
// ** Methods **
// *************
void loadCapacitors();
bool initCamera();
bool initWifi();
void showLEDError(int _flashes);
esp_err_t http_event_handler(esp_http_client_event_t *evt);
esp_err_t take_send_photo();

// **************
// **   Main   **
// **************
void setup() {
  // Init Serial
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

  // Check if caps need to be loaded
     if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_TIMER)
       loadCapacitors();
     else
       Serial.println("[*] Capacitors loaded. Start init");
     
  // Init LED
    pinMode(LAMP, OUTPUT);
    digitalWrite(LAMP, LOW);

  // Main Init
  if(LED_ERROR_SIGNAL){
    if(!initCamera())
      showLEDError(2);
    if(!initWifi())
      showLEDError(3);
  
  } else {
    if(initCamera() && initWifi())
      Serial.println("[*] INIT SUCCESSFUL");
    else 
      Serial.println("[!] INIT FAILED");
  }
}

// If TIME_TP_SLEEP > 0 this will not be executed
void loop() {
   take_send_photo();
   Serial.println("[*] Sleeping for " + String(TIME_TO_LOAD) + " Seconds before taking next photo...");
   delay(TIME_TO_SLEEP*1000);
}

// *********************
// ** Implementations **
// *********************

void loadCapacitors(){
  esp_sleep_enable_timer_wakeup(TIME_TO_LOAD * uS_TO_S_FACTOR);
  
  Serial.println("[*] Loading Capacitors for " + String(TIME_TO_LOAD) + " Seconds now!");
  Serial.flush(); 
  
  esp_deep_sleep_start();
}

bool initCamera(){

  Serial.println("[*] Init Camera");
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
    
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[!] Camera init failed with error 0x%x", err);
    return false;
  }
  
  Serial.println("[*] Camera Init successful");
  return true;
}


bool initWifi(){

  WiFi.begin(ssid, passwd);
  Serial.print("[*] Connect to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("[*] WiFi connected on ");
  Serial.print(WiFi.localIP());
  Serial.print(" (");
  Serial.print(WiFi.RSSI());
  Serial.println("dBm)");

  return true;
}

// *****************************************
// ** Shows Error Codes with LED flasges  ** 
// *****************************************
// ** CODE    Description                 **
// ** 01      /                           **
// ** 02      Camera init failed          **
// ** 03      Wifi init failed            **
// ** 04      /                           **
// ** 05      /                           **
// *****************************************
void showLEDError(int _flashes){

  while(_flashes > 0)
  {
      digitalWrite(LAMP, HIGH);
      delay(500);
      digitalWrite(LAMP, LOW);
      delay(200);
      _flashes--;
  }
  
}

// *********************************
// ** Serial print of HTTP Event  ** 
// *********************************
esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
  // Check if detailed Info is wanted
    if(!DETAILED_HTTP_STATUS)
      return ESP_OK;
 
  switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
      Serial.println("[!] HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      Serial.println("[*] HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      Serial.println("[*] HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      Serial.println();
      Serial.printf("[*] HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      Serial.println();
      Serial.printf("[*] HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      break;
    case HTTP_EVENT_ON_FINISH:
      Serial.println("");
      Serial.println("[*] HTTP_EVENT_ON_FINISH");
      break;
    case HTTP_EVENT_DISCONNECTED:
      Serial.println("[*] HTTP_EVENT_DISCONNECTED");
      break;
  }
  return ESP_OK;
}

// ****************************************
// ** Takes Image and send it to baseURL ** 
// ****************************************
esp_err_t take_send_photo()
{
  // Take Image
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    if (FLASH_FOR_CAM) {
      digitalWrite(LAMP, HIGH);
      delay(300);
      fb = esp_camera_fb_get();
      digitalWrite(LAMP, LOW);
    } else {
      fb = esp_camera_fb_get();
    }
      
    if (!fb) {
      Serial.println("[!] Camera capture failed");
      return ESP_FAIL;
    }

  // Prepare HTTP Client
    esp_http_client_handle_t http_client;
    String url = (baseURL + "?ip=" + WiFi.localIP().toString() + "&quality=" + WiFi.RSSI() + "dBm");
    esp_http_client_config_t config_client = {0};
    config_client.url = const_cast<char*>(url.c_str());
    config_client.event_handler = http_event_handler;
    config_client.method = HTTP_METHOD_POST;
    http_client = esp_http_client_init(&config_client);

  // Send Image
    esp_http_client_set_post_field(http_client, (const char *)fb->buf, fb->len);
    esp_http_client_set_header(http_client, "Content-Type", "image/jpg");
    esp_err_t err = esp_http_client_perform(http_client);
  
    if (err == ESP_OK) {
      Serial.print("[*] Status Code ");
      Serial.println(esp_http_client_get_status_code(http_client));
    }

  // Clean
    esp_http_client_cleanup(http_client);

  esp_camera_fb_return(fb);
}
