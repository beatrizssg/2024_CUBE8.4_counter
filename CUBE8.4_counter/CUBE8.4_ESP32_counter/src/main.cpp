#include <Arduino.h>

//basic settings (pin definition, wifi credentials, etc)
#include "config.h"

//server libraries
#include <WiFi.h>
#include <HTTPClient.h>

//espnow library
#include <esp_now.h>

//eeprom library and declaration
#include <Preferences.h>
Preferences preferences;

//display libraries and declaration
#include <SPI.h> 
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); //declaration display (width, height, I2C, reset pin)

//variables
int IR_State=1;
int lastIR_State=0; //current and last state of IR Sensor
unsigned long lastIRTime=0; //last time detected

int buttonOn=0; //on/off button
int buttonReset=0; //reset button
bool systemOn=false; //system turn-on/turn off flag
bool systemState; //system state flash memory

int count=0; //IR sensor count
int lastCount; //last count flash memory 
String wifi="W:OFF/S:OFF"; //display

//flags server
bool serverOn; //=true; 
unsigned long tempoAtual;
unsigned long tempoTotal=10000;


bool qrCodeValid=0;
String qrResult;

void showDisplay(int screen)
{
	switch(screen)
	{
		case 1: //screen 1 - start
			display.clearDisplay();
			display.setTextSize(2);
			display.setTextColor(WHITE,BLACK);
			display.setCursor(0,5);
			display.print(title);
			display.setTextSize(1);
			display.setTextColor(WHITE,BLACK);
			display.setCursor(65,5);
			display.print(wifi);

			display.setTextSize(3);
			display.setCursor(60,15);
			display.print("OFF");

			display.setTextSize(1);
			display.setTextColor(WHITE,BLACK);
			display.setCursor(0,45);
			display.print(version);
			display.setCursor(0,55);
			display.print(WiFi.macAddress());
			display.display();
			break;
			
		case 2: //screen 2 - system on
			display.clearDisplay();
			display.setTextSize(2);
			display.setTextColor(WHITE,BLACK);
			display.setCursor(0,5);
			display.print(title);
			display.setTextSize(1);
			display.setTextColor(WHITE,BLACK);
			display.setCursor(65,5);
			display.print(wifi);
			display.setTextSize(3);
			display.setCursor(60,15);
			display.print("ON");
			display.display();
			break;
			
		case 3: //screen 3 - count
			display.clearDisplay();
			display.setTextSize(1);
			display.setTextColor(WHITE,BLACK);
			display.setCursor(0,5);
			display.print(title);

			display.setTextSize(1);
			display.setTextColor(WHITE,BLACK);
			display.setCursor(65,5);
			display.print(wifi);

			display.setTextSize(6);
			display.setCursor(42,20);
			display.print(count);
			display.display();
			break;
			
		case 4: //screen 4 - reset
			display.clearDisplay();
			display.setTextSize(1);
			display.setTextColor(WHITE,BLACK);
			display.setCursor(0,5);
			display.print(title);
			display.setTextSize(1);
			display.setTextColor(WHITE,BLACK);
			display.setCursor(65,5);
			display.print(wifi);
			display.setTextSize(4);
			display.setCursor(20,30);
			display.print("ZERO");
			display.display();
			break;
	}
}

void connectWifi()
{
	//connecting WiFi
	WiFi.begin(ssid, password);
	Serial.print("\nSearch networking");

	int i=0;
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(1000);
		if (millis()-tempoAtual <= tempoTotal) {
			i++;
			Serial.print(".");
		} 
		else break;
	}

	if(WiFi.status() == WL_CONNECTED)
	{
		Serial.println("WiFi connected :)");
		serverOn = true;
	}

	else
	{
		Serial.println("WiFi not connected. Starting offline.");
		serverOn=false;
	}	
}

void sendServer(int count)
{
	if(WiFi.status() == WL_CONNECTED)
	{
		WiFiClient client;
		HTTPClient http;		
		http.begin(client, serverUrl);

		http.addHeader("Content-Type", "application/x-www-form-urlencoded");
		String machine = WiFi.macAddress();	

		String request =  "machine="+machine+"&count="+String(count);

		http.setConnectTimeout(300);
		int httpResponseCode = http.POST(request);

		if (httpResponseCode>0)
		{
			serverOn = true;
			//Serial.println(http.getString());
			Serial.println("\nServer connected.");	
			digitalWrite(ledRed_Pin, 0); //red led on		
		}
		else{
			serverOn = false;
			Serial.println("Server not connected. Working offline.");
			digitalWrite(ledRed_Pin, 1); //red led on				
		}
	}
	else{
		Serial.println("WiFi não conectado. Tentando conectar...");
		connectWifi();
	}
}

//structure to send data
typedef struct struct_message{
	char qrCode[100];
} struct_message;

struct_message myData;

//callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len){
  memcpy(&myData, incomingData, sizeof(myData));
  //Serial.print("Bytes recebidos: ");
  //Serial.println(len);
  Serial.print("QR Code: ");
  Serial.println(myData.qrCode);
  Serial.println();

  qrResult = String(myData.qrCode);
  if(qrResult==qrFinal){
	qrCodeValid=1;
  }
}

void setup()
{
	Serial.begin(115200, 12, 14);

	//esp32 as access point and wifi station
	WiFi.mode(WIFI_AP_STA);
	connectWifi();

	//initialize input variables as input
	pinMode(sensorIR_Pin, INPUT); //IR sensor
	
	pinMode(buttonOn_Pin, INPUT); //on/off button
	pinMode(buttonReset_Pin, INPUT); //reset button

	//initialize output variables as output
	pinMode(ledRed_Pin, OUTPUT); //red led
	pinMode(ledGreen_Pin, OUTPUT); //green led
	pinMode(speaker_Pin, OUTPUT); //speaker

	//create namespace called "myCount"
	preferences.begin("myCount", false);
	//read last count from flash memory
  	lastCount = preferences.getInt("count", false); 
  	Serial.printf("Count before reset: %d \n", lastCount);
	//set atual count - last count
	count = lastCount;

	//create namespace called "mySystem"
	preferences.begin("mySystem", false);
	//read systemState from flash memory
  	systemState = preferences.getBool("systemOn", false);
  	Serial.printf("System State before reset: %d \n", systemState);
  	systemOn = systemState;

	//initialize OLED Display, address: 0x3C
	if(!display.begin(SSD1306_SWITCHCAPVCC, 0X3C))
	{
		Serial.println(F("Display connection failed!"));
		for(;;);
	}
	delay(200);

	if(esp_now_init() != ESP_OK){
    	Serial.println("Erro na inicializacao do ESP-NOW");
    	return;
  	}

	//callback function
	esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
  Serial.print(".");
  esp_now_register_recv_cb(OnDataRecv);

  //if restart - systemOn
  if(systemOn==1){
    showDisplay(3); //display - cont
  }
  else{
    showDisplay(1); //display - start
  }
    
  buttonOn = digitalRead(buttonOn_Pin); //reading on/off button
  buttonReset = digitalRead(buttonReset_Pin); //reading reset button

	//if on/off button was pressed or qrcode is valid
	if(buttonOn || qrCodeValid==1)
	{
		while(digitalRead(buttonOn_Pin)) //wait until it is released
		{
			Serial.println("Aguardando soltar botao");
		}
		systemOn = true; //system on
		preferences.putBool("systemOn", systemOn); //save system state in flash memory
		Serial.print("\nLigado\n");

		esp_now_deinit(); //espnow off
		sendServer(count);
		showDisplay(2);
	}

	int j=0;
	while(systemOn)
	{
		j++;
		if (j==5000)
		{
			Serial.print("\u2764 ");
			j=0;
		}

		// IR SENSOR - OBJECT DETECTION
		IR_State = digitalRead(sensorIR_Pin); //reading IR sensor

		//if object was detected (state change)
		if (!IR_State && lastIR_State)
		{
			lastIRTime = millis();
			count++; //add 1 to count			
			preferences.putInt("count", count); //save count in flash memory
			
			Serial.println("\n\nciclo \u2211 "+String(count));
			showDisplay(3);

			//blink leds
			digitalWrite(ledGreen_Pin, 1); //green led on
			delay(100);
			digitalWrite(ledGreen_Pin, 0); //green led off

			//speaker
			tone(speaker_Pin,440,100);
			noTone(speaker_Pin);

			if(serverOn)
			{
				sendServer(count);
			}

			//if reached maximum count value
			if (count>=countMax)
			{
				if(!serverOn)
				{
					sendServer(count);
				}
				
				tone(speaker_Pin,392,500);
				noTone(speaker_Pin);
				Serial.print("\nGame Over!!!\n");
				count=0;
			}
		}

		lastIR_State = IR_State; 

		//if reset button was pressed
		if(digitalRead(buttonReset_Pin))
		{
			while(digitalRead(buttonReset_Pin)); //wait until it is released
			count=0; //reset count
			Serial.print("\nReset\n");
			showDisplay(4);
		}
 
		//if on/off button was pressed
		if(digitalRead(buttonOn_Pin))
		{
			while(digitalRead(buttonOn_Pin)); //wait until it is released
			if(!serverOn)
			{
				sendServer(count);
			}

			Serial.print("\nDesligado\n");
			systemOn = false; //system off

			preferences.putBool("systemOn", systemOn); //save system state in flash memory
			digitalWrite(ledRed_Pin, 0); //red led off
			showDisplay(1);

			qrCodeValid=0;
			esp_now_init(); //esp now on
		}
  }
}