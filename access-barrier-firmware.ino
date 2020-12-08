#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>         
#include <HTTPClient.h>
#include <EEPROM.h>

AsyncWebServer server(8083);
DNSServer dns;
AsyncWiFiManager wm(&server, &dns);
HTTPClient httpClient;

String serverAddr = "";
const int EEPROM_SERVER_ADDR = 0;

String serialBuffer;
void handle_SerialCmd(String& serialBuffer);
String getIndexHtml(String inputVal, String msg);
const char* SERVER_ADDR_PARAM = "serverAddr";


void setup() {
    Serial.begin(115200);
    EEPROM.begin(512);

    // Assign fixed IP
    wm.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    
    // Try to connect WiFi, otherwise create AP
    // Note: it's a blocking function
    wm.autoConnect("ESP32_CFG", "password");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", getIndexHtml(serverAddr, "").c_str());
    });

    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
      if(request->hasParam(SERVER_ADDR_PARAM))
      {
        String reqString = request->getParam(SERVER_ADDR_PARAM)->value();
        if(serverAddr != reqString && reqString.length() <= 512)
        {
          EEPROM_put(EEPROM_SERVER_ADDR, reqString);
          serverAddr = reqString;
          request->send_P(200, "text/html", getIndexHtml(serverAddr, "Server Address changed!").c_str());
        }
          request->redirect("/");
      }
      request->send_P(400, "text/html", "NOK");
    });

    server.begin();
    Serial.println("  ##   HTTP server started");

    // Get server address from EEPROM
    serverAddr = EEPROM_get(EEPROM_SERVER_ADDR);

    delay(100); 
}


void loop() {

    handle_SerialCmd(serialBuffer);
    if(serialBuffer == "reset")
    {
      Serial.println("  ##   Erase settings and restart ...");
      delay(1000);
      wm.resetSettings(); 
      ESP.restart();  
    }
    else if(serialBuffer == "reboot")
    {
      Serial.println("  ##   Rebooting ...");
      ESP.restart();  
    }
    else if(serialBuffer == "get")
    {
      httpClient.begin(serverAddr.c_str());
      Serial.println("  ##   HTTP client started");

      // Send HTTP GET request
        int httpResponseCode = httpClient.GET();
        
        if (httpResponseCode>0) {
          Serial.print("  ##   HTTP Response code: ");
          Serial.println(httpResponseCode);
          String payload = httpClient.getString();
          Serial.print("  ##   HTTP body: ");
          Serial.println(payload);
        }
        else {
          Serial.print("  ##   Error code: ");
          Serial.println(httpResponseCode);
        }
        // Free resources
        httpClient.end();
    }
    else if(serialBuffer == "w")
    {
      String eepromVal = "localhost";
      EEPROM_put(EEPROM_SERVER_ADDR, eepromVal);
      if(eepromVal != nullptr) eepromVal = "Success";

      Serial.println("  ##   EEPROM_WRITE: " + eepromVal);
    }
}


void EEPROM_put(int addr, String data)
{
  int _size = data.length();
  int i;
  for(i=0;i<_size;i++)
  {
    EEPROM.write(addr+i,data[i]);
  }
  EEPROM.write(addr+_size,'\0');
  EEPROM.commit();
}


String EEPROM_get(int addr)
{
  int i;
  char data[100];
  int len=0;
  unsigned char k;
  k=EEPROM.read(addr);
  while(k != '\0' && len<500)
  {    
    k=EEPROM.read(addr+len);
    data[len]=k;
    len++;
  }
  data[len]='\0';
  return String(data);
}


void handle_SerialCmd(String& serialBuffer) {
  int rxByte = Serial.read();

  if(rxByte != -1)
  {
      if(rxByte != '\n')
      {
        serialBuffer += char(rxByte);
      }
      else
      {
        serialBuffer = "";
      }
  }
}


String getIndexHtml(String inputVal, String msg) {
  String indexHtml PROGMEM = R"rawliteral(
  <!DOCTYPE html>
  <html style="display: table; margin:auto;">
  <body style="display:table-cell; vertical-align: middle;">
      <h1>Access Barrier</h1>
    <form method="get" action="config">
      <label for="serverAddr">Server address:</label>
      <input type="text" name="serverAddr" style="display: block;" value="$INPUT_VAL"/>
      <button type="submit" value="save"">Save</button>
    </form>
    <p style="text-align: center;"><i>$MSG</i></p>
    </body>
  </html>
  )rawliteral";

  indexHtml.replace("$INPUT_VAL", inputVal);
  indexHtml.replace("$MSG", msg);
      
  return indexHtml;
}