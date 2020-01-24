#include <Arduino.h>
#include <painlessMesh.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>


#define   MESH_PREFIX     "HooControls"
#define   MESH_PASSWORD   "Hoo@WWwireless"
#define   MESH_PORT       5555
#define   TAB_SSID     "qwe"
#define   TAB_PASSWORD "hoopdm123"
//#define   TAB_SSID     "G42"
//#define   TAB_PASSWORD "0507894200"
#define   HOSTNAME "MQTT_Bridge"

unsigned long connectionTimeoutMillis,
         mqtt_connect; 
bool      onn = true,
          serial_flag = false,
          mesh_flag = false,
          w_flag = true,
          valIsSame = true,
          conction_flg = false,
          calc_delay = false;
long      minRSSI = -75,
          rssi,
          connectedToRSSI;
int       networks,
          ssid_len,
          pass_addr,
          i,
          meshExitTimeout = 0,
          lowRssiCount = 0,
          noOfTimeout = 0;
String    header,
          inputString = "",
          gang = "MCU",
          myID,
          msg,
          ssid,
          STATION_SSID,
          STATION_PASSWORD;
char      inChar [193];


//################################## PROTOTYPES #####################################
void routingTable();
void reg();
void receivedCallbackNoramlNodes(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);
void receivedCallback( const uint32_t &from, const String &msg );
void mqttCallback(char* topic, byte* payload, unsigned int length);
void serial_chk();


//############################## CREATING OBJECTS ###################################
WiFiServer server(80);
IPAddress getlocalIP();
Scheduler userScheduler; // to control your personal task
IPAddress myIP(0, 0, 0, 0);
IPAddress mqttBroker2(210, 2, 139, 183);
SimpleList<uint32_t> nodes;
painlessMesh  mesh;
WiFiClient wifiClient;
PubSubClient mqttClient(mqttBroker2, 1883, mqttCallback, wifiClient);

Task taskserial_chk( TASK_MILLISECOND * 1, TASK_FOREVER, &serial_chk ); // start with a one second interval
Task taskreg( TASK_SECOND * 2, TASK_FOREVER, &reg ); // start with a one second interval
Task taskroutingTable( TASK_MINUTE * 1, TASK_FOREVER, &routingTable ); // start with a one second interval

//######################### DATA STRINGS FOR BACKTRACK ##############################
byte on_1[] = {0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x01, 0x01, 0x00, 0x01, 0x01, 0x0E};
byte off_1[] = {0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x01, 0x01, 0x00, 0x01, 0x00, 0x0D};
byte on_2[] = {0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x02, 0x01, 0x00, 0x01, 0x01, 0x0F};
byte off_2[] = {0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x02, 0x01, 0x00, 0x01, 0x00, 0x0E};
byte on_3[] = {0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x01, 0x10};
byte off_3[] = {0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x00, 0x0F};
byte on_4[] = {0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x04, 0x01, 0x00, 0x01, 0x01, 0x11};
byte off_4[] = {0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x04, 0x01, 0x00, 0x01, 0x00, 0x10};
byte on_5[] = {0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x05, 0x01, 0x00, 0x01, 0x01, 0x12};
byte off_5[] = {0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x05, 0x01, 0x00, 0x01, 0x00, 0x11};
byte allOn[] = {0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x01, 0x01, 0x00, 0x01, 0x01, 0x0E, 0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x65, 0x01, 0x00, 0x01, 0x01, 0x72, 0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x66, 0x01, 0x00, 0x01, 0x01, 0x73, 0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x02, 0x01, 0x00, 0x01, 0x01, 0x0F, 0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x01, 0x10, 0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x04, 0x01, 0x00, 0x01, 0x01, 0x11, 0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x05, 0x01, 0x00, 0x01, 0x01, 0x12, 0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x06, 0x01, 0x00, 0x01, 0x01, 0x13};
byte allOff[] = {0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x01, 0x01, 0x00, 0x01, 0x00, 0x0D, 0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x65, 0x01, 0x00, 0x01, 0x00, 0x71, 0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x66, 0x01, 0x00, 0x01, 0x00, 0x72, 0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x02, 0x01, 0x00, 0x01, 0x00, 0x0E, 0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x00, 0x0F, 0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x04, 0x01, 0x00, 0x01, 0x00, 0x10, 0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x05, 0x01, 0x00, 0x01, 0x00, 0x11, 0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x06, 0x01, 0x00, 0x01, 0x00, 0x12};
byte lastString[] = {0x55, 0xAA, 0x01, 0x07, 0x00, 0x05, 0x01, 0x01, 0x00, 0x01, 0x00, 0x0F, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x05, 0x02, 0x01, 0x00, 0x01, 0x00, 0x10, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x00, 0x11, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x05, 0x04, 0x01, 0x00, 0x01, 0x00, 0x12, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x05, 0x05, 0x01, 0x00, 0x01, 0x00, 0x13, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x05, 0x06, 0x01, 0x00, 0x01, 0x00, 0x14, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x05, 0x65, 0x01, 0x00, 0x01, 0x00, 0x73, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x05, 0x66, 0x01, 0x00, 0x01, 0x00, 0x74, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x08, 0x07, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x08, 0x08, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x1D, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x08, 0x09, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x08, 0x0A, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x08, 0x0B, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x20, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x08, 0x0C, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x21, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x08, 0x67, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x08, 0x68, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x7D, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x08, 0x69, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x08, 0x6A, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x08, 0x6B, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x80, 0x55, 0xAA, 0x01, 0x07, 0x00, 0x08, 0x6C, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x81};


void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Setup Finish!!");
  delay(100);
  connectionTimeoutMillis = millis();
}


void newConnectionCallback(uint32_t nodeId) {
  //  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}


void changedConnectionCallback() {
  //  Serial.printf("Changed connections %s\n", mesh.subConnectionJson().c_str());
  nodes = mesh.getNodeList();

  //  Serial.printf("Num nodes: %d\n", nodes.size());
  //  Serial.printf("Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    //    Serial.printf(" %u", *node);
    node++;
  }
  //  Serial.println();
  calc_delay = true;
}


void nodeTimeAdjustedCallback(int32_t offset) {
  //  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}


void delayReceivedCallback(uint32_t from, int32_t delay) {
  //  Serial.printf("Delay to node %u is %d us\n", from, delay);
}


void receivedCallback( const uint32_t &from, const String &msg ) {
  Serial.printf("==>>1 Received from %u msg=%s\n", from, msg.c_str());
  if (msg == "ON1") {
    Serial.write(on_1, sizeof(on_1));
    sendACK(msg);
  }

  else if (msg == "ON2") {
    Serial.write(on_2, sizeof(on_2));
    sendACK(msg);
  }

  else if (msg == "ON3") {
    Serial.write(on_3, sizeof(on_3));
    sendACK(msg);
  }

  else if (msg == "ON4") {
    Serial.write(on_4, sizeof(on_4));
    sendACK(msg);
  }

  else if (msg == "OF1") {
    Serial.write(off_1, sizeof(off_1));
    sendACK(msg);
  }

  else if (msg == "OF2") {
    Serial.write(off_2, sizeof(off_2));
    sendACK(msg);
  }

  else if (msg == "OF3") {
    Serial.write(off_3, sizeof(off_3));
    sendACK(msg);
  }

  else if (msg == "OF4") {
    Serial.write(off_4, sizeof(off_4));
    sendACK(msg);
  }

  else if (msg == "ON5") {
    Serial.write(on_5, sizeof(on_5));
    sendACK(msg);
  }

  else if (msg == "OF5") {
    Serial.write(off_5, sizeof(off_5));
    sendACK(msg);
  }

  else if (msg == "AllON") {
    Serial.write(allOn, sizeof(allOn));
    sendACK(msg);
  }

  else if (msg == "AllOFF") {
    Serial.write(allOff, sizeof(allOff));
    sendACK(msg);
  }

  else if (msg == "RESET") {
    ESP.restart();
  }

  else if (msg == "Registred") {
    taskreg.disable();
    EEPROM.write(0x07, 'Y');
    EEPROM.commit();
  }

  else {
    //  Serial.println(":::::ELSE Condition::::");
    String comp = String(msg.charAt(0));
    if (comp == "#") {
      Serial.println("Writing SSID to EEPROM");
      String new_ssid = msg;
      ssid_len = new_ssid.length();
      EEPROM.write(0x04, ssid_len);
      for (int s = 0; s < ssid_len; s++) {
        EEPROM.write(0x10 + s, new_ssid[s]);
      }
      EEPROM.commit();
    }

    else if (comp == "*") {
      Serial.println("Writing Password to EEPROM");
      String new_pass = msg;
      int pass_len = new_pass.length();
      EEPROM.write(0x05, pass_len);
      pass_addr = 0x10 + ssid_len + 1;
      for (int p = 0 ; p < new_pass.length(); p++) {
        EEPROM.write(pass_addr + p, new_pass[p]);
      }
      EEPROM.commit();
      mqttClient.disconnect();
      WiFi.disconnect();
      delay(1000);
      ESP.restart();
    }

    else if (comp == "$") {
      //    Serial.println(msg);
      mqttClient.publish("HooPDM/from/gatewayReg", msg.c_str());
    }

    else if (comp == "@") {
      mqttClient.publish("HooPDMbackTrack", msg.c_str());
    }

    else if (comp == "&") {
      mqttClient.publish("HooPDM/from/StateACK", msg.c_str());
    }
  }
}


void receivedCallbackNoramlNodes(uint32_t from, String & msg) {
  Serial.printf("==>>2 Received from %u msg=%s\n", from, msg.c_str());
  if (msg == "ON1") {
    Serial.write(on_1, sizeof(on_1));
    sendACK(msg);
  }

  else if (msg == "ON2") {
    Serial.write(on_2, sizeof(on_2));
    sendACK(msg);
  }

  else if (msg == "ON3") {
    Serial.write(on_3, sizeof(on_3));
    sendACK(msg);
  }

  else if (msg == "ON4") {
    Serial.write(on_4, sizeof(on_4));
    sendACK(msg);
  }

  else if (msg == "OF1") {
    Serial.write(off_1, sizeof(off_1));
    sendACK(msg);
  }

  else if (msg == "OF2") {
    Serial.write(off_2, sizeof(off_2));
    sendACK(msg);
  }

  else if (msg == "OF3") {
    Serial.write(off_3, sizeof(off_3));
    sendACK(msg);
  }

  else if (msg == "OF4") {
    Serial.write(off_4, sizeof(off_4));
    sendACK(msg);
  }

  else if (msg == "ON5") {
    Serial.write(on_5, sizeof(on_5));
    sendACK(msg);
  }

  else if (msg == "OF5") {
    Serial.write(off_5, sizeof(off_5));
    sendACK(msg);
  }

  else if (msg == "AllON") {
    Serial.write(allOn, sizeof(allOn));
    sendACK(msg);
  }

  else if (msg == "AllOFF") {
    Serial.write(allOff, sizeof(allOff));
    sendACK(msg);
  }

  else if (msg == "RESET") {
    ESP.restart();
  }

  else if (msg == "Registred") {
    taskreg.disable();
    EEPROM.write(0x07, 'Y');
    EEPROM.commit();
  }

  else {
    //  Serial.println(":::::ELSE Condition::::");
    String comp = String(msg.charAt(0));
    if (comp == "#") {
      Serial.println("Writing SSID to EEPROM");
      String new_ssid = msg;
      ssid_len = new_ssid.length();
      EEPROM.write(0x04, ssid_len);
      for (int s = 0; s < ssid_len; s++) {
        EEPROM.write(0x10 + s, new_ssid[s]);
      }
      EEPROM.commit();
    }

    else if (comp == "*") {
      Serial.println("Writing Password to EEPROM");
      String new_pass = msg;
      int pass_len = new_pass.length();
      EEPROM.write(0x05, pass_len);
      pass_addr = 0x10 + ssid_len + 1;
      for (int p = 0 ; p < new_pass.length(); p++) {
        EEPROM.write(pass_addr + p, new_pass[p]);
      }
      EEPROM.commit();
      mqttClient.disconnect();
      WiFi.disconnect();
      delay(1000);
      ESP.restart();
    }

    else if (comp == "$") {
      //    Serial.println(msg);
      mqttClient.publish("HooPDM/from/gatewayReg", msg.c_str());
    }

    else if (comp == "@") {
      mqttClient.publish("HooPDMbackTrack", msg.c_str());
    }

    else if (comp == "&") {
      mqttClient.publish("HooPDM/from/StateACK", msg.c_str());
    }
  }
}


void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  //  Serial.println("MQTT Received");
  //  Serial.println(topic);
  char* cleanPayload = (char*)malloc(length + 1);
  payload[length] = '\0';
  memcpy(cleanPayload, payload, length + 1);
  msg = String(cleanPayload);
  free(cleanPayload);

  String targetStr = String(topic).substring(10);
  //##########################################
  if (targetStr == "gateway")
  {
    if (msg == "getNodes")
    {
      uint32_t list;
      mqttClient.publish("HooPDM/from/gateway", mesh.subConnectionJson().c_str());
      //      Serial.println(mesh.subConnectionJson().c_str());
    }
  }
  //##########################################
  else if (targetStr == "broadcast")
  {
    Serial.println(msg);
    mesh.sendBroadcast(msg);
    if (msg == "AllON") {
      Serial.write(allOn, sizeof(allOn));
      sendACK(msg);
    }

    else if (msg == "AllOFF") {
      Serial.write(allOff, sizeof(allOff));
      sendACK(msg);
    }

    else if (msg == "RESET") {
      ESP.restart();
    }
  }
  //##########################################
  else if (targetStr == myID) {
    Serial.println(msg);
    if (msg == "ON1") {
      Serial.write(on_1, sizeof(on_1));
      sendACK(msg);
    }

    else if (msg == "ON2") {
      Serial.write(on_2, sizeof(on_2));
      sendACK(msg);
    }

    else if (msg == "ON3") {
      Serial.write(on_3, sizeof(on_3));
      sendACK(msg);
    }

    else if (msg == "ON4") {
      Serial.write(on_4, sizeof(on_4));
      sendACK(msg);
    }

    else if (msg == "OF1") {
      Serial.write(off_1, sizeof(off_1));
      sendACK(msg);
    }

    else if (msg == "OF2") {
      Serial.write(off_2, sizeof(off_2));
      sendACK(msg);
    }

    else if (msg == "OF3") {
      Serial.write(off_3, sizeof(off_3));
      sendACK(msg);
    }

    else if (msg == "OF4") {
      Serial.write(off_4, sizeof(off_4));
      sendACK(msg);
    }

    else if (msg == "ON5") {
      Serial.write(on_5, sizeof(on_5));
      sendACK(msg);
    }

    else if (msg == "OF5") {
      Serial.write(off_5, sizeof(off_5));
      sendACK(msg);
    }

    else if (msg == "AllON") {
      Serial.write(allOn, sizeof(allOn));
      sendACK(msg);
    }

    else if (msg == "AllOFF") {
      Serial.write(allOff, sizeof(allOff));
      sendACK(msg);
    }

    else if (msg == "RESET") {
      ESP.restart();
    }

    else if (msg == "Registred") {
      taskreg.disable();
      EEPROM.write(0x07, 'Y');
      EEPROM.commit();
    }
  }
  //##########################################
  else if (targetStr == "Router_SSID") {
    String new_ssid = msg;
    ssid_len = new_ssid.length();
    EEPROM.write(0x04, ssid_len);
    for (int s = 0; s < ssid_len; s++) {
      EEPROM.write(0x10 + s, new_ssid[s]);
    }
    EEPROM.commit();
  }

  else if (targetStr == "Router_Pass") {
    String new_pass = msg;
    int pass_len = new_pass.length();
    EEPROM.write(0x05, pass_len);
    pass_addr = 0x10 + ssid_len + 1;
    for (int p = 0 ; p < new_pass.length(); p++) {
      EEPROM.write(pass_addr + p, new_pass[p]);
    }
    EEPROM.commit();
    String WiFiConfig = "WiFiAck";
    mqttClient.publish("HooPDM/from/WiFiConfig", WiFiConfig.c_str());
    delay(1000);
    ESP.restart();
  }

  else
  {
    uint32_t target = strtoul(targetStr.c_str(), NULL, 10);
    if (mesh.isConnected(target))
    {
      mesh.sendSingle(target, msg);
    }
    else
    {
      mqttClient.publish("HooPDM/from/gateway", "Client not connected!");
      Serial.println("Client not connected!");
    }
    //    loop();
  }
}


IPAddress getlocalIP() {
  return IPAddress(mesh.getStationIP());
}


//################################  ROUTING TABLE ##############################
void routingTable() {
  String routing =  mesh.subConnectionJson().c_str();
  String routing1 = routing.substring(0, 70);
  String routing2 = routing.substring(70, 140);
  String routing3 = routing.substring(140, 210);
  String routing4 = routing.substring(210, 280);
  mqttClient.publish("HooPDM/from/RoutingTab", routing1.c_str());
  mqttClient.publish("HooPDM/from/RoutingTab", routing2.c_str());
  mqttClient.publish("HooPDM/from/RoutingTab", routing3.c_str());
  mqttClient.publish("HooPDM/from/RoutingTab", routing4.c_str());
}


//################################  REGESTRATION ###############################
void reg() {
  if (EEPROM.read(0x07) != 'Y') {
    String regID = "$" + myID + ":" + gang;
    if (mesh_flag) {
      mesh.sendBroadcast(regID);
    }
    else
      mqttClient.publish("HooPDM/from/gatewayReg", regID.c_str());
  }
}


//################################  ACKNOWLEDGEMENT ###############################
void sendACK(String ACKmsg) {
  String msgACK =  "&" + myID + "," + gang + ACKmsg + "OK";
  if (mesh_flag) {
    mesh.sendBroadcast(msgACK);
  }
  else
    mqttClient.publish("HooPDMbackTrack", msgACK.c_str());
}


//###################################  BACKTRACK ##################################
void sendBackTrack(String backTrack) {
  String trackMsg =  "@" + myID + "," + gang + backTrack;
  if (mesh_flag) {
    mesh.sendBroadcast(trackMsg);
  }
  else
    mqttClient.publish("HooPDMbackTrack", trackMsg.c_str());
}


//######################### CHECK SERIAL DATA FOR BACKTRACK ########################
void serial_chk() {
  while (Serial.available()) {
    Serial.readBytes(inChar, 193);
    for (int b = 0; b < 192; b++) {
      if (inChar[b] != lastString[b]) {
        valIsSame = false;
      }
    }
    if (!valIsSame) {
      memcpy(lastString, inChar, sizeof(inChar));
      //      byte switchData[] = {lastString[11], lastString[12], lastString[23], lastString[24], lastString[35], lastString[36], lastString[47], lastString[48], lastString[59], lastString[60], lastString[71], lastString[72], lastString[83], lastString[84], lastString[95], lastString[96]};
      byte switchData[] = {lastString[12], lastString[24], lastString[36], lastString[48], lastString[60]};
      String switchDataString = String((char*)switchData);
      sendBackTrack(switchDataString);
      valIsSame = true;
    }
  }
}


//############################# ACTIVITY FOR ROOT NODES ############################
void rootActivity(int wifiTimeout) {
  while (lowRssiCount <= 5) {
    mesh.update();
    userScheduler.execute(); // it will run mesh scheduler as well
    mqttClient.loop();
    myID = mesh.getNodeId();

    if (myIP != getlocalIP()) {
      myIP = getlocalIP();
      mqtt_connect = millis();
      Serial.println("My IP is " + myIP.toString());
      const char *clientID = myID.c_str();
      if (mqttClient.connect(clientID)) {
        mqttClient.publish("HooPDM/from/gateway", clientID);
        mqttClient.subscribe("HooPDM/to/#");
        Serial.println("Connected to MQTT Broker!");
      }
    }

    int stat = WiFi.status();
    long connectedToRSSI = WiFi.RSSI();

    //#############################################################
    if (stat != WL_CONNECTED && conction_flg == false)
    { delay(50);
      Serial.print(".");
      Serial.println(myID);
      wifiTimeout--;
      //      Serial.println("TimeOut: " + wifiTimeout);
    }

    else if (stat == WL_CONNECTED && conction_flg == false)
    { if (millis() - connectionTimeoutMillis > 7000) {
        conction_flg = true;
      }
    }

    else if (connectedToRSSI < minRSSI) {
      lowRssiCount = lowRssiCount++;
      //      Serial.println("Low RSSI Count: " + lowRssiCount);
    }

    else if (stat != WL_CONNECTED && conction_flg == true)
    {
      mesh.stop();
      lowRssiCount = 6;
      //      Serial.println("stat != WL_CONNECTED && conction_flg == true");
    }
    //#############################################################

    if (wifiTimeout == 0) {
      mesh.stop();
      noOfTimeout++;
      lowRssiCount = 6;
      //      Serial.println("END OF ROOT ACT (TimeOut)" + wifiTimeout);
    }

    if (mqttClient.connected() == false && (millis() - mqtt_connect) >= 30000) {
      //      Serial.println("Not connected to MQTT Broker!");
      const char *clientID = myID.c_str();
      if (mqttClient.connect(clientID)) {
        mqttClient.publish("HooPDM/from/gateway", clientID);
        mqttClient.subscribe("HooPDM/to/#");
        Serial.println("Connected to MQTT Broker!");
      }
    }

    if (EEPROM.read(0x06) != 'Y' && EEPROM.read(0x10) == '#' && (millis() - connectionTimeoutMillis) > 30000) {
      String _STATION_SSID = "#" + STATION_SSID;
      String _STATION_PASSWORD = "*" + STATION_PASSWORD;
      mesh.sendBroadcast(_STATION_SSID);
      delay(1000);
      mesh.sendBroadcast(_STATION_PASSWORD);
      EEPROM.write(0x06, 'Y');
      EEPROM.commit();
      Serial.println("SSID,Pass Sent Broadcasted!!");
    }
    digitalWrite(LED_BUILTIN, LOW);
  }
  //  Serial.println("END OF ROOT ACT (TimeOut) " + wifiTimeout);
}


//############################ ACTIVITY FOR NORMAL NODES ############################
void mainActivity() {
  while (meshExitTimeout <= 500) {
    mesh.update();
    userScheduler.execute(); // it will run mesh scheduler as well
    myID = mesh.getNodeId();

    if (myIP != getlocalIP()) {
      myIP = getlocalIP();
      Serial.println("My IP is " + myIP.toString());
    }

    int stat = WiFi.status();
    if (stat != WL_CONNECTED && conction_flg == false)
    { delay(50);
      Serial.print(".");
      Serial.println(myID);
      meshExitTimeout++;
    }

    else if (stat == WL_CONNECTED && conction_flg == false)
    { if (millis() - connectionTimeoutMillis > 7000) {
        conction_flg = true;
        meshExitTimeout = 0;
      }
    }

    else if (stat != WL_CONNECTED && conction_flg == true) {
      meshExitTimeout++;
    }

    if (EEPROM.read(0x06) != 'Y' && EEPROM.read(0x10) == '#' && (millis() - connectionTimeoutMillis) > 30000) {
      String _STATION_SSID = "#" + STATION_SSID;
      String _STATION_PASSWORD = "*" + STATION_PASSWORD;
      mesh.sendBroadcast(_STATION_SSID);
      delay(1000);
      mesh.sendBroadcast(_STATION_PASSWORD);
      EEPROM.write(0x06, 'Y');
      EEPROM.commit();
      Serial.println("SSID,Pass Sent Broadcasted!!");
    }
  }
}

//###################################################################################
//################################## MAIN LOOP ######################################
void loop() {
  networks = WiFi.scanNetworks();
  Serial.println(networks);
  networks = networks + 1;
  Serial.println("scan done");
  long wifi_del = 3000;
  int wifiTimeout = 900;
  long start_count = millis();
  int ssid_length = int(EEPROM.read(0x04));
  int s_length = ssid_length;
  noOfTimeout = 0;
  //  Serial.println("SSID Length: " + ssid_length);
  for (int z = 1; z < (ssid_length); z++) {
    STATION_SSID = STATION_SSID + char(EEPROM.read(0x10 + z));
  }
  unsigned long wifiScanTimeout;
  wifiScanTimeout = millis();
  while (1)
  { ssid = WiFi.SSID(i);
    Serial.print(WiFi.SSID(i));
    Serial.print(", ");
    rssi = WiFi.RSSI(i);
    Serial.println(rssi);
    delay(50);
    meshExitTimeout = 0;

    if (ssid == (TAB_SSID) && rssi > minRSSI)
    {
      mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages
      mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6 );
      mesh.onReceive(&receivedCallback);
      mesh.stationManual(TAB_SSID, TAB_PASSWORD);
      mesh.setHostname(HOSTNAME);
      mesh.setRoot(onn);
      mesh.setContainsRoot(onn);
      userScheduler.addTask(taskserial_chk);
      taskserial_chk.enable();
      userScheduler.addTask(taskroutingTable);
      taskroutingTable.enable();
      if (EEPROM.read(0x07) != 'Y') {
        userScheduler.addTask( taskreg );
        taskreg.enable();
      }
      i = 0;
      wifiScanTimeout = 0;
      conction_flg = false;
      lowRssiCount = 0;
      wifiTimeout = 900;
      if (noOfTimeout < 2) {
        rootActivity(wifiTimeout);
      }
      mesh.stop();
      digitalWrite(LED_BUILTIN, HIGH);
      conction_flg == false;
    }

    else if (ssid.startsWith(MESH_PREFIX) && (millis() - wifiScanTimeout) >= 5000 )
    {
      Serial.println("Connecting to Mesh");
      mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages
      mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
      mesh.onReceive(&receivedCallbackNoramlNodes);
      mesh.onNewConnection(&newConnectionCallback);
      mesh.onChangedConnections(&changedConnectionCallback);
      mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
      mesh.onNodeDelayReceived(&delayReceivedCallback);
      mesh.setContainsRoot(onn);
      userScheduler.addTask( taskserial_chk );
      taskserial_chk.enable();
      if (EEPROM.read(0x07) != 'Y') {
        userScheduler.addTask( taskreg );
        taskreg.enable();
      }
      randomSeed(analogRead(A0));
      mesh_flag = true;
      wifiScanTimeout = 0;
      noOfTimeout = 0;
      mainActivity();
      mesh.stop();
      meshExitTimeout = 0;
      conction_flg = false;
    }

    else if (i == networks) {
      i = 0;
      Serial.println("########################");
      networks = WiFi.scanNetworks() + 1;
    }
    else {
      i++;
    }
  }
}
//################################## MAIN LOOP END ##################################
