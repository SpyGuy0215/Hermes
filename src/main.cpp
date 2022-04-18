#include <Arduino.h>

#define BLYNK_TEMPLATE_ID "TMPLsg0JJ61n"
#define BLYNK_DEVICE_NAME "Arduino Uno Wifi Rev2"

char auth[] = "Fy6_JdaKUWOw0-tEfBKQW3OMz2IsYmsY";

char ssid[] = "ArcherA20_2.4";
char pass[] = "jupiter30alien!";


#include <SPI.h>
#include <Pixy2.h>
#include <PIDLoop.h>
#include <WiFiNINA.h>
#include <BlynkSimpleWiFiNINA.h>
#include <Arduino_LSM6DS3.h>
#include <utility/wifi_drv.h>

Pixy2 pixy; 
PIDLoop panLoop(400, 0, 400, true);
PIDLoop tiltLoop(500, 0, 500, true);

unsigned long lastFind; 


const int rightSpeed = 9;
const int rightForward = 8;
const int rightBackward = 7;

const int leftSpeed = 5;
const int leftForward = 4;
const int leftBackward = 6;

int panFromCenter, tiltFromCenter = 0; 
int oldPan, oldTilt = 0; 

int32_t pan = 500;
int32_t tilt = 500;

int base_width = 170;
int base_height = 158;

int cloth_width, cloth_height;

int mode = 0;  // 0 = manual(app), 1 = auto(follow)
int manual_X = 0;
int manual_Y = 0;

int direction = 0; //0 = forwards, 1 = backwards


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(leftSpeed, OUTPUT);
  pinMode(leftForward, OUTPUT);
  pinMode(leftBackward, OUTPUT);
  
  pinMode(rightSpeed, OUTPUT);
  pinMode(rightForward, OUTPUT);
  pinMode(rightBackward, OUTPUT);

  WiFiDrv::pinMode(27, OUTPUT); //BLUE
  WiFiDrv::pinMode(26, OUTPUT); // GREEN
  WiFiDrv::pinMode(25, OUTPUT); // RED

  Serial.begin(9600);

  WiFiDrv::analogWrite(25, 255);
  WiFiDrv::analogWrite(26, 100);

  Serial.println("Starting Blynk connection...");
  Serial.print("On WiFi network: ");
  Serial.println(ssid);

  Blynk.begin(auth, ssid, pass);

  Serial.println("Connected to Blynk!");

  WiFiDrv::analogWrite(25, 0);
  WiFiDrv::analogWrite(26, 0);
  WiFiDrv::digitalWrite(27, HIGH);

  pixy.init();

  Serial.println("Starting program!");

  digitalWrite(LED_BUILTIN, HIGH);

  pixy.setServos(500, 500);

  Blynk.run();
  Blynk.virtualWrite(V6, pan);
  Blynk.virtualWrite(V7, tilt);



}

void pan_tilt(int32_t &pan, int32_t &tilt){
  pixy.ccc.getBlocks();
  int32_t panOffset, tiltOffset;

  if(pixy.ccc.numBlocks){

    cloth_width = pixy.ccc.blocks[0].m_width;
    cloth_height = pixy.ccc.blocks[0].m_height;

    panOffset = (int32_t)pixy.frameWidth/2 - (int32_t)pixy.ccc.blocks[0].m_x;
    tiltOffset = (int32_t)pixy.ccc.blocks[0].m_y - (int32_t)pixy.frameHeight/2;  

    panLoop.update(panOffset);
    tiltLoop.update(tiltOffset);
    pan = panLoop.m_command;
    tilt = tiltLoop.m_command;
    pixy.setServos(panLoop.m_command, tiltLoop.m_command);
    lastFind = millis();

    // Blynk.virtualWrite(V9, 255);
  }

  else{
    if(millis() - lastFind > 500){
      panLoop.reset();
      tiltLoop.reset();
      pixy.setServos(500, 500);
    }

    Blynk.virtualWrite(V9, 0);
  }

  // Blynk.virtualWrite(V6, pan);
  // Blynk.virtualWrite(V7, tilt);

  // Blynk.virtualWrite(V3, cloth_width);
  // Blynk.virtualWrite(V4, cloth_height);

}

void auto_move(int pan, int tilt){

  pixy.ccc.getBlocks();

  if(pixy.ccc.numBlocks){ // if there is a block

  // Pan good zone: 450-550
    if(cloth_width + 20 < base_width && cloth_height + 20 < base_height ){  // need to go forward
      analogWrite(leftSpeed, 180);
      digitalWrite(leftForward, HIGH);
      digitalWrite(leftBackward, LOW);

      analogWrite(rightSpeed, 180);
      digitalWrite(rightForward, HIGH);
      digitalWrite(rightBackward, LOW);
    }

    else if(cloth_width - 20 > base_width && cloth_height - 20 > base_height ){  // need to go backward
      analogWrite(leftSpeed, 180);
      digitalWrite(leftForward, LOW);
      digitalWrite(leftBackward, HIGH);

      analogWrite(rightSpeed, 180);
      digitalWrite(rightForward, LOW);
      digitalWrite(rightBackward, HIGH);
    } 

    else if(pan<450){  // need to turn right
      int speed = 180;

      analogWrite(leftSpeed, speed);
      digitalWrite(leftForward, HIGH);
      digitalWrite(leftBackward, LOW);

      analogWrite(rightSpeed, speed);
      digitalWrite(rightForward, LOW);
      digitalWrite(rightBackward, HIGH);
    }
    
    else if(pan>550){  // need to turn left
      int speed = 180;


      analogWrite(leftSpeed, speed);
      digitalWrite(leftForward, LOW);
      digitalWrite(leftBackward, HIGH);

      analogWrite(rightSpeed, speed);
      digitalWrite(rightForward, HIGH);
      digitalWrite(rightBackward, LOW);
    }

    else{
      analogWrite(leftSpeed, 0);
      digitalWrite(leftForward, LOW);
      digitalWrite(leftBackward, LOW);

      analogWrite(rightSpeed, 0);
      digitalWrite(rightForward, LOW);
      digitalWrite(rightBackward, LOW);
    }
  }

  else{  // if there is no block
    digitalWrite(leftSpeed, 0);
    digitalWrite(leftForward, LOW);
    digitalWrite(leftBackward, LOW); 
    digitalWrite(rightSpeed, 0);
    digitalWrite(rightForward, LOW);
    digitalWrite(rightBackward, LOW);
  }

  // Blynk.virtualWrite(V6, pan);
  // Blynk.virtualWrite(V7, tilt);
  if(millis() % 250 == 0){
    digitalWrite(leftSpeed, 0);
    digitalWrite(leftForward, LOW);
    digitalWrite(leftBackward, LOW); 
    digitalWrite(rightSpeed, 0);
    digitalWrite(rightForward, LOW);
    digitalWrite(rightBackward, LOW); 
  }
  
}

void manual_move(int left_motor, int right_motor){

  analogWrite(leftSpeed, left_motor);
  digitalWrite(leftForward, direction== 0?HIGH:LOW);
  digitalWrite(leftBackward, direction== 1?HIGH:LOW);

  analogWrite(rightSpeed, right_motor);
  digitalWrite(rightForward,direction== 0?HIGH:LOW);
  digitalWrite(rightBackward, direction== 1?HIGH:LOW);
}

void loop() {
  Blynk.run();

  if(mode == 0){
    manual_move(manual_X, manual_Y);
    pixy.setServos(pan, tilt);
    pixy.ccc.getBlocks();
  }

  else if(mode == 1){
    pan_tilt(pan, tilt);
    auto_move(pan, tilt);
 }

 if(millis() % 1000 == 0){
    Blynk.virtualWrite(V1, mode);
    Blynk.virtualWrite(V2, pan);
    Blynk.virtualWrite(V3, tilt);
    Blynk.virtualWrite(V4, cloth_width);
    Blynk.virtualWrite(V6, panLoop.m_command);
    Blynk.virtualWrite(V7, tiltLoop.m_command);
  }
  // int32_t pan, tilt = 0; 
  // pan_tilt(pan, tilt); 
  // move(pan, tilt); 
}

BLYNK_WRITE(V0){  //X coords of joystick
  manual_X = param.asInt();
}

BLYNK_WRITE(V1){  // Y coords of joystick
  manual_Y = param.asInt();
}

BLYNK_WRITE(V2){  // mode
  mode = param.asInt();
  
  if(mode == 1){
    WiFiDrv::analogWrite(26, 255);
    WiFiDrv::digitalWrite(27, LOW);
  }
  else if(mode == 0){
    WiFiDrv::analogWrite(26, 0);
    WiFiDrv::digitalWrite(27, HIGH);
  }
}

BLYNK_WRITE(V6){  //Pan
  pan = param.asInt();  
}

BLYNK_WRITE(V7){  //Tilt
  tilt = param.asInt();
}

BLYNK_WRITE(V8){
  int lights = param.asInt();

  pixy.setLamp(lights, 0);
}

BLYNK_WRITE(V10){
  direction = param.asInt();
}
// void forward(int speed){
//   if(speed < 0){
//     speed = 0;
//   }
//   if(speed>255){
//     speed = 255;
//   }

//   digitalWrite(b_dir1_pin, HIGH);
//   digitalWrite(b_dir2_pin, LOW);
//   analogWrite(b_pwm_pin, speed);

//   digitalWrite(a_dir1_pin, HIGH);
//   digitalWrite(a_dir2_pin, LOW);
//   analogWrite(a_pwm_pin, speed);
// }

/* VALUES

Optimal Tilt: 250
Optimal Pan: 500

Pan & Tilt min/max = 0/1000

*/