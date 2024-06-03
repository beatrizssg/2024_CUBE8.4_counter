#include <Arduino.h>

//espnow library
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

//qrcode library
#include <ESP32QRCodeReader.h>

//declaration qr code reader (camera model)
ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER); 

//MAC address of the receiver (esp32)
uint8_t broadcastAddress[] = {0xCC, 0xDB, 0xA7, 0x62, 0xAF, 0x50};
esp_now_peer_info_t peerInfo; 

//structure to receive data
typedef struct struct_message{
	char qrCode[100];
} struct_message;

struct_message myData;

constexpr char WIFI_SSID[] = "AP 201_2.4G";

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

//callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  Serial.print("\r\nStatus do ultimo envio de pacote:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Entrega realizada com sucesso" : "Falha na entrega");  
}

void onQrCodeTask(void *pvParameters)
{
  struct QRCodeData qrCodeData;

  while (true)
  {
    if (reader.receiveQrCode(&qrCodeData, 100))
    {
      Serial.println("Found QRCode");
      if (qrCodeData.valid)
      {
        Serial.print("Payload: ");
        Serial.println((const char *)qrCodeData.payload);
		//setar valores para enviar
		strcpy(myData.qrCode, (const char *)qrCodeData.payload);
		//enviar mensagem
		esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
		if(result == ESP_OK)
		{
			Serial.println("Enviado com sucesso");
		}
		else
		{
			Serial.println("Erro ao enviar mensagem");
		}
      }
      else
      {
        Serial.print("Invalid: ");
        Serial.println((const char *)qrCodeData.payload);
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup() {
	Serial.begin(115200);
	Serial.println("Hello");
	//set device as wifi station and set channel
	WiFi.mode(WIFI_STA);
	int32_t channel = getWiFiChannel(WIFI_SSID);
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
	esp_wifi_set_promiscuous(false);
	
	//init ESP-Now
	if(esp_now_init() != ESP_OK){
		Serial.println("Erro na inicializacao do ESP-NOW");
		return;
	}

	//get status trasnmitted packet
	esp_now_register_send_cb(OnDataSent);

	//register peer
	memcpy(peerInfo.peer_addr, broadcastAddress, 6);
	peerInfo.channel = 0;
	peerInfo.encrypt = false;

	//add peer
	if(esp_now_add_peer(&peerInfo) != ESP_OK){
		Serial.println("Falha ao add par");
		return;
	}

	//qr reader begin and setup
	reader.setup();
	Serial.println("Setup QRCode Reader");

	reader.beginOnCore(1);
	Serial.println("Begin on Core 1");
}

void loop() {
	//read qrcodes
	xTaskCreate(onQrCodeTask, "onQrCode", 4 * 1024, NULL, 4, NULL);	
	delay(100);
}