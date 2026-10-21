#define BLYNK_TEMPLATE_ID "TMPL67dnpNQ8b"
#define BLYNK_TEMPLATE_NAME "DOOR"
#define BLYNK_AUTH_TOKEN "F9N4WsndPxNhOZWmpVzIgYaRHiM8ga02"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Stepper.h>

const int rainSensorPin = 15;  // chân digital từ cảm biến mưa
const int gasSensorPin = 17;
const int ledPin2 = 2;          // chân điều khiển LED
const int ledPin4 = 4;            // LED vật lý
const int ledPin13 = 13;
const int buttonPin = 16;        // nút bấm vật lý
const int AutoPin = 23; 
const int OpenPin = 18;
const int ClosePin = 19;
const int bPin = 21;
const int BuzzerPin = 22;
const int ledPin32 = 32;
const int CheckPin = 33;

const int motorPin1 = 14;
const int motorPin2 = 27;
const int motorPin3 = 26;
const int motorPin4 = 25;

const int stepsPerRevolution = 2048;
const int StepClose = 3072;
const int StepOpen = -3072;

char ssid[] = "Iphone 17 pro max";
char pass[] = "tri12345";

Stepper myStepper(stepsPerRevolution, motorPin1, motorPin3, motorPin2, motorPin4);

bool ledState = false;
bool ledAutoState = false;
bool buzzerState = false;
bool gasState = false;
bool gasOverride = false;
bool rainState = false;
bool wifiConnected = false;
bool lastButtonState = HIGH;
bool lastAutoState = HIGH;
bool lastOpenState = HIGH;
bool lastCloseState = HIGH;
bool lastBState = HIGH;
bool motorDirection;
unsigned long StepPlus;
unsigned long holdStepCount = 0;
unsigned long RainCount = 0;
unsigned long lastBuzzerToggle = 0;
const unsigned long buzzerInterval = 2000;


bool buttonPressed = false;  // Trạng thái nút đang được nhấn
unsigned long buttonPressTime = 0;  // Thời gian bắt đầu nhấn nút
const unsigned long holdThreshold = 500;  // Thời gian nhấn giữ (ms)

WidgetTerminal terminal(V0);

void notifyToTerminal(String message) {
  terminal.println(message);
  terminal.flush();  // bắt buộc để hiển thị lên app
}

void checkButton() {
  bool buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    // Nhấn mới xảy ra (đổi trạng thái LED)
    ledState = !ledState;

    digitalWrite(ledPin4, ledState);

    if (wifiConnected) {
      Blynk.virtualWrite(V1, ledState);  // Đồng bộ với nút Blynk
    }

    delay(100); // Chống dội nút đơn giản
  }
  lastButtonState = buttonState;
}


void Open(){
  while (holdStepCount != 3072){
    myStepper.step(-1);
    holdStepCount++; 
    delay(2);
    if (wifiConnected) {
      Blynk.run(); // giữ kết nối ổn định
    }
  }
  if (wifiConnected && holdStepCount == 3072) {
    notifyToTerminal("Completely Opened!");
  }
}

void Close(){
  while(motorDirection){
    myStepper.step(1);
    delay(2);
    motorDirection = digitalRead(CheckPin);
    if (wifiConnected && !motorDirection) {
      notifyToTerminal("Completely Closed!");
    }
  }
  holdStepCount = 0;
}

void ButtonOpen(){
  bool OpenState = digitalRead(OpenPin);

  while (OpenState == LOW && holdStepCount != StepClose) {  // Quay từng bước nhỏ theo chiều thuận
    myStepper.step(-1);
    holdStepCount++; 
    delay(2);
    OpenState = digitalRead(OpenPin);

    if (wifiConnected && holdStepCount == 3072) {
        notifyToTerminal("Completely Opened!");
      }
  }
}

void ButtonClose() {
  bool CloseState = digitalRead(ClosePin);

  while (CloseState == LOW ) {  // Quay từng bước nhỏ theo chiều thuận
    if (motorDirection) {
      myStepper.step(1);  // Quay thuận chiều 180 độ
      holdStepCount--;
      delay(2);
      CloseState = digitalRead(ClosePin);
      motorDirection = digitalRead(CheckPin);
    } 
    if (!motorDirection) {
      holdStepCount = 0;
      CloseState = HIGH;
      if (wifiConnected) {
        notifyToTerminal("Completely Closed!");
      }
    }
  }
}

void Button(){
  bool BState = digitalRead(bPin);
  if (BState == LOW && lastBState == HIGH) {
    if (holdStepCount != 0 && motorDirection != 0){
      for (int i = 0; i < holdStepCount; i++) {
        myStepper.step(1);

        if (wifiConnected) {
          Blynk.run(); // giữ kết nối ổn định
        }
        delay(2);    // tránh trễ và ngắt
      }
      holdStepCount = 0;
    }
    else{
      if (motorDirection) {
        Close();
      } else {
        Open();
      }
    }
  }
}

void ButtonAuto(){
  bool AutoState = digitalRead(AutoPin);
  if (AutoState == LOW && lastAutoState == HIGH){
    ledAutoState = !ledAutoState;
    digitalWrite(ledPin32, ledAutoState);

    if (wifiConnected) {
      Blynk.virtualWrite(V4, ledAutoState);
    }
    delay(100);
  }
  lastAutoState = AutoState;
}

void  performHoming() {
  motorDirection = digitalRead(CheckPin);
  if(motorDirection){
    while(motorDirection){
      myStepper.step(1);
      delay(2);
      motorDirection = digitalRead(CheckPin);
    }
  }
}

void checkRain(bool rainState, bool ledAutoState) {
  bool currentGasState = digitalRead(gasSensorPin);
  if (currentGasState == LOW) {
    // Nếu có gas, ngừng xử lý mưa và tắt LED mưa
    digitalWrite(ledPin2, LOW);
    digitalWrite(BuzzerPin, LOW);
    buzzerState = false;
    return; // Thoát ngay lập tức
  }

  if (rainState == LOW) {  // LOW = có mưa (thường cảm biến kéo xuống mức thấp khi có mưa)
    digitalWrite(ledPin2, HIGH);   // bật LED

    if (wifiConnected) {
      Blynk.virtualWrite(V6, 255);
      notifyToTerminal("Rain detected!");
    }

    

    unsigned long currentTime = millis();
    if (currentTime - lastBuzzerToggle >= buzzerInterval) {
    lastBuzzerToggle = currentTime;
    buzzerState = !buzzerState; // đảo trạng thái
    digitalWrite(BuzzerPin, buzzerState ? HIGH : LOW);
    }
    
    if(ledAutoState && !gasOverride){
      if (motorDirection != 0) {
        Close();
      }
      else if (holdStepCount > 3072 && motorDirection != 0) {
        StepPlus = 4,294,967,295 - holdStepCount;
        myStepper.step(StepPlus);
      }
      if (wifiConnected && motorDirection == 0) {
        notifyToTerminal("Completely Closed!");
      }
      holdStepCount = 0;
    }
  } else {
    digitalWrite(ledPin2, LOW);  
    digitalWrite(BuzzerPin, LOW);
    buzzerState = false; 

    if (wifiConnected) {
      Blynk.virtualWrite(V6, 0); 
    }
  }
}


void checkGas(bool gasState) {
  if (gasState == LOW) {
    digitalWrite(ledPin2, LOW); 
    digitalWrite(ledPin13, HIGH);

    if (wifiConnected) {
      Blynk.virtualWrite(V5, 255);
      notifyToTerminal("Gas detected!");
      Blynk.logEvent("gas_alert", "Phát hiện khí gas!");
    }

    unsigned long currentTime = millis();
    if (currentTime - lastBuzzerToggle >= buzzerInterval) {
      buzzerState = !buzzerState;
      digitalWrite(BuzzerPin, buzzerState);
      lastBuzzerToggle = currentTime;
    }

    if (holdStepCount == 3072) {
      return;
    }

    if(!motorDirection || holdStepCount == 0) {
      Open();
    } else if (holdStepCount > 0 && holdStepCount < 768) {
      Open();
    } else return;
    gasOverride = true;

    if (wifiConnected && holdStepCount == 3072) {
        notifyToTerminal("Completely Opened!");
      }
    
  } else {
    digitalWrite(ledPin13, LOW);
    digitalWrite(BuzzerPin, LOW);
    buzzerState = false;
    gasOverride = false;

    if (wifiConnected) {
      Blynk.virtualWrite(V5, 0);
    }
  }
}

BLYNK_WRITE(V1) { 
  int value = param.asInt();
  ledState = value;
  digitalWrite(ledPin4, ledState);
}

BLYNK_WRITE(V2) { // Mở cửa
  int value = param.asInt();
  if (value == 1) {
    myStepper.step(-(3072 - holdStepCount));
    holdStepCount = 3072;
  }
}

BLYNK_WRITE(V3) { // Đóng cửa
  int value = param.asInt();
  if (value == 1) {  // Quay từng bước nhỏ theo chiều thuận
    myStepper.step(holdStepCount);  // Quay thuận chiều 180 độ
    holdStepCount = 0;
  }
}

BLYNK_WRITE(V4) { // Bật/tắt chế độ tự động
  int value = param.asInt();
  ledAutoState = value;
  digitalWrite(ledPin32, ledAutoState);
}

void setup() {
  pinMode(rainSensorPin, INPUT);  // chân cảm biến là ngõ vào
  pinMode(gasSensorPin, INPUT);
  pinMode(ledPin2, OUTPUT);        // chân LED là ngõ ra
  pinMode(ledPin4, OUTPUT);
  pinMode(ledPin13, OUTPUT);
  pinMode(ledPin32, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(OpenPin, INPUT_PULLUP);
  pinMode(ClosePin, INPUT_PULLUP);
  pinMode(bPin, INPUT_PULLUP);
  pinMode(AutoPin, INPUT_PULLUP);
  pinMode(BuzzerPin, OUTPUT); 
  pinMode(CheckPin, INPUT);
  Serial.begin(115200);           // mở serial để debug

  myStepper.setSpeed(15);
  digitalWrite(BuzzerPin, LOW);

  WiFi.begin(ssid, pass);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(100);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nĐã kết nối Wi-Fi");
    wifiConnected = true;
    Blynk.config(BLYNK_AUTH_TOKEN);
    if (Blynk.connect(5000)) {
      Serial.println("Đã kết nối Blynk server");
    } else {
      Serial.println("Không thể kết nối Blynk server. Chạy chế độ OFFLINE.");
      wifiConnected = false;
    }
  } else {
    Serial.println("\nKhông thể kết nối Wi-Fi. Chế độ OFFLINE.");
    wifiConnected = false;
  }

  performHoming();
}

void loop() {

  
  bool gasState = digitalRead(gasSensorPin);
  bool rainState = digitalRead(rainSensorPin); 
  bool gasDetected = (gasState == LOW);
  motorDirection = digitalRead(CheckPin);
  
  checkGas(gasState);
  
  if (!gasDetected) {
    checkButton();
    if (!ledState == true) {
      ButtonOpen();
      ButtonClose();
    } else {
      Button();
    }
    ButtonAuto();
    
    checkRain(rainState, ledAutoState);
  }

  if (wifiConnected) {
    Blynk.run();
  }

  yield();
}
