﻿#include <Servo.h>
#include <DistanceSRF04.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
/*************************************************************************************************/
/****                 程序说明                                                                ****/
/*************************************************************************************************/
/*
 * 这是一块arduino板加上一块四路电机的L293D板子做的遥控小车程序，采用路由器的TTL连接arduino的TTL口。
 * 通过WIFI和手机连接来实现控制，可以扩展灯光，喇叭，还有舵机。
 * 功能的开关：不存在的功能直接注释掉就可以了，如雷达，在雷达配置节点：
 *将  #define HaveRadar 变为 //#define HaveRadar
 *将 #define RadarPorts {15,14} 变为 //#define RadarPorts {15,14}
 *就能关闭雷达功能
 */
/*************************************************************************************************/
/****                 基础设置（串口，电机相关）                                              ****/
/*************************************************************************************************/
//串口速度
#define SerialSpeed 115200
//电机口位置
#define MotorPort {14,15,16,17} //具体电机顺序请自行测试调整，单数，1，3位为一侧，2，4位为一侧
//马达速度
#define MotorSpeed {0,0}//left,right 左侧，右侧
#define MotorSpeedPorts{3,5}//left,right 左侧，右侧
//马达方向
#define MotorDirection {LOW,LOW,LOW,LOW}   //四个马达的电平状态，对应电机顺序

/*************************************************************************************************/
/****                 雷达设置                                                                ****/
/*************************************************************************************************/
//是否有雷达
//#define HaveRadar
//雷达端口
#define RadarPorts {9,8}

/*************************************************************************************************/
/****                 灯光设置                                                                ****/
/*************************************************************************************************/
//#define HaveLight
#define LightPort 2 //灯光端口

/*************************************************************************************************/
/****                 喇叭设置                                                                ****/
/*************************************************************************************************/
//#define HaveHorn
#define HornPort 12 //喇叭端口

/*************************************************************************************************/
/****                 舵机设置                                                                ****/
/*************************************************************************************************/
//#define HaveServo
#define ServoStartTest
#define ServoPorts {9,10} //x,y
#define ServoCenterAngle {98,88} //两个舵机的居中角度。//x,y
#define ServoMaxAngle {170,126} //两个舵机的最大角度。//x,y
#define ServoMinAngle {10,10} //两个舵机的最小角度。//x,y
#define ServoStepAngle {4,4} //两个舵机的每步角度。//x,y


/*************************************************************************************************/
/*
 * ------------------------------------------------------------------------------------------------
 * ------------------------------------------------------------------------------------------------
 * 配置节点结束，以下部分为代码
 */
/*************************************************************************************************/
int8_t _motors[4] = MotorPort;
int8_t _motorSpeed[2] = MotorSpeed;
int8_t _motorDirection[4] = MotorDirection;
int8_t _motorSpeedPorts[2] = MotorSpeedPorts;


#if defined(HaveRadar)
int8_t _radarPorts[2] = RadarPorts;
//是否开启雷达
byte radar = false;
//雷达
DistanceSRF04 Dist;
int distance;
#endif

#if defined(HaveLight)
byte light = false;
#endif


#if defined(HaveServo)
int8_t  _servoPorts[2] = ServoPorts;//两个舵机的端口X,Y
int8_t _servoCenterAngle[2] = ServoCenterAngle;//两个舵机的居中角度X,Y
int8_t _servoMaxAngle[2] = ServoMaxAngle; //两个舵机的最大角度
int8_t _servoMinAngle[2] = ServoMinAngle; //两个舵机的最小角度
int8_t _servoStepAngle[2] = ServoStepAngle;//两个舵机每转动一次的角度递增值

//两个舵机的当前角度，用于回传，也不用再读取计算，节约资源
byte servoXPoint = 0;
byte servoYPoint = 0;
Servo servoX;
Servo servoY;
#endif




byte serialIn = 0;
byte commandAvailable = false;
String strReceived = "";


///声明部分结束，注意，很多用的byte，如果大于255的还是用int



void setup()
{
#if defined(HaveRadar)
  Dist.begin(_radarPorts[0], _radarPorts[1]); //雷达信号口
#endif

#if defined(HaveServo)
  servoX.attach(_servoPorts[0]);//1号舵机信号口
  servoY.attach(_servoPorts[1]);//2号舵机信号口
#if defined(ServoStartTest)
  servo_test();//舵机测试
#endif
#endif
  pinMode(_motors[0], OUTPUT);
  pinMode(_motors[1], OUTPUT);
  pinMode(_motors[2], OUTPUT);
  pinMode(_motors[3], OUTPUT);

  pinMode(LightPort, OUTPUT);
  pinMode(HornPort, OUTPUT);


  Serial.begin(SerialSpeed);

}


void loop()
{
#if defined(HaveRadar)
  if (radar == true)
  {
    distance = Dist.getDistanceCentimeter();
    if (distance <= 5 & distance > 1) {
      hou();
      delay(100);
      ting();
      distance = 0;
    }
  }
#endif
  getSerialLine();
  if (commandAvailable) {
    processCommand(strReceived);
    strReceived = "";
    commandAvailable = false;
  }


}

void getSerialLine()
{
  //使用\r字符作为两条命令间隔符，拼接收到的字符
  while (serialIn != '\r')
  {
    if (!(Serial.available() > 0))
    {
      return;
    }

    serialIn = Serial.read();
    if (serialIn != '\r') {
      //对于某些语言可能无法单独输出\r 忽略后续的\n字符符
      if (serialIn != '\n') {
        char a = char(serialIn);
        strReceived += a;
      }

    }
  }
  //读取到分割符，重新启动拼接
  if (serialIn == '\r') {
    commandAvailable = true;
    serialIn = 0;
  }
}

///
//命令处理过程
//建议所有的命令识别都在这里完成。每个动作独立方法，不要在里面写操作过程，这样看起来比较清晰。
///
void processCommand(String input)
{
  String command = getValue(input, ' ', 0);
  byte iscommand = true;
  int val;
  if (command == "MD_Qian")
  {
    qian();
  }
  else if (command == "MD_Hou")
  {
    hou();
  }
  else if (command == "MD_Zuo")
  {
    zuo();
  }
  else if (command == "MD_You")
  {
    you();
  }
  else if (command == "MD_Ting")
  {
    ting();
  }
  else if (command == "MD_SD")
  {
    val = getValue(input, ' ', 1).toInt();
    _motorSpeed[0] = val;
    val = getValue(input, ' ', 2).toInt();
    _motorSpeed[1] = val;
  }
#if defined(HaveServo)
  else if (command == "DJ_CS")
  {
    servo_test();
  }
  else if (command == "DJ_Shang")
  {
    servo_up();
  }
  else if (command == "DJ_Xia")
  {
    servo_down();
  }
  else if (command == "DJ_Zuo")
  {
    servo_left();
  }
  else if (command == "DJ_You")
  {
    servo_right();
  }
  else if (command == "DJ_Zhong")
  {
    servo_center();
  }
  else if (command == "DJ_CZ_JD")//VerticalRotation
  {
    val = getValue(input, ' ', 1).toInt();
  }
  else if (command == "DJ_SP_JD")//Horizontal rotation
  {
    val = getValue(input, ' ', 1).toInt();
  }
#endif
#if defined(HaveLight)
  else if (command == "LED_Status")
  {
    val = getValue(input, ' ', 1).toInt();
    light = val == 0 ? false : true;
    SetLight(light);
  }
  else if (command == "LED_Status_Swich")
  {
    light = light ? false : true;
    SetLight(light);
  }
#endif
#if defined(HaveRadar)
  else if (command == "Radar_Status")
  {
    val = getValue(input, ' ', 1).toInt();
    radar = val ? true : false;
  }
  else if (command == "Radar_Status_Swich")
  {
    radar = radar ? false : true;
  }
#endif
#if defined(HaveHorn)
  else if (command == "Horn_Start") {
    SetHorn(true);
  }
  else if (command == "Horn_Stop") {
    SetHorn(false);
  }
#endif
  else
  {
    iscommand = false;
  }
  //是否收到的是已经定义的命令，如果不是则不回送状态，免得浪费带宽
  if (iscommand) {
    SendMessage("cmd:" + input);
    SendStatus();
  }

}
//命令参数获取方法，支持一个命令多个参数，采用空格符作为分割符号，注意是半角空格符，不是制表符也不是全角空格，可或者可以自行定义，并修改，懒改了。
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {
    0, -1
  };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
#if defined(HaveServo)
#if defined(ServoStartTest)
//舵机动作部分，字面意思你懂的。。。
void servo_test(void) {
  int nowcornerY = servoY.read();
  int nowcornerX = servoX.read();
  servo_Vertical(_servoMinAngle[1]);
  delay(500);
  servo_Vertical(_servoMaxAngle[1]);
  delay(500);
  servo_Vertical(_servoCenterAngle[1]);
  delay(500);
  servo_Horizontal(_servoMinAngle[0]);
  delay(500);
  servo_Horizontal(_servoMaxAngle[0]);
  delay(500);
  servo_Horizontal(_servoCenterAngle[0]);
  delay(500);
  servo_center();
}
#endif
void servo_right(void)
{
  int servotemp = servoX.read();
  servotemp -= _servoStepAngle[0];
  servo_Horizontal(servotemp);
}
void servo_left(void)
{
  int servotemp = servoX.read();
  servotemp += _servoStepAngle[0];
  servo_Horizontal(servotemp);
}
void servo_down(void)
{
  int servotemp = servoY.read();
  servotemp += _servoStepAngle[1];
  servo_Vertical(servotemp);
}
void servo_up(void)
{
  int servotemp = servoY.read();
  servotemp -= _servoStepAngle[1];
  servo_Vertical(servotemp);
}
void servo_center(void)
{
  servo_Vertical(_servoCenterAngle[1]);
  servo_Horizontal(_servoCenterAngle[0]);
}
void servo_Vertical(int corner)
{
  int cornerY = servoY.read();
  if (cornerY > corner) {
    for (int i = cornerY; i > corner; i = i - _servoStepAngle[1]) {
      servoY.write(i);
      servoYPoint = i;
      delay(50);
    }
  }
  else {
    for (int i = cornerY; i < corner; i = i + _servoStepAngle[1]) {
      servoY.write(i);
      servoYPoint = i;
      delay(50);
    }
  }
  servoY.write(corner);
  servoYPoint = corner;
}
void servo_Horizontal(int corner)
{
  int i = 0;
  byte cornerX = servoX.read();
  if (cornerX > corner) {
    for (i = cornerX; i > corner; i = i - _servoStepAngle[0]) {
      servoX.write(i);
      servoXPoint = i;
      delay(50);
    }
  }
  else {
    for (i = cornerX; i < corner; i = i + _servoStepAngle[0]) {
      servoX.write(i);
      servoXPoint = i;
      delay(50);
    }
  }
  servoX.write(corner);
  servoXPoint = corner;
}
#endif

//电机方向动作部分，中文拼音大声念出来就是意思了。。
void qian(void)
{
  _motorDirection[0] = LOW;
  _motorDirection[1] = HIGH;
  _motorDirection[2] = LOW;
  _motorDirection[3] = HIGH;
  SetEN();
}
void hou(void)
{
  _motorDirection[0] = HIGH;
  _motorDirection[1] = LOW;
  _motorDirection[2] = HIGH;
  _motorDirection[3] = LOW;
  SetEN();
}
void you(void)
{
  _motorDirection[0] = LOW;
  _motorDirection[1] = HIGH;
  _motorDirection[2] = HIGH;
  _motorDirection[3] = LOW;
  SetEN();
}
void zuo(void)
{
  _motorDirection[0] = HIGH;
  _motorDirection[1] = LOW;
  _motorDirection[2] = LOW;
  _motorDirection[3] = HIGH;
  SetEN();
}
void ting(void)
{
  _motorSpeed[0] = 0;
  _motorSpeed[1] = 0;
  _motorDirection[0] = LOW;
  _motorDirection[1] = LOW;
  _motorDirection[2] = LOW;
  _motorDirection[3] = LOW;
  SetEN();
}
//设置两侧速度
void SetEN() {
  analogWrite(_motorSpeedPorts[0], _motorSpeed[0]);
  analogWrite(_motorSpeedPorts[1], _motorSpeed[1]);
  digitalWrite(_motors[0], _motorDirection[0]);
  digitalWrite(_motors[1], _motorDirection[1]);
  digitalWrite(_motors[2], _motorDirection[2]);
  digitalWrite(_motors[3], _motorDirection[3]);
}
#if defined(HaveLight)
//灯光开关
void SetLight(bool status) {
  digitalWrite(LightPort, status);
}
#endif
#if defined(HaveHorn)
void SetHorn(bool status) {
  if (status)
    digitalWrite(HornPort, HIGH);//发声音
  else
    digitalWrite(HornPort, LOW);
}
#endif

//拼接状态
void SendStatus() {

  String out = "";
  out += _motorDirection[0];
  out += ",";
  out += _motorDirection[1];
  out += ",";
  out += _motorDirection[2];
  out += ",";
  out += _motorDirection[3];
  out += ",";
  out += _motorSpeed[0];
  out += ",";
  out += _motorSpeed[1];
  out += ",";
#if defined(HaveServo)
  out += servoXPoint;
  out += ",";
  out += servoYPoint;
#else
  out += "0,0";
#endif
  out += ",";
#if defined(HavaRadar)
  out += radar;
#else
  out += "0";
#endif
  out += ",";
#if defined(HaveLight)
  out += light;
#else
  out += "0";
#endif
  SendMessage(out);
}
//串口消息发送
void SendMessage(String data) {
  Serial.println(data);
}
