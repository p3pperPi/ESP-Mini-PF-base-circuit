// Little FS のファイルアップロードについて
// https://github.com/lorol/arduino-esp32fs-plugin?tab=readme-ov-file
// を参照

#include <LittleFS.h>

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("--------LittleFS test Program-------");
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS マウントに失敗しました。");
    return;
  }

  Serial.println("LittleFS マウント成功。ファイル一覧:");
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.println(file.name());
    file = root.openNextFile();
  }
}

void loop() {
	if(Serial.available()){
		char c = Serial.read();
		if(c == '\n'){
		  Serial.println("ファイル一覧:");
			
			File root = LittleFS.open("/");
		  File file = root.openNextFile();
  		while (file) {
   			Serial.println(file.name());
    		file = root.openNextFile();
  		}
		}
	}
	
	
}
