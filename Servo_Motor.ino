#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

// Wifi 세팅

const char* ssid     = "ssid";
const char* password = "password";
const char* host = "host ip";
const uint16_t port = 7770;
ESP8266WiFiMulti WiFiMulti;
WiFiClient client;

// 디바이스, 유닛 정의 

#define UNIT_COUNT 3
bool units_state[UNIT_COUNT] = {false};
const int unit_control_pins[UNIT_COUNT] = {D1, D2, D3};
const int unit_control_pins[UNIT_COUNT] = {D5, D6, D7};
String device_id = "device_id_SW1";
String device_type = "switch";
String raspberry_id = "rasp_id_1"; //test
String raspberry_group = "rasp_group_1"; //test
volatile bool is_interrupted_0 = false;
volatile bool is_interrupted_1 = false;
volatile bool is_interrupted_2 = false;

Servo servo1;
Servo servo2;
Servo servo3;

int angle = 0;

bool write_func(String data); //TCP 서버 요청 보냄
String get_token(String data, int index); //토큰을 가져옴
void toggle_unit_state(int n);
ICACHE_RAM_ATTR void isr_button_0(); // isr_button_0 : button 0에 대한 인터럽트 서비스 루틴
ICACHE_RAM_ATTR void isr_button_1(); // isr_button_1 : button 1에 대한 인터럽트 서비스 루틴
ICACHE_RAM_ATTR void isr_button_2(); // isr_button_2 : button 2에 대한 인터럽트 서비스 루틴


void setup() {
  Serial.begin(115200);
   // Unit과 관련된 설정 - Pin mode 등 -
  int i = 0;
  for (i = 0; i < UNIT_COUNT; i++) {
    pinMode(unit_control_pins[i], OUTPUT);
  }
  for (int i = 0; i < UNIT_COUNT; i++) {
    pinMode(unit_toggle_button_pins[i], INPUT_PULLUP);
  }

  // Unit과 관련된 설정 - Interrupt
  attachInterrupt(digitalPinToInterrupt(unit_toggle_button_pins[0]), isr_button_0, FALLING);
  attachInterrupt(digitalPinToInterrupt(unit_toggle_button_pins[1]), isr_button_1, FALLING);
  attachInterrupt(digitalPinToInterrupt(unit_toggle_button_pins[2]), isr_button_2, FALLING);
  
  
  // 와이파이 설정 및 연결 시작
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(500);

  // TCP 서버와의 connnection을 생성함
  Serial.print("connecting to ");
  Serial.print(host);
  Serial.print(':');
  Serial.println(port);
  while(!client.connect(host, port)) {
    Serial.println("connection failed");
    Serial.println("wait 5 sec...");
    delay(5000);
  }

  // TCP 서버에 현재 디바이스에 대한 정보를 등록함
  String register_data = "OK;6;4;2;0;0;" + device_id + ";" + device_type + ";\n";
  if (write_func(register_data)) {
    String read_data = client.readStringUntil('\n');
    Serial.print("received : ");
    Serial.println(read_data);
    Serial.println("Registerd new device");  
  } else {
    Serial.println("Failed to register new device");
  }
}

void switch_on(int unitnum){
  if(unitnum == D5){
    servo1.attach(unitnum);
    for(angle = 0; angle < 45; angle++){
     servo1.write(angle);
     delay(15);
    }
  }
  else if(unitnum == D6){
    servo2.attach(unitnum);
    for(angle = 0; angle < 45; angle++){
      servo2.write(angle);
      delay(15);
    }
  }
  else if(unitnum == D7){
    servo3.attach(unitnum);
    for(angle = 0; angle < 45; angle++){
      servo3.write(angle);
      delay(15);
    }
  }
}

void switch_off(int unitnum){
  if(unitnum == D5){
    servo1.attach(unitnum);
    for(angle = 45; angle > 0; angle--){
      servo1.write(angle);
      delay(15);
    }
  }
  else if(unitnum == D6){
    servo2.attach(unitnum);
    for(angle = 45; angle > 0; angle--){
      servo2.write(angle);
      delay(15);
    }
  }
  else if(unitnum == D7){
    servo3.attach(unitnum);
    for(angle = 45; angle > 0; angle--){
      servo3.write(angle);
      delay(15);
    }
  }
}

void loop() {
  String read_data = client.readStringUntil('\n');
  if (read_data.length() > 0) {
    Serial.print("received : ");
    Serial.print(read_data);
    String unit_index = get_token(read_data, 4);
    bool on_off = get_token(read_data, 5) == "1";
    if (unit_index == "0") {
      units_state[0] = on_off;
      Serial.print("[0] ON_OFF : ");
      Serial.println(on_off);
    }
    else if (unit_index == "1") {
      units_state[1] = on_off;
      Serial.print("[1] ON_OFF : ");
      Serial.println(on_off);
    }
    else if (unit_index == "2") {
      units_state[2] = on_off;
      Serial.print("[2] ON_OFF : ");
      Serial.println(on_off);
    }
    else {
      // 만약 unit_index가 허용 범위 외 라면
      // req_err_data 패킷을 remote server로 전송함
      String req_err_data = "NO;6;4;2;0;0;" + device_id + ";" + device_type + ";\n";
      write_func(req_err_data);
      Serial.println("req err");
      return;
    }
    String req_ok_data = "OK;6;4;2;0;0;" + device_id + ";" + device_type + ";\n";
    write_func(req_ok_data);
    String req_data_for_raspberry = "OK;8;4;3;" + get_token(read_data, 4) + ";" + get_token(read_data, 5) + ";" + get_token(read_data, 6) + ";" + get_token(read_data, 7) + ";" + get_token(read_data, 8) + ";" + get_token(read_data, 9) + ";\n";
    write_func(req_data_for_raspberry);
    Serial.print(req_data_for_raspberry + " & " + req_ok_data + " : ");
    Serial.println("req ok");
  }
  else {
    if (is_interrupted_0) {
      is_interrupted_0 = false;
      toggle_unit_state(0);
      Serial.println("Button 0 Clicked");
    } 
    else if (is_interrupted_1) {
      is_interrupted_1 = false;
      toggle_unit_state(1);
      Serial.println("Button 1 Clicked");
    }
    else if (is_interrupted_2) {
      is_interrupted_2 = false;
      toggle_unit_state(2);
      Serial.println("Button 2 Clicked");
    }
  }

  for(int i = 0; i< UNIT_COUNT; i++){
    if (units_state[i]) switch_on(unit_control_pins[i]);
    else switch_off(unit_control_pins[i]);
  }
}
  /*********************************************/
/* write_func(String) : TCP 서버 요청 보냄    */
/* Return Type : bool -> 요청 성공 여부       */
/*********************************************/
bool write_func(String data) {
  client.print(data);
  String read_data = client.readStringUntil('\n');
  Serial.println(read_data);
  
  String req_state = get_token(read_data, 0);
  String client_type = get_token(read_data, 2);
  return req_state == "OK" && client_type == "0";
}

/*********************************************/
/* get_token(String, index) : 토큰을 가져옴   */
/* Return Type : String -> 패킷을 분리한 토큰  */
/*********************************************/
String get_token(String data, int index) {
  int i = 0;
  int semicolon_index = 0;
  String data_copy = data;
  String ret = "";
  
  for (i = 0; i < index + 1; i++) {
    semicolon_index = data_copy.indexOf(";");
    if (semicolon_index != -1) {
      ret = data_copy.substring(0, semicolon_index);
      data_copy = data_copy.substring(semicolon_index + 1);
    } else {
      return data_copy;
    }
  }
  return ret;
}

/*********************************************/
/* toggle_unit_state(int) : unit state를 변화 */
/* Return Type : void                        */
/* Args - INT : unit index                   */
/*                                           */
/* n번째 Unit에 대한 상태를 Toggle하고          */
/* 동기화를 위해 라즈베리파이에 신호를 전송함      */
/*********************************************/
void toggle_unit_state(int n) { 
  // n의 허용 범위 밖일 경우, 동작을 수행하지 않고 종료.
  if (n > 1 or n < 0) return;
  String req_data_for_raspberry = "OK;8;4;3;";

  units_state[n] = !units_state[n];
  if (units_state[n]) {
    if (n == 0) {
      req_data_for_raspberry += "0;1;" + device_id + ";" + device_type + ";" + raspberry_id + ";" + raspberry_group + ";`REQ`\n"; 
    } 
    else if (n == 1) {
      req_data_for_raspberry += "1;1;" + device_id + ";" + device_type + ";" + raspberry_id + ";" + raspberry_group + ";`REQ`\n"; 
    }
  } 
  else {
    if (n == 0) {
      req_data_for_raspberry += "0;0;" + device_id + ";" + device_type + ";" + raspberry_id + ";" + raspberry_group + ";`REQ`\n"; 
    } 
    else if (n == 1) {
      req_data_for_raspberry += "1;0;" + device_id + ";" + device_type + ";" + raspberry_id + ";" + raspberry_group + ";`REQ`\n"; 
    }
  }
  
  if (write_func(req_data_for_raspberry)) {
    Serial.print(req_data_for_raspberry + " : ");
    Serial.println("req ok"); 
  } else {
    // 동기화에 실패할 경우, Toggle 이전 상태로 만듦.
    units_state[n] = !units_state[n];
    Serial.println("request failed");
  }
}

/*********************************************/
/* isr_button_0 : Button 0에 대한 ISR         */
/*********************************************/
ICACHE_RAM_ATTR void isr_button_0() {
  is_interrupted_0 = true;
}

/*********************************************/
/* isr_button_1 : Button 1에 대한 ISR         */
/*********************************************/
ICACHE_RAM_ATTR void isr_button_1() {
  is_interrupted_1 = true;
}

/*********************************************/
/* isr_button_2 : Button 2에 대한 ISR         */
/*********************************************/
ICACHE_RAM_ATTR void isr_button_2() {
  is_interrupted_2 = true;
}
