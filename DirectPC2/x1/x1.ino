#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#include <Wire.h>
#include <Adafruit_MCP4725.h>

#include <Servo.h>
#include <TEA5767.h>

#define fullFrameSize 21


#define r_slope 400

#define defaultFreq 1700
#define f0 500
#define f1 800
#define f2 1100
#define f3 1400


#define N 4

// Type of Frame
#define IFrame 7
#define UFrame 4
#define SFrame 6

// Mode
#define INIT_MODE 0
#define SPLIT_AXIS 1
#define PIXEL_MODE 2
#define COMPLETE 99


// Dac
double S[N];
uint16_t S_DAC[N];
Adafruit_MCP4725 dac;
TEA5767 radio = TEA5767();

// VAR
bool canSend = true;
bool intialize = true;
bool startTimer = false;
bool receivedAck = false;
bool boolFrame = false;
bool boolAck = true;

char type = 'O';
String cmd = "";
int mode = 0;
int sf = 0;
unsigned int frameNo = 0, ackNo = 0;
uint32_t data = 0x0;

int delay0, delay1, delay2, delay3;
unsigned long long currentTime = 0, startTime = 0, timeOut = 20000;
unsigned long long sendTime = 0, sendTimeOut = 10000;
uint32_t outFrame = 0;
uint16_t inFrame = 0;
unsigned int baud_count = 0;
int inFrameType = 0;


int prev = 0;
int count = 0;

int baud_check = 0;

int bit_check = -1;

bool check_amp = false;
bool check_baud = false;

const float frequency = 89.6; //Enter your own Frequency // old wave = 89.7

uint32_t baud_begin = 0;

String pictureStore[3];
int pictureIndex = 0;
int pixelIndex = 0;
int quadIndex = 0;

String code = "";

int dataArray12Bit[12]; // เอาไว้เก็บ data ให้เต็ม 12 Bits
String quadrant1[5]; // ค่า grayscale ของ quadrant 1 ที่อ่านได้จาก Camera ส่งมาที่ PC2
String quadrant2[5]; // ค่า grayscale ของ quadrant 2 ที่อ่านได้จาก Camera ส่งมาที่ PC2
String quadrant3[5]; // ค่า grayscale ของ quadrant 3 ที่อ่านได้จาก Camera ส่งมาที่ PC2
String quadrant4[5]; // ค่า grayscale ของ quadrant 4 ที่อ่านได้จาก Camera ส่งมาที่ PC2
String X[5]; // เส้น X ที่ใช้สำหรับการตีเส้นแนว X
String Y[5]; // เส้น Y ที่ใช้สำหรับการตีเส้นแนว Y 

String gBit = "";
//Servo setup

int x = 0;
Servo myservo;
int num = 40;

//End
void setup() {
  // 
  Serial.begin(115200);
  dac.begin(0x64);
  Wire.begin();
  
  // ค่าที่เอาไว้ให้สำหรับ dac
  for (int i = 0; i < N; i++) 
  {
    S[i] = sin(2 * PI * (i / (double)N));
    S_DAC[i] = 4095 / 2.0 * (1 + S[i]);
  }
  // เซ็ตค่าเริ่มต้นใน array
  for (int i = 0; i < 12; i++) dataArray12Bit[i] = 0;
  
  
  // กำหนดค่า delay สำหรับใช้ในการส่งข้อมูลแบบ FSK
  delay0 = (1000000 / f0 - 1000000 / defaultFreq) / N;
  delay1 = (1000000 / f1 - 1000000 / defaultFreq) / N;
  delay2 = (1000000 / f2 - 1000000 / defaultFreq) / N;
  delay3 = (1000000 / f3 - 1000000 / defaultFreq) / N;

  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);
  pinMode(A0, INPUT);

  // เช็ทค่า frequency ที่ต้องการให้ radio รับ
  radio.set_frequency(frequency);
  
  // Servo init
  myservo.attach(9); // กำหนดขา 9 ควบคุม Servo
  if (x < num) {
    while (x <= num) {
      myservo.write(x);
      x++;
      if (x == num) {
        myservo.write(x);
      }
      delay(10);
    }
  }
  else if (x > num) {
    while (x >= num) {
      myservo.write(x);
      x--;
      if (x == num) {
        myservo.write(x);
      }
      delay(10);
    }
  }
  // Servo end

  Serial.flush();
}


void clearDataArray12Bit()
{
  for (int i = 0; i < 12; i++)
  {
    dataArray12Bit[i] = 0;
  }
}

// Reset ค่าทุกอย่างเป็นค่าเริ่มต้น
void flushData()
{
  canSend = true;
  intialize = true;
  startTimer = false;
  receivedAck = false;
  boolFrame = false;
  boolAck = true;

  type = 'O';
  cmd = "";
  mode = 0;
  sf = 0;
  frameNo = 0;
  ackNo = 0;
  data = 0x0;
  currentTime = 0;
  startTime = 0;
  sendTime = 0;
  sendTimeOut = 10000;
  outFrame = 0;
  inFrame = 0;
  baud_count = 0;
  inFrameType = 0;
  clearDataArray12Bit();
  x = 0; num = 40;
  code = "";
  gBit = "";
  for (int i = 0; i < 5;i++)quadrant1[i] = "";  

for (int i = 0; i < 5;i++)quadrant2[i] = ""; 

for (int i = 0; i < 5;i++)quadrant3[i] = ""; 

for (int i = 0; i < 5;i++)quadrant4[i] = ""; 

for (int i = 0; i < 5;i++)X[i] = ""; 

for (int i = 0; i < 5;i++)Y[i] = ""; 


  prev = 0;
  count = 0;

  baud_check = 0;

  bit_check = -1;

  check_amp = false;
  check_baud = false;

  baud_begin = 0;

  for (int i = 0;i<3;i++)
  {
    pictureStore[i]="";  
  }
  pictureIndex = 0;
  pixelIndex = 0;
  quadIndex = 0;

  String code = "";
}

/* เช็คว่าเฟรมที่รับมามี Error หรือไม่ จากนั้นเช็คชนิดของเฟรมว่าเป็นเฟรมอะไร */
void checkFrame()
{
  if (checkError(inFrame) == false) // No error
  {
    int inFrameType = inFrame >> 10; // 
    if (inFrameType == UFrame)
    {      
      int cmd = (inFrame >> 5) & B11111;

      /*
       * ตอบกลับด้วย U-Frame 
       * data = 0x1000 หมายถึงตอบรับเมื่อได้รับคำสั่งว่า ต้องการเริ่มต้นการเชื่อมต่อ
       * data = 0x1800 หมายถึงตอบรับเมื่อได้รับคำสั่งว่า ต้องการเริ่มต้นการเชื่อมต่อใหม่ทั้งหมด
       * data = 0x1c00 หมายถึงตอบรับเมื่อได้รับคำสั่งว่า ต้องการสิ้นสุดการเชื่อมต่อทั้งหมด
      
      */
      
      if (cmd == B00110) // ถ้าได้รับคำสั่งให้เชื่อมต่อ
      {
        type = 'U';
        data = 0x1000; 
        sendFrame(true, false);
        Serial.println("Waiting for PC1 command ...");
      }
      else if (cmd == B00001) // ถ้าได้รับคำสั่งให้เริ่มทำงานใหม่ตั้งแต่ต้น
      {
        type = 'U';
        data = 0x1800;
        sendFrame(true, false);
        Serial.println(" ---- Restart ---- ");
        flushData();
      }
      else if (cmd == B00111) // ถ้าได้รับคำสั่งที่ให้สิ้นสุดการทำงาน
      { 
        type = 'U';
        data = 0x1c00;
        sendFrame(true, false);
        Serial.println("---- End ----");
        Serial.print(3); // ส่งไปบอกส่วน Camera ให้หยุดการทำงาน
      }
    }
    
    else if (inFrameType == SFrame) //ถ้าเฟรมที่ได้รับคือ S-Frame
    {
      uint8_t tmp = (inFrame >> 6) & B1111;
      if (tmp == 0x9) // ถ้า unpack เฟรมออกมาแล้วได้ค่า 0x9 คือแสดงว่าได้รับ ack ที่ถูกต้อง
      {
        Serial.println("Received ack.");
        startTimer = false;
        boolFrame = !boolFrame;
        canSend = true; // ให้เริ่มส่งเฟรมต่อไปได้
        if (mode == SPLIT_AXIS) prepareCapture(); // ถ้าโหมดเป็น SPLIT_AXIS ให้เริ่มการถ่ายภาพ
        else if (mode == PIXEL_MODE and pixelIndex < 5) prepareAnalysis(); // ถ้่่าเป็น PIXEL_MODE และยังได้รับค่าที่ส่งมาจากกล้องยังไม่ครบ ให้ส่งข้อมูลกลับไปเรื่อยๆ
        else if (mode == COMPLETE) Serial.println("Terminated."); // ถ้าเป็น COMPLETE ให้จบการทำงาน
      }
    }
    else if (inFrameType == IFrame) //ถ้าเฟรมที่ได้รับคือ I-Frame
    {
      int D = (inFrame >> 6) & B1111; // แกะออกมาดูว่า PC1 ส่งข้อมูลอะไรมา
      if (D == B1111 and mode == INIT_MODE) captureAll(); // ถ้าโหมดเป็น INIT_MODE และได้รับข้อมูล 1111 ให้เริ่มทำการถ่ายภาพ 3 มุม
      else if (mode == PIXEL_MODE) analysis(cmd); // ถ้าโหมดเป็น PIXEL_MODE ให้เริ่มวิเคราะห์ภาพ ตามค่ารหัสที่ PC 1 ส่งมา
      sendFrame(false,false); // ส่ง ack กลับว่าได้รับแล้ว
    }
  }
  else
  {
    Serial.println("Error detected in data"); 
  }
}

void analysis(int inCode)
{
  mode = PIXEL_MODE;
  code = String(inCode, BIN);
  addZeroTo4Bit(&code);
  Serial.println(" - - Pixel mode - - ");
  //  prepareAnalysis();
  Serial.print(code); // วิเคราะห์ข้อมูลของรหัสภาพ (ส่งไปที่ python)

}

void prepareAnalysis() // วนส่งข้อมูล x,y, grayscale ของแต่ละ quadrant ไปที่ PC1 เมื่อได้รับ ack ตอบกลับ
{
  data = 0;
  if (quadIndex == 0)
  {
    sendPixelToPc1(quadrant1, pixelIndex);
  }
  else if (quadIndex == 1)
  {
    sendPixelToPc1(quadrant2, pixelIndex);
  }
  else if (quadIndex == 2)
  {
    sendPixelToPc1(quadrant3, pixelIndex);
  }
  else if (quadIndex == 3)
  {
    sendPixelToPc1(quadrant4, pixelIndex);
  }
  else if (quadIndex == 4) // X
  {
    sendPixelToPc1(X, pixelIndex);
  }
  else if (quadIndex == 5) // Y
  {
    sendPixelToPc1(Y, pixelIndex);
  }

  pixelIndex++;

  if (pixelIndex == 5)
  {
    if (quadIndex == 4)
    {
      Serial.println(" - - Sended X Line - - ");

    }
    else if (quadIndex == 5)
    {
      Serial.println(" - - Sended Y Line - - ");
    }
    else
    {
      Serial.print(" - - Sended all ");
      Serial.print(quadIndex + 1);
      Serial.println(" quadrant data - - ");
    }

    quadIndex++;
    pixelIndex = 0;
  }
}

int strToInt(String colorCode) // Format รูปแบบ
{
  int tempData = 0;

  if (colorCode == "0000") tempData = 0;

  else if (colorCode == "0001") tempData = 1;

  else if (colorCode == "0010") tempData = 2;

  else if (colorCode == "0011") tempData = 3;

  else if (colorCode == "0100") tempData = 4;

  else if (colorCode == "0101") tempData = 5;

  else if (colorCode == "0110") tempData = 6;

  else if (colorCode == "0111") tempData = 7;

  else if (colorCode == "1000") tempData = 8;

  else if (colorCode == "1001") tempData = 9;

  else if (colorCode == "1010") tempData = 10;

  else if (colorCode == "1011") tempData = 11;

  else if (colorCode == "1100") tempData = 12;

  else if (colorCode == "1101") tempData = 13;

  else if (colorCode == "1110") tempData = 14;

  else if (colorCode == "1111") tempData = 15;

  return tempData;

}


void sendPixelToPc1(String arr[], int index) 
{

  int i = index;
  String tempStr = strTo4Bit(arr[i]);
  for (int j = 0; j < tempStr.length(); j++)
  {
    String firstPart = "";
    String secPart = "";
    String thirdPart = "";
    for (int a = 0; a < 4; a++) firstPart += tempStr[a];firstPart += tempStr[a];
 
    for (int b = 4; b < 8; b++) secPart += tempStr[b];secPart += tempStr[b];

    for (int c = 8; c < 12; c++) thirdPart += tempStr[c];thirdPart += tempStr[c];

    data = strToInt(firstPart);
    data <<= 4;
    data |= strToInt(secPart);
    data <<= 4;
    data |= strToInt(thirdPart);

    String bitk = String(data, BIN);

    addZeroTo12Bit(&bitk); // ถ้าข้อมูลไม่ครบ 12 bit เติม 0 ข้างหน้าให้เติม
    for (int i = 0; i < bitk.length(); i++) dataArray12Bit[i] = String(bitk[i]).toInt();dataArray12Bit[i] = String(bitk[i]).toInt();
    type = 'I';

  }
  sendFrame(true, true); // สร้างเฟรมแบบพิเศษ​สำหรับส่งค่าที่เป็น x,y และ grayscale ต่างๆ
  data = 0;

}


String strTo4Bit(String str) // Format รูปแบบ
{
  String full = "";
  for (int i = 0; i < str.length(); i++)
  {
    int strInt = String(str[i]).toInt();
    String strBinary = String(strInt, BIN);
    addZeroTo4Bit(&strBinary);
    full += strBinary;
  }
  return full;
}
void prepareCapture() 
{
  mode = SPLIT_AXIS;
  /* สั่งให้นำข้อมูลที่ได้รับจากกล้องมา pack ข้อมูลให้อยู่ในรูปแบบที่พร้อมจะส่งเป็นเฟรม */
  if (pictureIndex < 3) {
    
    type = 'I';
    String colorCode = pictureStore[pictureIndex].substring(0, 4);
    String rotate = pictureStore[pictureIndex].substring(4);
    uint32_t tempData = 0;
    if (colorCode == "0000") tempData = 0;

    else if (colorCode == "0001") tempData = 1;

    else if (colorCode == "0010") tempData = 2;

    else if (colorCode == "0011") tempData = 3;

    else if (colorCode == "0100") tempData = 4;

    else if (colorCode == "0101") tempData = 5;

    else if (colorCode == "0110") tempData = 6;

    else if (colorCode == "0111") tempData = 7;

    else if (colorCode == "1000") tempData = 8;

    else if (colorCode == "1001") tempData = 9;

    else if (colorCode == "1010") tempData = 10;

    else if (colorCode == "1011") tempData = 11;

    else if (colorCode == "1100") tempData = 12;

    else if (colorCode == "1101") tempData = 13;

    else if (colorCode == "1110") tempData = 14;

    else if (colorCode == "1111") tempData = 15;

    if (rotate == "L")
    {
      tempData <<= 2;
      tempData |= B00;
    }
    else if (rotate == "C")
    {
      tempData <<= 2;
      tempData |= B01;
    }
    else if (rotate == "R")
    {
      tempData <<= 2;
      tempData |= B10;
    }
    tempData *= 64;
    data = tempData;
    sendFrame(true, false);
    pictureIndex++;
  }
  else mode = PIXEL_MODE; // ถ้าเกิน pictureIndex ให้เปลี่ยนเป็นโหมด PIXEL_MODE

}

void captureAll()
{
  Serial.flush();
  Serial.println(" - - Capture mode - - ");
  Serial.print("g"); // ส่งคำสั่งไปให้ฝั่ง python เริ่มถ่าย 3 มุม
}

void CRC() //เอา Data มาต่อ CRC
{
  if (outFrame != 0)
  {
    unsigned long long canXOR = 0x800000000;// ตัวเช็คขนาด data
    unsigned long long remainder = outFrame << 5; 
    unsigned long divisor = B101100; // Divisor
    unsigned long long tmp = canXOR & remainder; // เช็คตำแหน่งการ or ของ remainder กับ data
    while (tmp == 0)
    {
      canXOR >>= 1;
      tmp = canXOR & remainder;
    }
    tmp = canXOR & divisor;
    while (tmp == 0)
    {
      divisor <<= 1;
      tmp = canXOR & divisor;
    }
    while (divisor >= B101100)
    {
      tmp = remainder & canXOR;
      if (tmp > 0) remainder = remainder ^ divisor; // XOR
      divisor >>= 1;
      canXOR >>= 1;
    }
    outFrame <<= 5; //เติม0 5ตัว
    outFrame += remainder; //เปลี่ยน5บิตสุดท้ายเป็นremainder
  }

}
void makePixelFrame()
{
  outFrame = 0;
  switch (type)
  {
    case 'I':
      outFrame = IFrame;
      outFrame <<= 12;
      for (int i = 0; i < 12 ; i++)
      {
        outFrame <<= 1;
        outFrame |= dataArray12Bit[i];
      }
      outFrame <<= 1;
      outFrame |= frameNo;
      break;
    case 'S':
      outFrame = SFrame;
      outFrame <<= 12;
      outFrame |= data;
      outFrame <<= 1;
      outFrame |= ackNo;
      break;

    case 'U':
      outFrame = UFrame;
      outFrame <<= 13;
      outFrame |= data;
      break;

    default:
      Serial.println("Please defined type of the frame.");
      break;

  }
  CRC();
}

void makeFrame()
{
  /*
    Frame
    size : 21
    type: U-frame, S-Frame, I-frame
    U-Frame = [   100    |  00000...(13ตัว) | 00000]
                Type         cmd             CRC
    S-Frame = [   111    |     1000...(12ตัว)     |   0  | 00000 ]
               Type             data               ackNo   CRC
    I-Frame = [   110   | 110011...(12ตัว) |    0   | 00000 ]
               Type            Data         FrameNo    CRC
  */
  outFrame = 0;
  switch (type)
  {
    case 'I':
      outFrame = IFrame;
      outFrame <<= 12;
      outFrame |= data;
      outFrame <<= 1;
      outFrame |= frameNo;
      break;
    case 'S':
      outFrame = SFrame;
      outFrame <<= 12;
      outFrame |= data;
      outFrame <<= 1;
      outFrame |= ackNo;
      break;

    case 'U':
      outFrame = UFrame;
      outFrame <<= 13;
      outFrame |= data;
      break;

    default:
      Serial.println("Please defined type of the frame.");
      break;

  }

  CRC();

}
/* ใช้ CRC เช็ค Error ถ้ remainder = 0 แสดงว่าข้อมูลที่ได้รับถูกต้อง */
bool checkError(unsigned int frame)
{
  if (frame != 0)
  {
    unsigned long long canXOR = 0x80000000;
    unsigned long long remainder = frame;
    unsigned long divisor = B101100;
    unsigned long long tmp = canXOR & remainder;
    while (tmp == 0)
    {
      canXOR >>= 1;
      tmp = canXOR & remainder;
    }

    tmp = canXOR & divisor;
    while (tmp == 0)
    {
      divisor <<= 1;
      tmp = canXOR & divisor;
    }
    while (divisor >= B101100)
    {
      tmp = remainder & canXOR;
      if (tmp > 0)remainder = remainder ^ divisor;
      divisor >>= 1;
      canXOR >>= 1;
    }

    if (remainder == 0) return false;
    else return true;
  }
}

void sendFSK(int freq, int in_delay)
{
  int cyc;
  switch (freq)
  {
    case 500:
      cyc = 5;
      break;
    case 1400:
      cyc = 14;
      break;
  }

  for (int sl = 0; sl < cyc; sl++)
  {
    for (int s = 0; s < N; s++) 
    {
      dac.setVoltage(S_DAC[s], false);
      delayMicroseconds(in_delay);
    }
  }
}

void sendFrame(bool isFrame, bool special)
{
  if (isFrame) // ส่งแบบมีการจับเวลา Time out
  {
    startTimer = true;
    startTime = millis();
  }
  else
  {
    type = 'S';
    data = 0x801; // ตอบกลับ ack
  }
  switch (boolFrame)
  {
    case true:
      frameNo = 1;
      break;
    case false:
      frameNo = 0;
      break;
  }
  if (special) makePixelFrame(); // เฟรมสำหรับใช้ส่งในโหมด PIXEL เป็นการต่อข้อมูลให้ครบรูปแบบ
  else makeFrame();

  Serial.print("Sending Frame ");
  Serial.print("{");
  Serial.print(type);
  Serial.print("} : ");
  Serial.println(outFrame, BIN);
  // Frame มีทั้งหมด 16 (ขนาดเฟรม) + 5 (บิต CRC) = 21
  for (int i = 0; i < fullFrameSize; i ++)
  {
    int out = outFrame & 1;
    if (out == 0) sendFSK(f0, delay0);
    else if (out == 1) sendFSK(f3, delay3);
    outFrame >>= 1;
  }
  dac.setVoltage(0, false);
}

void addZeroTo4Bit(String * str) // Format รูปแบบ
{
  int sizeStr = str->length();
  String preset = "";
  for (int i = 0; i < 4 - sizeStr; i++)
  {
    preset += "0";
  }
  *str = preset + *str;
}


 // Format รูปแบบ
{
  int sizeStr = str->length();
  String preset = "";
  for (int i = 0; i < 12 - sizeStr; i++)
  {
    preset += "0";
  }
  *str = preset + *str;
}
void receiveFrame() {
  int tmp = analogRead(A0);
  
  if ( tmp > r_slope and prev < r_slope and !check_amp ) // check amplitude
  {
    check_amp = true; // is first max amplitude in that baud
    if ( !check_baud )
    {
      baud_begin = micros();
      bit_check++;
    }
  }

  if (tmp < r_slope and check_baud) {
    if (micros() - baud_begin  > 9900)
    {
      if (count >= 4 && count < 9) count = 5;
      else if (count >= 9) count = 14;

      uint16_t last = (((count - 5) / 9) & 1) << (bit_check);

      inFrame |= last;

      baud_check++;

      

      if (baud_check == 13) // 13 bits
      {
      
        checkFrame();
        Serial.flush();
        radio.setFrequency(frequency);
        clearDataArray12Bit();
        inFrame = 0;
        baud_check = 0;
        bit_check = -1;
        //        delay(500);
      }
      check_baud = false;
      count = 0;
    }
  }

  if (tmp > r_slope and check_amp) {
    count++;
    check_baud = true;
    check_amp = false;
  }
  prev = tmp;
  
}

//-------------------------------------Servo-------------------------------------//

void servoControl()
{
  if (Serial.available() > 0) {
    char input = Serial.read();
    if (input == 'A') {
      num = 40;
    }
    else if (input == 'B') {
      num = 90;
    }
    else if (input == 'C') {
      num = 144;
    }
    if (x < num) {
      while (x <= num) {
        myservo.write(x);
        x++;
        if (x == num) {
          myservo.write(x);
          //          Serial.println(x/10);
        }
        delay(10);
      }
    }
    else if (x > num) {
      while (x >= num) {
        myservo.write(x);
        x--;
        if (x == num) {
          myservo.write(x);
          //          Serial.println(x/10);
        }
        delay(10);
      }
    }
    delay(10);
  }
}
//-------------------------------------Timer-------------------------------------//

void timer()
{
  if (startTimer)
  {
    unsigned long long currentTime = millis();
    if (currentTime - startTime > timeOut)
    {
      startTime = currentTime;
      Serial.println("Retransmitting the frame ...");
      sendFrame(true, false);
    }
  }
}


//-----------------------------------Timer end-------------------------------------//

void loop()
{

  receiveFrame();
  servoControl();
  timer();
  if (Serial.available() > 0)
  {
    /*อ่านข้อมูลจากส่วน camera มาเก็บค่ารหัสของมุม 3 มุม ไว้ เพื่อเตรียม pack frame ส่งไปให้ PC 1 */
    if (mode == INIT_MODE)
    {
      String cameraBit = Serial.readStringUntil('/');
      servoControl();
      if (cameraBit.length() > 10)
      {
        Serial.print("Data from camera : ");
        Serial.println(cameraBit);
        gBit = cameraBit;
        String tmpS = "";
        int j = 0;
        for (int i = 0; i < cameraBit.length(); i++)
        {
          if (cameraBit[i] == ',')
          {
            pictureStore[j] = tmpS;
            tmpS = "";
            j++;
          }
          else
          {
            tmpS += cameraBit[i];
          }
        }
        mode = SPLIT_AXIS;
        prepareCapture();
      }

    }
    /*อ่านข้อมูลจากส่วน camera มาเก็บค่า x,y, grayscale ไว้ เพื่อเตรียม pack frame ส่งไปให้ PC 1 */
    else if (mode == PIXEL_MODE)
    {
      String axisBit = Serial.readStringUntil(',');

      if (axisBit.length() > 40 and sf == 0)
      {

        Serial.print("Data from firstTime : ");
        Serial.println(axisBit);
        gBit = axisBit;

        quadrant1[0] = gBit.substring(0, 3);
        quadrant1[1] = gBit.substring(3, 6);
        quadrant1[2] = gBit.substring(6, 9);
        quadrant1[3] = gBit.substring(9, 12);
        quadrant1[4] = gBit.substring(12, 15);

        quadrant2[0] = gBit.substring(15, 18);
        quadrant2[1] = gBit.substring(18, 21);
        quadrant2[2] = gBit.substring(21, 24);
        quadrant2[3] = gBit.substring(24, 27);
        quadrant2[4] = gBit.substring(27, 30);

        quadrant3[0] = gBit.substring(30, 33);
        quadrant3[1] = gBit.substring(33, 36);
        quadrant3[2] = gBit.substring(36, 39);
        quadrant3[3] = gBit.substring(39, 42);
        quadrant3[4] = gBit.substring(42, 45);
        sf = 1;
      }
      else if (axisBit.length() > 40 and sf == 1 ) {

        quadrant4[0] = axisBit.substring(0, 3);
        quadrant4[1] = axisBit.substring(3, 6);
        quadrant4[2] = axisBit.substring(6, 9);
        quadrant4[3] = axisBit.substring(9, 12);
        quadrant4[4] = axisBit.substring(12, 15);

        X[0] = axisBit.substring(15, 18);
        X[1] = axisBit.substring(18, 21);
        X[2] = axisBit.substring(21, 24);
        X[3] = axisBit.substring(24, 27);
        X[4] = axisBit.substring(27, 30);

        Y[0] = axisBit.substring(30, 33);
        Y[1] = axisBit.substring(33, 36);
        Y[2] = axisBit.substring(36, 39);
        Y[3] = axisBit.substring(39, 42);
        Y[4] = axisBit.substring(42, 45);

        sf = 0;
        
        prepareAnalysis();

      }

    }
  }
}
