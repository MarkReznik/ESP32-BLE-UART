/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE"
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define MAX_MACS  20

int scanTime = 1;  //In seconds
BLEScan *pBLEScan;
int devcnt=0,scan_cnt=0,save_cnt=0;
String devmac_save="";
String devmac_scan="";
uint64_t ui64_scan[MAX_MACS];
uint64_t ui64_temp[MAX_MACS];
uint64_t ui64_save[MAX_MACS];
int8_t i8_rssi[MAX_MACS];


BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue[10];// = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("Received Value: ");
      for (int i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
      }

      Serial.println();
      Serial.println("*********");
      strcpy((char*)txValue,"OK\r\n");
      pTxCharacteristic->setValue(txValue, strlen((char*)txValue));
      //txValue[0]='O';txValue[1]='K';
      //pTxCharacteristic->setValue(txValue, 2);
      pTxCharacteristic->notify();
    }
  }
};
// Callback when a BLE device is detected
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String scn_str=advertisedDevice.getAddress().toString();
        //Serial.println("mac as is: "+scn_str);
        scn_str.replace(":","");
        //Serial.println("mac clean: "+scn_str);
        ui64_scan[scan_cnt]=strtoull(scn_str.c_str(),NULL,16);
        //Serial.println("mac val: "+String(ui64_scan[scan_cnt],HEX));
        i8_rssi[scan_cnt]=advertisedDevice.getRSSI();
        
        if(i8_rssi[scan_cnt]>=-86){
          //devmac_scan+=advertisedDevice.getAddress().toString()+"\r\n";//+" "+String(rssi)+"\r\n";
          Serial.print("+");

          //Serial.printf("Add. Advertised Device: %s %d \n", advertisedDevice.getAddress().toString().c_str(),rssi);
        }else{
          //Serial.printf("Advertised Device: %s %d \n", advertisedDevice.getAddress().toString().c_str(),rssi);
        }
        if(scan_cnt < (MAX_MACS-1)){
          scan_cnt++;
        }
        
    }
};
void setup() {
  Serial.begin(115200);
  
  Serial.println("Scanning...");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(false);  //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
/*
  // Create the BLE Device
  BLEDevice::init("UART Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);

  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
*/

}

void loop() {
  int dev_i,dev_j;
  bool found=false;
  BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);
  pBLEScan->clearResults();  // delete results fromBLEScan buffer to release memory
  delay(1000);
  Serial.println();
  //remove save array mac if missing in scan array
  Serial.println("save_cnt: "+String(save_cnt));
  for(dev_i=0;dev_i<save_cnt;dev_i++){
    Serial.println(String(dev_i+1)+". save: "+String(ui64_save[dev_i],HEX));
    found=false;
    for(dev_j=0;(dev_j<scan_cnt);dev_j++){
      if(ui64_scan[dev_j]==ui64_save[dev_i]){
        found=true;
      }
    }
    if(found==false){
      Serial.println("clear from save");
      ui64_save[dev_i]=0;
      //save_cnt++;
    }
  }
  //add missing mac to save array
  Serial.println("scan_cnt: "+String(scan_cnt));
  for(dev_i=0;dev_i<scan_cnt;dev_i++){
    Serial.println(String(dev_i+1)+". scan: "+String(ui64_scan[dev_i],HEX)+" "+String(i8_rssi[dev_i]));
    found=false;
    for(dev_j=0;(dev_j<save_cnt);dev_j++){
      if(ui64_scan[dev_i]==ui64_save[dev_j]){
        found=true;
      }
    }
    if(found==false){
      Serial.println("add to save");
      ui64_save[save_cnt]=ui64_scan[dev_i];
      if(save_cnt < (MAX_MACS-1)){
        save_cnt++;
      }
      
    }
  }
  scan_cnt=0;
  Serial.println("****************");
  if (deviceConnected) {
    /*
    pTxCharacteristic->setValue(&txValue, 1);
    pTxCharacteristic->notify();
    txValue++;
    delay(10);  // bluetooth stack will go into congestion, if too many packets are sent
    */
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
void func1(void){
  String scan_str,save_str;
  int str_i;
  Serial.println();
  Serial.println("devmac_scan");
  Serial.println(devmac_scan);
  Serial.println("devmac_save");
  Serial.println(devmac_save);
  Serial.println("-----------");
  Serial.println("save remove if not scan");
  str_i=0;
  while(devmac_save.indexOf("\r\n",str_i)>=0){
    save_str=devmac_save.substring(str_i,devmac_save.indexOf("\r\n"));
    Serial.print("str from save: ");Serial.println(save_str);
    if(devmac_scan.indexOf(save_str)<0){
      Serial.print("-del:");Serial.println(save_str);
      devmac_save.remove(devmac_save.indexOf(save_str+"\r\n"),(save_str+"\r\n").length());
    }
    //else{
      str_i+=(save_str+"\r\n").length();
      
    //}
    Serial.print("next save_str index: ");Serial.println(str_i);
    if(str_i>=devmac_save.length()){
        break;
    }
  }
  
  Serial.println("while loop for 'save add if new scan'");
  while(devmac_scan.indexOf("\r\n")>=0){
    if(devmac_save.indexOf("\r\n")<0){
      devmac_save=devmac_scan;
      Serial.print("last scan to save: ");Serial.println(devmac_scan);
      break;
    }
    scan_str=devmac_scan.substring(0,devmac_scan.indexOf("\r\n"));
    Serial.print("str from scan: ");Serial.println(scan_str);
    if(devmac_save.indexOf(scan_str)<0){
      Serial.print("+new:");Serial.println(scan_str);
      devmac_save+=scan_str+"\r\n";
    }
    devmac_scan.remove(0,devmac_scan.indexOf(scan_str+"\r\n")+(scan_str+"\r\n").length());
  }
  Serial.print("new devmac_save:");Serial.println(devmac_save);
  devmac_scan="";
  devcnt=0;
}