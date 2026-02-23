#include <SPI.h>
#include <mcp_can.h>

// Chip select pin for MCP2515 Transmitter
#define CS_CHIP0 10
MCP_CAN CAN0(CS_CHIP0);

// Chip select pin for MCP2515 Receiver
#define CS_CHIP1 9
MCP_CAN CAN1(CS_CHIP1);


float duration, distance;
const int pinTrig = 4;
const int pinEcho = 5;

void setup() {
  // put your setup code here, to run once:
  pinMode(10, OUTPUT);
  pinMode(9, OUTPUT);
  Serial.begin(115200);
  delay(1000);
  
  initUltraSensor(); // Initializing the HC-SR04 sensor
  
  initMCP2515(CAN0); // Initializing the MCP2515 Transmitter
  initMCP2515(CAN1); // Initializing the MCP2515 Receiver
  
  
  
}

void loop() {
  // put your main code here, to run repeatedly:
  distanceSensor(); // get distance from sensor 
  CANtransmission(CAN1); // Transmit data
  CAN_Receive(CAN0); // receive data 

  delay(10);
}

void initUltraSensor(){
  pinMode(pinTrig,OUTPUT);
  pinMode(pinEcho,INPUT);
}

void initMCP2515(MCP_CAN &CANx){
  delay(500);
  // Initialize MCP2515 at 500 kbps with 8 MHz crystal
  if (CANx.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("CAN init ok!");
  } else {
    Serial.println("CAN init fail, check wiring!");
    while (1); // Traps code in infinite loop in case of error
  }
  // Set module to normal mode 
  CANx.setMode(MCP_NORMAL);
  
}

void distanceSensor(){
  //Making sure that pin is low first
  digitalWrite(pinTrig,LOW);
  delayMicroseconds(2);

  //Triggering the ultrasonar wave for a duration of 
  digitalWrite(pinTrig,HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig,LOW);

  // Calculation of distance from measuring the time it takes for sonar pulse to be sent and received
  duration = pulseIn(pinEcho,HIGH);
  distance = (0.0343 *duration)/2;

  // Display the data
  /*
  Serial.print("Distance : ");
  Serial.println(distance);
  Serial.print("\n");
  */
  delay(100);
}

void CANtransmission(MCP_CAN &CANx) {
  // The CAN data frame is 64 bits(8 bytes)
  byte payload[8];
  uint16_t distInt = (uint16_t)(distance*100); // Cast distance from float to int 
  
  if (duration == 0) { // Condition for invalid reading 
    payload[0] = 0xFF;
    payload[1] = 0xFF;
  } else {
    // the uint16_t is an unsigned 16 bits-wide data type so it takes up 2 bytes
    payload[0] = highByte(distInt);
    payload[1] = lowByte(distInt);
  }
  // Sending CAN message with id of 0x101, type frame (=standard frame), 2 bytes wide from payload
  byte status = CANx.sendMsgBuf(0x101, 0,  2, payload);
  
  if (status == CAN_OK) {
    Serial.print("[Sensor] Sent distance (cm): ");
    Serial.println(distance);
  } else {
    Serial.println("[Sensor] Send failed!");
  }
}


void CAN_Receive(MCP_CAN &CANx){
  
  if(CANx.checkReceive() == CAN_MSGAVAIL){
    unsigned long rxId; // receiver ID
    byte len; // length of data
    byte buf[8]; // Buffer of 8 bytes because CAN frames can carry up to 8 bytes of data
    CANx.readMsgBuf(&rxId,&len,buf);

    if(rxId == 0x101 && len >= 2) {
      uint16_t recvDist = word(buf[0], buf[1]); // recombine the bytes to get the value of distance sent
      float distance_rec = float(recvDist)/100; // converting the value back to float
      if(recvDist == 0xFFFF) {
        Serial.println("[Display] Received INVALID reading");
      }
      else{
        Serial.print("[Display] Distance received (cm): ");
        Serial.println(distance_rec);
      }
    }

  }
  else{
    Serial.println("No message available");
  }
}



