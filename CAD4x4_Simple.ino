#include <U8g2lib.h>
#include <Wire.h>
#include <HX711_ADC.h>
#include <ArduinoHttpClient.h>
#include <WiFi101.h>
#include <ArduinoJson.h>
#include <Button.h>


// Oled Display contructor
// Update the pin numbers according to your setup. Use U8X8_PIN_NONE if the reset pin is not connected
// The complete list is available here: https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);


//HX711 constructor (dout pin, sck pin)
HX711_ADC LoadCell(0, 5);




/////// Wifi Settings ///////
char ssid[] = "your SSID";
char pass[] = "Your PASS";

const char serverAddress[] = "server name";  // server address
int port = 80;
String endPoint = "EndPoint";//
String access_token = "ACCESS_TOCKEN"; // use the access_token we provided you
String deviceID = "DeviceID";//use the device id we provided you


WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);
int status = WL_IDLE_STATUS;
int statusCode = 0;
String response;


bool deviceIsReady = false;
String deviceStatus = "Starting";
bool shouldSendData = false;
long t;
int lastIngredientBack;//hold the number of the last igredient 



// Uses debounce mode. You may need to experiment with the debounce duration
// for your particular switch.

Button ingredient1 = Button(1, BUTTON_PULLUP_INTERNAL, true, 50);
Button ingredient2 = Button(2, BUTTON_PULLUP_INTERNAL, true, 50);
Button ingredient3 = Button(3, BUTTON_PULLUP_INTERNAL, true, 50);
Button ingredient4 = Button(4, BUTTON_PULLUP_INTERNAL, true, 50);

// Callbacks
// "Button& b" is a reference to the button that triggered the callback
// This way we can access its information so we know which button it was

void onPress(Button& b){//when ingredient is placed
//Avoid the first onPress calls 
  if (deviceIsReady) {
    //set device not ready to avoid calls while communication with server
    //An other way to handel it is to queue requests and process them on by one
    deviceIsReady = false;
    deviceStatus = "Please wait...";
    showDeviceStatus(deviceStatus);

    //set lastIngredientBack to the current ingredient
    lastIngredientBack = b.pin;

    //each time an ingredient is back, we read the value and send it to the server.
    //the server will then process it ,subtracts the container weight and returns the new weight.
    //Because the scale need time to stablize and we can't use delay, 
    //we must set shouldSendData to true and reset t
    shouldSendData = true;
    t = millis();
  }
  

}
void onRelease(Button& b){//when ingredient is taken away
  showDeviceStatus(deviceStatus);
  //we need to set the LoadCell to zero each time an ingredient is taken away
  LoadCell.tare();


}



void connectToWifi(){
   while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);     // print the network name (SSID);

    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
  }

    // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

void setup(){
  Serial.begin(9600);
  
  u8g2.begin();
  //setUp LoadCells
  LoadCell.begin();
  long stabilisingtime = 1000; // tare preciscion can be improved by adding a few seconds of stabilising time
  LoadCell.start(stabilisingtime);
  LoadCell.setCalFactor(499.0); // set Your calibration factor (float)

  showDeviceStatus(deviceStatus);

  // Assign callback functions
  
  ingredient1.releaseHandler(onRelease);
  ingredient1.pressHandler(onPress);

  
  ingredient2.releaseHandler(onRelease);
  ingredient2.pressHandler(onPress);

  ingredient3.releaseHandler(onRelease);
  ingredient3.pressHandler(onPress);

  ingredient4.releaseHandler(onRelease);
  ingredient4.pressHandler(onPress);


}

void loop(){

  if (millis() > 5000 && millis() < 6000 && !deviceIsReady) {

    deviceIsReady = true;
    deviceStatus = "Ready";
    showDeviceStatus(deviceStatus); 
  }
  //showDeviceStatus(deviceStatus);

  // update the buttons' internals
  ingredient1.process();
  ingredient2.process();
  ingredient3.process();
  ingredient4.process();

  
  //update() should be called at least as often as HX711 sample rate; >10Hz@10SPS, >80Hz@80SPS
  //longer delay in scetch will reduce effective sample rate (be carefull with delay() in loop)
  LoadCell.update();
  if (millis() > t + 2000 && shouldSendData) {
    
    int i = LoadCell.getData();
    Serial.print("Load_cell output val: ");
    Serial.println(i);
    //send data
    deviceStatus = "Sending...";
    showDeviceStatus(deviceStatus);
    makeRequest();
    //clean up
    shouldSendData = false;
    LoadCell.tare();
    deviceIsReady = true;
    deviceStatus = "Ready";
  }
}

void makeRequest(){
  //1- connect to  your wifi
       connectToWifi();
  //2- Get the new wight
       int value = LoadCell.getData();
  //3- Construct path
       String path = endPoint + access_token + "&deviceId=" + deviceID  + lastIngredientBack + "&value=" + value;
  //4- send the GET request
       Serial.println("making GET request");
       client.get(path);

  //5- Read the status code and body of the response
       statusCode = client.responseStatusCode();
       response = client.responseBody();
       Serial.print("Status code: ");
       Serial.println(statusCode);
       Serial.print("Response: ");
       Serial.println(response);
  //6- Parse response to json
       StaticJsonBuffer<200> jsonBuffer;
       JsonObject& root = jsonBuffer.parseObject(response);


  //7- Test if parsing succeeds.
       if (!root.success()) {
        //display Error 
         u8g2.clearBuffer();          // clear the internal memory
         u8g2.setFont(u8g2_font_profont17_tf);
         u8g2.drawStr( 7, 18, "Error");
         u8g2.setFont(u8g2_font_profont15_tf);
         u8g2.drawStr( 7, 40, "could not parse");
         u8g2.drawStr( 7, 52, "json");
         u8g2.sendBuffer();          // transfer internal memory to the display
         
         Serial.println("parseObject() failed");
         return;
       }
  //8- Check if any error
       if(!root["success"].as<bool>()){
        //display Error 
         const char* error = root["error"];
         const char* error_description = root["error_description"];
         u8g2.clearBuffer();          // clear the internal memory
         u8g2.setFont(u8g2_font_profont17_tf);
         u8g2.drawStr( 7, 18, "Error");
         u8g2.setFont(u8g2_font_profont15_tf);
         u8g2.drawStr( 7, 40, error);
         u8g2.sendBuffer();          // transfer internal memory to the display
       }else{
         //if everything goes fine, you will get the name and the new value of the ingredient in a json format
         //no need to subtract the container weight, it is already caluculated
         const char* ingredientName = root["name"];
         const char* newValue = root["newValue"];
         u8g2.clearBuffer();          // clear the internal memory
         u8g2.setFont(u8g2_font_profont22_tf); // choose a suitable font
         u8g2.drawStr(10,30,ingredientName); // write something to the internal memory
         u8g2.drawStr(10,55,newValue);  // write something to the internal memory         
         u8g2.sendBuffer();          // transfer internal memory to the display
       }

}


void showDeviceStatus(String msg){
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_profont17_tf);  // choose a suitable font
  u8g2.drawStr( 7, 18, msg.c_str());

  //Draw empty circles for each ingredient
  u8g2.drawCircle(16,40,10);
  u8g2.drawCircle(48,40,10);
  u8g2.drawCircle(80,40,10);
  u8g2.drawCircle(116,40,10); 
  
  //check for present ingredient and draw disk
  if (ingredient4.isPressed()){ u8g2.drawDisc(16,40,7);}
  if (ingredient3.isPressed()){ u8g2.drawDisc(48,40,7);}
  if (ingredient2.isPressed()){ u8g2.drawDisc(80,40,7);}
  if (ingredient1.isPressed()){ u8g2.drawDisc(116,40,7);}
  
  u8g2.sendBuffer();          // transfer internal memory to the display

}


