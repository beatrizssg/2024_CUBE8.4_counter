//BASIC SETTINGS FILE

//definition esp32 pins 
//sensors
#define sensorIR_Pin 4 //IR sensor on D4

//buttons, leds and speaker
#define buttonOn_Pin 2 //on/off button on D2
#define buttonReset_Pin 18 //reset button on D18
#define speaker_Pin 15 //buzzer on D15
#define ledRed_Pin 19 //red led on D19
#define ledGreen_Pin 23 //green led on D23

//oled display
#define SCREEN_WIDTH 128 //width in pixels
#define SCREEN_HEIGHT 64 //height in pixels

//network credentials
const char* ssid = "AP 201_2.4G";
const char* password = "20099940";
const char* serverUrl = "http://192.168.1.10:8080/cube/cube.jsp";

//changeable variables
int countMax = 10; //maximum count value

String title = "CUBE";
String version ="V8.4";
String qrFinal = "Produto 1";