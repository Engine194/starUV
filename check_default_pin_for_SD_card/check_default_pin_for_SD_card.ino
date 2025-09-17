#include <SPI.h>
#include <SD.h>

void setup() {
  Serial.begin(115200);
  // SPI.begin(sclk, miso, mosi, new_cs_pin);
  Serial.print("MOSI: ");
  Serial.println(MOSI);
  Serial.print("MISO: ");
  Serial.println(MISO);
  Serial.print("SCK: ");
  Serial.println(SCK);
  Serial.print("SS: ");
  Serial.println(SS);


  Serial.println(SD.begin());

  if (!SD.begin()) {
    Serial.println("SD Card loaded failed !");
    Serial.println("Retry again...");
    delay(1200);
    Serial.println(SD.begin());
    return;
  }
  Serial.println("SD Card loaded...");  

}

void loop() {
  // put your main code here, to run repeatedly:

}
