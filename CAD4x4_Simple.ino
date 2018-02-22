

#include <Button.h>

#include <ArduinoHttpClient.h>
#include <WiFi101.h>
#include <ArduinoJson.h>

#include <HX711_ADC.h>

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



// Uses debounce mode. You may need to experiment with the debounce duration
// for your particular switch.

Button ingredient1 = Button(1, BUTTON_PULLUP_INTERNAL, true, 50);
Button ingredient2 = Button(2, BUTTON_PULLUP_INTERNAL, true, 50);
Button ingredient3 = Button(3, BUTTON_PULLUP_INTERNAL, true, 50);
Button ingredient4 = Button(4, BUTTON_PULLUP_INTERNAL, true, 50);

// Callbacks
// "Button& b" is a reference to the button that triggered the callback
// This way we can access its information so we know which button it was


void onRelease(Button& b){//when ingredient is taken away
	Serial.print("onRelease: ");
	Serial.println(b.pin);
  //we need to set the LoadCell to zero each time an ingredient is taken away
  LoadCell.tare();

}

void onHold(Button& b){//when ingredient is put back
	Serial.print("Held for at least 1 second: ");
	Serial.println(b.pin);
  //each time an ingredient is back, we read the value and send it to the server.
  //the server will then compare it with the old value,subtract the container weight and give as the new weight.
  
  int i = LoadCell.getData();
  Serial.print("The new value is: ");
  Serial.println(i);
  makeRequest(b.pin,i);
  
}

void setup(){
  Serial.begin(9600);


  //setUp LoadCells
  LoadCell.begin();
  long stabilisingtime = 2000; // tare preciscion can be improved by adding a few seconds of stabilising time
  LoadCell.start(stabilisingtime);
  LoadCell.setCalFactor(696.0); // set Your calibration factor (float)

  while (!Serial);
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


  // Assign callback functions
  
  ingredient1.releaseHandler(onRelease);
  ingredient1.holdHandler(onHold, 1000); // must be held for at least 1000 ms to trigger

  
  ingredient2.releaseHandler(onRelease);
  ingredient2.holdHandler(onHold, 1000); // must be held for at least 1000 ms to trigger

  ingredient3.releaseHandler(onRelease);
  ingredient3.holdHandler(onHold, 1000); // must be held for at least 1000 ms to trigger

  ingredient4.releaseHandler(onRelease);
  ingredient4.holdHandler(onHold, 1000); // must be held for at least 1000 ms to trigger
}

void loop(){
  //update the LoadCell
  LoadCell.update();
  // update the buttons' internals
  ingredient1.process();
  ingredient2.process();
  ingredient3.process();
  ingredient4.process();
}

void makeRequest(int ingredient,int  value){


  String path = endPoint + access_token + "&deviceId=" + deviceID  + ingredient + "&value=" + value;
  Serial.println(path);
  // send the GET request
  Serial.println("making GET request");
  client.get(path);

  // read the status code and body of the response
  statusCode = client.responseStatusCode();
  response = client.responseBody();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

  StaticJsonBuffer<200> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(response);

  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  //check if any error
  if(root["error"]){
    const char* error = root["error"];
    const char* error_description = root["error_description"];

    Serial.println(error);
  }else{
    //if everything goes fine, you will the name and the new value of the ingredient in a json format
    const char* ingredientName = root["name"];
    int newValue = root["newValue"];
  }

}

