/*
  Digital Pot Control

  This example controls an Analog Devices AD5206 digital potentiometer.
  The AD5206 has 6 potentiometer channels. Each channel's pins are labeled
  A - connect this to voltage
  W - this is the pot's wiper, which changes when you set it
  B - connect this to ground.

 The AD5206 is SPI-compatible,and to command it, you send two bytes,
 one with the channel number (0 - 5) and one with the resistance value for the
 channel (0 - 255).

 The circuit:
  * All A pins  of AD5206 connected to +5V
  * All B pins of AD5206 connected to ground
  * An LED and a 220-ohm resistor in series connected from each W pin to ground
  * CS - to digital pin 10  (SS pin)
  * SDI - to digital pin 11 (MOSI pin)
  * CLK - to digital pin 13 (SCK pin)

 created 10 Aug 2010
 by Tom Igoe

 Thanks to Heather Dewey-Hagborg for the original tutorial, 2005

*/


// include the SPI library:
#include <SPI.h>
#include <Arduino.h>

#define MSB(x) ((uint8_t)((x >> 8) & 0xFF))
#define LSB(x) ((uint8_t)(x & 0xFF))

// set pin 10 as the slave select for the digital pot:
const int slaveSelectPin = 9;

enum OP_MODE{
  IDLE_MODE,
  CURSOR_MODE,
  SCROLL_MODE
};

enum BT_STATE{
    BT_CONNECTED,
    BT_DISCONNECTED
};

void send_packet(uint8_t *packet, size_t len);
void format_packet(uint16_t x, uint16_t y, uint8_t *packet);

void setup() {
  // set the slaveSelectPin as an output:
  pinMode(slaveSelectPin, OUTPUT);
  // initialize SPI:
  SPI.begin();
  Serial.begin(9600);
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
}

int a = 0;
uint16_t x {0}, y {32000};
uint8_t packet[8];
uint8_t recv_packet[8];
int16_t pitch {0}, roll {0}, yaw {0};
enum BT_STATE state = BT_DISCONNECTED;

char buffer[50];

void loop() {
  delay(98);
  x+=1;
  y-=1;
  format_packet(x, y, CURSOR_MODE, packet);
  transcieve_packets(packet, recv_packet, 8);
  convert_recieved(&roll, &pitch, &yaw, &state, recv_packet);
  
  snprintf(buffer, sizeof(buffer), "State: %u  Pitch: %i.%u  Roll: %i.%u  Yaw: %i.%u", state, pitch/10, abs(pitch%10), roll/10, abs(roll%10), yaw/10, abs(yaw%10));
  Serial.println(buffer);

}

void format_packet(uint16_t x, uint16_t y, OP_MODE mode, uint8_t *packet){
  packet[0] = (uint8_t)mode;
  packet[1] = MSB(x);
  packet[2] = LSB(x);
  packet[3] = 0x67;
  packet[4] = MSB(y);
  packet[5] = LSB(y);
  packet[6] = 0;
  packet[7] = 0;
}

void convert_recieved(int16_t *roll, int16_t *pitch, int16_t *yaw, BT_STATE *state, uint8_t *packet){
  if(packet[3] != 0x68) return;

  *state = (BT_STATE)packet[0];
  *pitch = (packet[1] << 8) + packet[2];
  *roll = (packet[4] << 8) + packet[5];
  *yaw = (packet[6] << 8) + packet[7];
}

void transcieve_packets(uint8_t *packet, uint8_t *recv_packet, size_t len) {
  // take the SS pin low to select the chip:
  digitalWrite(slaveSelectPin, LOW);
  delay(1);
  // send in the address and value via SPI:
  for(int i = 0; i < len; i++){
    recv_packet[i] = SPI.transfer(packet[i]);
  }
  delay(1);
  // take the SS pin high to de-select the chip:
  digitalWrite(slaveSelectPin, HIGH);
}
