#include <Arduino.h>
#include <painlessMesh.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
//#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <EEPROM.h>
//3265753360
bool  onn = true;

#define   MESH_PREFIX     "HooPDMNCRA"
#define   MESH_PASSWORD   "Hoo@WWwireless"
#define   MESH_PORT       5555
//#define   STATION_SSID     "ptcl"
//#define   STATION_PASSWORD "12345678"
#define   TAB_SSID     "hoocontrols"
#define   TAB_PASSWORD "hoopdm123"
#define HOSTNAME "MQTT_Bridge"

#define LED_relay2   13
#define TouchInput2  12
boolean ledState2 = false;
boolean lastButtonState2 = false;
boolean buttonState2 = false;
unsigned long lastDebounceTime2 = 0;

boolean buttonActive = false; // indicates if the button is active/pressed
unsigned long debounceDelay = 1;    // debounce time
unsigned long longPress_10 = 20000;

unsigned long mqtt_connect;
bool serial_flag = false;
bool mesh_flag = false;
bool w_flag = true;
bool valIsSame = true;
bool conction_flg = false;
long minRSSI = -75;
long rssi;
int networks;
String  header;
int ssid_len;
int pass_addr;
int       i;
long connectedRSSI;
int lowRssiCount = 0;
String inputString = "";
String    gang = "1G";
String    myID;
String    msg;
String    ssid;
String   STATION_SSID;
String   STATION_PASSWORD;
char inChar [290];
String backTrackString;

unsigned long snd_ssid_millis;
bool calc_delay = false;
int meshExit = 0;
// Prototypes
void sendMessage();
//void Temp_Hum();
void reg();
void receivedCallback2(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);
void receivedCallback( const uint32_t &from, const String &msg );
void mqttCallback(char* topic, byte* payload, unsigned int length);
void touch();

WiFiServer server(80);
IPAddress getlocalIP();
Scheduler userScheduler; // to control your personal task
IPAddress myIP(0, 0, 0, 0);
//IPAddress mqttBroker2(210, 2, 139, 183);
IPAddress mqttBroker2(192, 168, 0, 150);
//Adafruit_HTU21DF htu = Adafruit_HTU21DF();
//WiFiManager wifiManager;
SimpleList<uint32_t> nodes;
painlessMesh  mesh;
WiFiClient wifiClient;
PubSubClient mqttClient(mqttBroker2, 1883, mqttCallback, wifiClient);

Task tasktouch( TASK_MILLISECOND * 5, TASK_FOREVER, &touch ); // start with a one second interval
//Task taskserial_chk( TASK_MILLISECOND * 1, TASK_FOREVER, &serial_chk ); // start with a one second interval
Task taskreg( TASK_SECOND * 2, TASK_FOREVER, &reg ); // start with a one second interval
//Task taskSendMessage( TASK_MINUTE * 3, TASK_FOREVER, &sendMessage ); // start with a one second interval

void setup() {
  //  Serial.begin(9600);
  EEPROM.begin(512);
  EEPROM.write(0x07, 'Y');
  EEPROM.commit();
  pinMode(LED_relay2, OUTPUT);
  digitalWrite(LED_relay2, LOW);
  ledState2 = LOW;
  pinMode(TouchInput2, INPUT);
  snd_ssid_millis = millis();
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
  if (msg == "ON2") {
    digitalWrite(LED_relay2, HIGH);
    ledState2 = HIGH;
    sendACK(msg);
  }

  else if (msg == "OF2") {
    digitalWrite(LED_relay2, LOW);
    ledState2 = LOW;
    sendACK(msg);
  }

  else if (msg == "AllON") {
    digitalWrite(LED_relay2, HIGH);
    ledState2 = HIGH;
    sendACK(msg);
  }

  else if (msg == "AllOFF") {
    digitalWrite(LED_relay2, LOW);
    ledState2 = LOW;
    sendACK(msg);
  }

  else if (msg == "RESET") {
    for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    ESP.restart();
  }

  else if (msg == "Registred") {
    taskreg.disable();
    EEPROM.write(0x07, 'Y');
    EEPROM.commit();
  }

  else {
    Serial.println(":::::ELSE Condition::::");
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
      delay(1000);
      ESP.restart();
    }

    else if (comp == "$") {
      mqttClient.publish("HooPDM/from/gatewayReg", msg.c_str());
    }

    else if (comp == "@") {
      mqttClient.publish("HooPDMbackTrack", msg.c_str());
    }
  }
}


void receivedCallback2(uint32_t from, String & msg) {
  Serial.printf("==>>2 Received from %u msg=%s\n", from, msg.c_str());
  if (msg == "ON2") {
    digitalWrite(LED_relay2, HIGH);
    ledState2 = HIGH;
    sendACK(msg);
  }

  else if (msg == "OF2") {
    digitalWrite(LED_relay2, LOW);
    ledState2 = LOW;
    sendACK(msg);
  }

  else if (msg == "AllON") {
    digitalWrite(LED_relay2, HIGH);
    ledState2 = HIGH;
    sendACK(msg);
  }

  else if (msg == "AllOFF") {
    digitalWrite(LED_relay2, LOW);
    ledState2 = LOW;
    sendACK(msg);
  }

  else if (msg == "RESET") {
    for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    ESP.restart();
  }

  else if (msg == "Registred") {
    EEPROM.write(0x07, true);
  }

  else {
    Serial.println(":::::ELSE Condition::::");
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
      delay(1000);
      ESP.restart();
    }

    else if (comp == "$") {
      mqttClient.publish("HooPDM/from/gatewayReg", msg.c_str());
    }

    else if (comp == "@") {
      mqttClient.publish("HooPDMbackTrack", msg.c_str());
    }
  }
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  Serial.println("MQTT Received");
  Serial.println(topic);
  char* cleanPayload = (char*)malloc(length + 1);
  payload[length] = '\0';
  memcpy(cleanPayload, payload, length + 1);
  msg = String(cleanPayload);
  free(cleanPayload);

  String targetStr = String(topic).substring(10);

  if (targetStr == "gateway")
  {
    Serial.println(msg);
    if (msg == "getNodes")
    {
      uint32_t list;
      mqttClient.publish("HooPDM/from/gateway", mesh.subConnectionJson().c_str());
      Serial.println(mesh.subConnectionJson().c_str());
    }
    //    loop();
  }

  else if (targetStr == "broadcast")
  {
    Serial.println("In Broadcast");
    mesh.sendBroadcast(msg);
    Serial.println(msg);
    if (msg == "ON2") {
      digitalWrite(LED_relay2, HIGH);
      ledState2 = HIGH;
      sendACK(msg);
    }

    else if (msg == "OF2") {
      digitalWrite(LED_relay2, LOW);
      ledState2 = LOW;
      sendACK(msg);
    }

    else if (msg == "AllON") {
      digitalWrite(LED_relay2, HIGH);
      ledState2 = HIGH;
      sendACK(msg);
    }

    else if (msg == "AllOFF") {
      digitalWrite(LED_relay2, LOW);
      ledState2 = LOW;
      sendACK(msg);
    }

    else if (msg == "RESET") {
      for (int i = 0 ; i < EEPROM.length() ; i++) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      mesh.sendBroadcast(msg);
      ESP.restart();
    }

    //    loop();
  }

  else if (targetStr == myID)
  { //mesh.sendBroadcast(msg);
    if (msg == "ON2") {
      digitalWrite(LED_relay2, HIGH);
      ledState2 = HIGH;
      sendACK(msg);
    }

    else if (msg == "OF2") {
      digitalWrite(LED_relay2, LOW);
      ledState2 = LOW;
      sendACK(msg);
    }

    else if (msg == "AllON") {
      digitalWrite(LED_relay2, HIGH);
      ledState2 = HIGH;
      sendACK(msg);
    }

    else if (msg == "AllOFF") {
      digitalWrite(LED_relay2, LOW);
      ledState2 = LOW;
      sendACK(msg);
    }

    /////FOR SONOFF/////
    else if (msg == "ON") {
      //    digitalWrite(LED_relay, LOW);
      //    digitalWrite(LED_PIN, LOW);
      String bTrack = "@" + myID + "," + "T1" + ",0";
      Serial.println(bTrack);
      sendBackTrack(bTrack);
    }
    /////FOR SONOFF/////
    else if (msg == "OFF") {
      //    digitalWrite(LED_relay, HIGH);
      //    digitalWrite(LED_PIN, HIGH);
      String bTrack = "@" + myID + "," + "T1" + ",0";
      Serial.println(bTrack);
      sendBackTrack(bTrack);
    }

    else if (msg == "RESET") {
      for (int i = 0 ; i < EEPROM.length() ; i++) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      ESP.restart();
    }

    else if (msg == "Registred") {
      taskreg.disable();
      EEPROM.write(0x07, 'Y');
      EEPROM.commit();
    }
    //    loop();
  }


  else if (targetStr == "Router_SSID") {
    Serial.println("Writing SSID to EEPROM");
    String new_ssid = msg;
    ssid_len = new_ssid.length();
    EEPROM.write(0x04, ssid_len);
    for (int s = 0; s < ssid_len; s++) {
      EEPROM.write(0x10 + s, new_ssid[s]);
    }
    EEPROM.commit();
  }

  else if (targetStr == "Router_Pass") {
    Serial.println("Writing Password to EEPROM");
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
    }
    //    loop();
  }
}


IPAddress getlocalIP() {
  return IPAddress(mesh.getStationIP());
}

//////////*****************************************/////////
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

void sendACK(String ACKmsg) {
  String msgACK =  "&" + myID + "," + gang + ACKmsg + "OK";
  if (mesh_flag) {
    mesh.sendBroadcast(msgACK);
  }
  else
    mqttClient.publish("HooPDMbackTrack", msgACK.c_str());
}

void sendBackTrack(String backTrack) {
  String trackMsg =  "@" + myID + "," + gang + "," + backTrack;
  if (mesh_flag) {
    mesh.sendBroadcast(trackMsg);
  }
  else
    mqttClient.publish("HooPDMbackTrack", trackMsg.c_str());
}

void touch() {
  pinMode(TouchInput2, INPUT);

  int reading2 = digitalRead(TouchInput2);
  if (reading2 != lastButtonState2) {
    lastDebounceTime2 = millis();
  }
  if ((millis() - lastDebounceTime2) > debounceDelay && (millis() - lastDebounceTime2) < longPress_10 && reading2 != buttonState2) {
    buttonState2 = reading2;
    if (buttonState2 == LOW) {
      ledState2 = !ledState2;
      digitalWrite(LED_relay2, ledState2);
      if (ledState2 == LOW) {
        String bTrack = "00000000";
        sendBackTrack(bTrack);
      }
      else if (ledState2 == HIGH) {
        String bTrack = "10000000";
        sendBackTrack(bTrack);
      }
    }
  }
  lastButtonState2 = reading2;
}


void rootActivity(int wifiTimeout) {
  while (lowRssiCount <= 5) {
    touch();
    mesh.update();
    userScheduler.execute(); // it will run mesh scheduler as well
    mqttClient.loop();
    myID = mesh.getNodeId();

    if (myIP != getlocalIP()) {
      myIP = getlocalIP();
      mqtt_connect = millis();
      Serial.println("My IP is " + myIP.toString());
      const char *clientID = myID.c_str();
      touch();
      if (mqttClient.connect(clientID)) {
        mqttClient.publish("HooPDM/from/gateway", clientID);
        mqttClient.subscribe("HooPDM/to/#");
        Serial.println("Connected to MQTT Broker!");
      }
    }
    touch();
    int stat = WiFi.status();
    long connectedRSSI = WiFi.RSSI();
    if (stat != WL_CONNECTED && conction_flg == false)
    { delay(50);
      Serial.print(".");
      Serial.println(myID);
      wifiTimeout--;
      if (wifiTimeout == 0) {
        mesh.stop();
        lowRssiCount = 6;
        //        ESP.restart();
      }
    }

    else if (stat == WL_CONNECTED && conction_flg == false)
    { if (millis() - snd_ssid_millis > 7000) {
        conction_flg = true;
      }
    }

    else if (connectedRSSI < minRSSI) {
      lowRssiCount = lowRssiCount++;
    }

    else if (stat != WL_CONNECTED && conction_flg == true && mesh_flag == false)
    {
      mesh.stop();
      lowRssiCount = 6;
    }

    if (mqttClient.connected() == false && (millis() - mqtt_connect) >= 30000) {
      //      Serial.println("Not connected to MQTT Broker!");
      const char *clientID = myID.c_str();
      mqtt_connect = millis();
      touch();
      if (mqttClient.connect(clientID)) {
        mqttClient.publish("HooPDM/from/gateway", clientID);
        mqttClient.subscribe("HooPDM/to/#");
        Serial.println("Connected to MQTT Broker!");
      }
    }

    if (EEPROM.read(0x06) != 'Y' && EEPROM.read(0x10) == '#' && (millis() - snd_ssid_millis) > 30000) {
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
  lowRssiCount = 0;
  wifiTimeout = 600;
  mesh.stop();
  digitalWrite(LED_BUILTIN, HIGH);
}


void mainActivity() {
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
  }
  else if (stat == WL_CONNECTED && conction_flg == false)
  { if (millis() - snd_ssid_millis > 7000) {
      conction_flg = true;
    }
  }

  else if (stat != WL_CONNECTED && conction_flg == true)
  {
    if (meshExit > 1000) {
      mesh.stop();
      //      ESP.restart();
    }
    else
      meshExit++;
  }


  if (EEPROM.read(0x06) != 'Y' && EEPROM.read(0x10) == '#' && (millis() - snd_ssid_millis) > 30000) {
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
//##################################################################
//##################################################################
void loop() {
  networks = WiFi.scanNetworks();
  Serial.println(networks);
  networks = networks + 1;
  Serial.println("scan done");
  long wifi_del = 3000;
  int wifiTimeout = 600;
  long start_count = millis();
  int ssid_length = int(EEPROM.read(0x04));
  int s_length = ssid_length;
  //  Serial.println("SSID Length: " + ssid_length);
  for (int z = 1; z < (ssid_length); z++) {
    STATION_SSID = STATION_SSID + char(EEPROM.read(0x10 + z));
  }
  unsigned long wifiScanTimeout;
  wifiScanTimeout = millis();
  while (1)
  { touch();
    ssid = WiFi.SSID(i);
    Serial.print(WiFi.SSID(i));
    Serial.print(", ");
    rssi = WiFi.RSSI(i);
    Serial.println(rssi);
    delay(50);

    if (ssid == (TAB_SSID) && rssi > minRSSI)
    {
      mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages
      mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6 );
      mesh.onReceive(&receivedCallback);
      mesh.stationManual(TAB_SSID, TAB_PASSWORD);
      mesh.setHostname(HOSTNAME);
      mesh.setRoot(onn);
      mesh.setContainsRoot(onn);
      //            userScheduler.addTask( taskSendMessage );
      //            taskSendMessage.enable();
      userScheduler.addTask( tasktouch );
      tasktouch.enable();
      if (EEPROM.read(0x07) != 'Y') {
        userScheduler.addTask( taskreg );
        taskreg.enable();
      }
      i = 0;
      wifiScanTimeout = 0;
      rootActivity(wifiTimeout);
    }

    else if (ssid.startsWith(MESH_PREFIX) && (millis() - wifiScanTimeout) >= 5000 )
    {
      Serial.println("Connecting to Mesh");
      mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages
      mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
      mesh.onReceive(&receivedCallback2);
      mesh.onNewConnection(&newConnectionCallback);
      mesh.onChangedConnections(&changedConnectionCallback);
      mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
      mesh.onNodeDelayReceived(&delayReceivedCallback);
      mesh.setContainsRoot(onn);
      userScheduler.addTask( tasktouch );
      tasktouch.enable();
      if (EEPROM.read(0x07) != 'Y') {
        userScheduler.addTask( taskreg );
        taskreg.enable();
      }

      randomSeed(analogRead(A0));
      mesh_flag = true;
      wifiScanTimeout = 0;
      while (meshExit <= 999) {
        mainActivity();
      }
      meshExit = 0;
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
