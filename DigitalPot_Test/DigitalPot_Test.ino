#include <SPI.h>


const int CS_PIN = 53; // Chip select pin for the MCP4161
const int CS_HZ = 10000000; // Max Clock frequency
const int8_t Write_Data = 0b00000000; // Adress (0000) & Command C1 C1 (00) - Command to write to volatile wiper 0


void setup() {
  
  pinMode(CS_PIN, OUTPUT);
  
  Serial.begin(9600); // Set baud rate to 9600 bps

  SPI.begin();

  const int8_t Position = 116; 

  SetWiper(Position);
}


void loop() {
  // Do nothing
}


void SetWiper(uint8_t Position){

  Serial.print("Recieved command\n");

  digitalWrite(CS_PIN, LOW); // Ensure selection of the device (a redundancy)

  SPI.beginTransaction(SPISettings(CS_HZ, MSBFIRST, SPI_MODE0));
  
  SPI.transfer(Write_Data);
  SPI.transfer(Position);

  SPI.endTransaction();
  SPI.end(); // Leaves pin modes unchanged

  digitalWrite(CS_PIN, HIGH); // Ensure de-selection of the device 

}
