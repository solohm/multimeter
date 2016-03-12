void setup() {
  Serial.begin(115200);
  Serial.print("\n\n");
  Serial.print(MAIN_NAME);
  Serial.println(" setup");
}

void loop() {
  Serial.print("helloworld\n");
  delay(1000);
}
