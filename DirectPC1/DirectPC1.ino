
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <TEA5767.h>
#include <Vector.h>
/********************************************FSK********************************************/

#define r_slope 400
// Config later
#define defaultFreq 1700
#define f0 500
#define f1 800
#define f2 1100
#define f3 1400

#define N 4

#define IFrame 7
#define UFrame 4
#define SFrame 6

#define fullFrameSize 13
#define receiveFullFrameSize 21

#define CAPTURE_MODE 0
#define DISPLAY_MODE 1
#define ANALYSIS_MODE 2

double S[N];
uint16_t S_DAC[N];
Adafruit_MCP4725 dac;
TEA5767 radio = TEA5767();

bool canSend = true;
bool initialize = true;
bool startTimer = false;
bool receivedAck = false;
bool startCapture = true;
bool boolFrame = false;
bool boolAck = true;


int delay0, delay1, delay2, delay3;
char type = 'O';
String cmd = "";
int mode = CAPTURE_MODE;
unsigned int frameNo = 0, ackNo = 0;
int countPic = 0;
String pictureStore[3];

unsigned long long startTime = 999999999, timeOut = 10000;
uint16_t outFrame = 0;
int prev = 0;
int count = 0;
int pictureIndex = 0;

uint16_t baud_check = 0;
uint32_t data = 0;
uint16_t bit_check = -1;

int test[21];
uint32_t inFrame = 0;
bool check_amp = false;
bool check_baud = false;

const float frequency = 102.4; //Enter your own Frequency

uint32_t baud_begin = 0;

String quadrant1[4];
int qIndex1 = 0;
String quadrant2[4];
int qIndex2 = 0;
String quadrant3[4];
int qIndex3 = 0;
String quadrant4[4];
int qIndex4;
String X[4];
int xIndex;
String Y[4];
int yIndex;

const int ELEMENT_COUNT_MAX = 4;
int storage_array1[ELEMENT_COUNT_MAX];
int storage_array2[ELEMENT_COUNT_MAX];
int storage_array3[ELEMENT_COUNT_MAX];
int storage_array4[ELEMENT_COUNT_MAX];
Vector<int> q1(storage_array1);
Vector<int> q2(storage_array2);
Vector<int> q3(storage_array3);
Vector<int> q4(storage_array4);



void setup() {
  Serial.begin(115200);
  dac.begin(0x60);
  Wire.begin();
 
  for (int i = 0; i < 21; i++)
  {
    test[i] = 0;
  }

  for (int i = 0; i < N; i++) {
    S[i] = sin(2 * PI * (i / (double)N));
    S_DAC[i] = 4095 / 2.0 * (1 + S[i]);
    //        Serial.println(S[i]);
  }

  delay0 = (1000000 / f0 - 1000000 / defaultFreq) / N;
  delay1 = (1000000 / f1 - 1000000 / defaultFreq) / N;
  delay2 = (1000000 / f2 - 1000000 / defaultFreq) / N;
  delay3 = (1000000 / f3 - 1000000 / defaultFreq) / N;


  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);
  pinMode(A0, INPUT);

  radio.setFrequency(frequency);
  Serial.flush();
//  delay(2000);
}


void sendFrame(bool isFrame)
{
  if (isFrame)
  {
    startTimer = true;
    startTime = millis();
  }
  else
  {
    type = 'S';
    data = 0x9;
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
  
  makeFrame();
  Serial.print("Sending Frame ");
  Serial.print("{");
  Serial.print(type);
  Serial.print("} : ");
  Serial.println(outFrame, BIN);
  // Frame มีทั้งหมด 8 (ขนาดเฟรม) + 5 (บิต CRC) = 13
  for (int i = 0; i < fullFrameSize; i += 1)
  {
    int out = outFrame & 1;
    if (out == 0) sendFSK(f0, delay0);
    else if (out == 1) sendFSK(f3, delay3);
    outFrame >>= 1;
    //delay(1000);
  }
  //    delay(1000);
  dac.setVoltage(0, false);
}

void sendFSK(int freq, int in_delay) //Config later
{
  int cyc;
  switch (freq)
  {
    case 500: //500
      cyc = 5;
      break;

    case 1400: //1400
      cyc = 14;
      break;
  }
  for (int sl = 0; sl < cyc; sl++)
  {
    for (int s = 0; s < N; s++) //4 sample/cycle
    {
      dac.setVoltage(S_DAC[s], false);//modify amplitude
      delayMicroseconds(in_delay);
    }
  }

}


void makeFrame()
{
  outFrame = 0;
  /*
    Frame + CRC
    Frame size: 13 bits
    type: U-frame, S-Frame, I-frame
    U-Frame = [   100    |  00000 | 00000]
                 Type       cmd      CRC
    S-Frame = [   111    |  0000 | 0 | 00000]
               Type         cmd  ackNo  CRC
    I-Frame = [   110    |   0000   |    0     | 00000 ]
               Type          Data     FrameNo    CRC
  */
  switch (type)
  {
    case 'I':
      outFrame = IFrame;
      outFrame <<= 4;
      outFrame |= data;
      outFrame <<= 1;
      outFrame |= frameNo;
      break;
    case 'S':
      outFrame = SFrame;
      outFrame <<= 4;
      outFrame |= data;
      outFrame <<= 1;
      outFrame |= ackNo;
      break;
    case 'U':
      outFrame = UFrame;
      outFrame <<= 5;
      outFrame |= data;
      break;
    default:
      Serial.println("Please defined type of the frame.");
      break;

  }
//  Serial.print("Before CRC : ");
//  Serial.println(outFrame,BIN);
  
  CRC();
//  Serial.print("After CRC : ");
//  Serial.println(outFrame,BIN);

}

void CRC()
{
  if (outFrame != 0)
  {
    unsigned long long canXOR = 0x800000000;//ตั้งใหญ่ๆไว้ก่อนเพื่อเอามาเช็คขนาดดาต้า
    unsigned long long remainder = outFrame << 5;//ตัวแปรใหม่ที่มาจากการเติม 0 หลัง outFrame 5 ตัว
    unsigned long divisor = B110101;//กำหนด divisor
    unsigned long long tmp = canXOR & remainder;//ไว้ตรวจว่าจะเอา remainder ไป or ตรงไหนใน data(ต้อง XOR ตัวหน้าสุดก่อน)
    while (tmp == 0)
    {
      canXOR >>= 1;//ปรับขนาดให้เท่ากับดาต้า
      tmp = canXOR & remainder;
    }
    tmp = canXOR & divisor;//ไว้ตรวจว่าจะเริ่ม XOR ตำแหน่งไหน
    while (tmp == 0)
    {
      divisor <<= 1;//ชิพไปเรื่อยๆจนกว่าจะถึงตำแหน่ง XOR แรก
      tmp = canXOR & divisor;
    }
    while (divisor >= B110101)//ทำจนกว่า divisor จะน้อยกว่าค่าที่กำหนด
    {
      tmp = remainder & canXOR;//ดูว่าตำแหน่งนี้ XOR ได้หรือไม่(XOR ตำแหน่งที่เป็น 1xxxxx)
      if (tmp > 0)remainder = remainder ^ divisor;//ทำการ XOR
      divisor >>= 1;
      canXOR >>= 1;
    }
    outFrame <<= 5;//เติม0 5ตัว
    outFrame += remainder;//เปลี่ยน5บิตสุดท้ายเป็นremainder
  }
}
bool checkError(uint32_t frame)
{
  if (frame != 0)
  {
    unsigned long long canXOR = 0x800000000;//ตั้งใหญ่ๆไว้ก่อนเพื่อเอามาเช็คขนาดดาต้า
    unsigned long long remainder = frame;//ตัวแปรใหม่ที่มาจากการเติม 0 หลัง outFrame 5 ตัว
    unsigned long divisor = B110101;//กำหนด divisor
    unsigned long long tmp = canXOR & remainder;//ไว้ตรวจว่าจะเอา remainder ไป or ตรงไหนใน data(ต้อง XOR ตัวหน้าสุดก่อน)
    while (tmp == 0)
    {
      canXOR >>= 1;//ปรับขนาดให้เท่ากับดาต้า
      tmp = canXOR & remainder;
    }
    tmp = canXOR & divisor;//ไว้ตรวจว่าจะเริ่ม XOR ตำแหน่งไหน
    while (tmp == 0)
    {
      divisor <<= 1;//ชิพไปเรื่อยๆจนกว่าจะถึงตำแหน่ง XOR แรก
      tmp = canXOR & divisor;
    }
    while (divisor >= B110101)//ทำจนกว่า divisor จะน้อยกว่าค่าที่กำหนด
    {
      tmp = remainder & canXOR;//ดูว่าตำแหน่งนี้ XOR ได้หรือไม่(XOR ตำแหน่งที่เป็น 1xxxxx)
      if (tmp > 0)remainder = remainder ^ divisor;//ทำการ XOR
      divisor >>= 1;
      canXOR >>= 1;
    }
//            Serial.print("Remainder : ");
//            Serial.println(remainder);
    
    if (remainder == 0) return false;
    else return true;
  }
}
void checkFrame()
{
  if (checkError(inFrame) == false) // No error
  {
    int inFrameType = (inFrame >> 18);
    String t = "";
    switch (inFrameType)
    {
        case 7:
          t="I";
          break;
        case 6:
          t="S";
          break;
        case 4:
          t="U";
          break;  
    }
    Serial.print("Receive Frame {");
    Serial.print(t);
    Serial.print("} : ");
    Serial.println(inFrame, BIN);
    if (inFrameType == SFrame) //S-Frame
    {
//       Serial.println(inFrame >> 6, BIN);
       uint32_t tmp = (inFrame >> 6) & 0x1fff;
      if (tmp == 0x801)
      {
        Serial.println("Received ack.");
        startTimer = false;
        boolFrame = !boolFrame;
        canSend = true;
        if (mode == CAPTURE_MODE)
        {
          Serial.println("/*--------------------- The connection has been established. ---------------------*/");
          Serial.println("Type \"capture\" or \"c\" to start Capturing.");
          mode = DISPLAY_MODE;
        }
        else if (mode == DISPLAY_MODE)
        {
//          Serial.println("Type \"Display\" or \"d\" to start Display in 3 directions(-45,0,+45).");
          mode = ANALYSIS_MODE; 
        }
        else if (mode == ANALYSIS_MODE)
        {
          Serial.println("/*--------------------- Analysis mode ---------------------*/");
        }
      }

      
    }
    if (inFrameType == UFrame) //U-Frame
    {
      // For terminated connection .
      uint32_t tmp = (inFrame >> 5) & 0x1ffffff;
      

    }
    else if (inFrameType == IFrame) //I-Frame
    {
      sendFrame(false);
      if (mode == DISPLAY_MODE)
      {
        inFrame &= 0x3f000;
        inFrame /= 4096; 
      } 
      else if (mode == ANALYSIS_MODE)
      {
        inFrame &= 0xfffe0;
        inFrame /= 64;
        inFrame &= 0x3ff;
      }
      
         
      switch (mode) 
      {
          case DISPLAY_MODE:
            pictureStore[pictureIndex] = String(inFrame,BIN);
            pictureIndex++;
            break;
          case ANALYSIS_MODE:
          String readyInFrame = String(inFrame,BIN);
          
          Serial.println(readyInFrame);
          addZeroTo12Bit(&readyInFrame);
          
          
          int number = convertToInt(readyInFrame);
          if (qIndex1 < 4)
          {
            quadrant1[qIndex1] = number;
            qIndex1++;
          }
          else if (qIndex2 < 4)
          {
            quadrant2[qIndex2] = number;
            qIndex2++;
          }
          else if (qIndex3 < 4)
          {
            quadrant3[qIndex3] = number;
            qIndex3++;
          }
          else if (qIndex4 < 4)
          {
            quadrant4[qIndex4] = number;
            qIndex4++;
          }
          else if (xIndex < 4)
          {
            X[xIndex] = number;
            xIndex++;
          }
          else if (yIndex < 4)
          {
            Y[yIndex] = number;
            yIndex++;
          }
          break;
            
      }
      if (pictureIndex == 3 and mode == DISPLAY_MODE) 
      {
        for (int i = 0 ; i < pictureIndex;i++) addZero(&pictureStore[i]);
        displayValue();
      }
      
      if (yIndex == 4 and mode == ANALYSIS_MODE)
      {
        for (int i = 0; i < qIndex1;i++)
        {
          Serial.print("Quadrant : ");
          Serial.println(quadrant1[i]);  
        }
        displayPixel();
      }

//      int tmpAck = (inFrame >> 15) & 1;
//      if (tmpAck == ackNo)
//      {
//        //Scan mode
//        if (mode == 1)
//        {
//
//        }
//        //Analysis mode
//        else if (mode == 2)
//        {
//
//        }
//
//        if (ackNo) ackNo = 0;
//        else ackNo = 1;
//        data = 0; // <--- define data
//        receivedAck = true;
//      }
//      else
//      {
//        receivedAck = false;
//      }
//      sendFrame(false);
//      if (receivedAck)
//      {
//        if (startCapture)
//        {
//          // Display value from camera
//          Serial.println("/*--------------------- Display ---------------------*/");
//          //                    Serial.println("Left :", );
//          //                    Serial.println("Center :", );
//          //                    Serial.println("Right :", );
//        }
//      }
    }
  }
  else
  {
    Serial.println("Error detected in data");
  }
}

void displayValue()
{
  Serial.println("/*--------------------- Display ---------------------*/");
  for (int i = 0; i < sizeof(pictureStore)/sizeof(pictureStore[0]);i++)
  {
    String checkDirection = "";
    for (int j = 4; j < pictureStore[i].length();j++) checkDirection += pictureStore[i][j]; 
    if (checkDirection == "00")
    {
      Serial.print("Left Picture is ");
      for (int k = 0; k < 4;k++) Serial.print(pictureStore[i][k]);
      Serial.println();
    }
    else if (checkDirection == "01")
    {
      Serial.print("Center Picture is ");
      for (int k = 0; k < 4;k++) Serial.print(pictureStore[i][k]);
      Serial.println();
    }
    else if (checkDirection == "10")
    {
      Serial.print("Right Picture is ");
      for (int k = 0; k < 4;k++) Serial.print(pictureStore[i][k]);
      Serial.println();
    }
  }
  mode = ANALYSIS_MODE;
  canSend = true;
  
}
void addZeroTo12Bit(String * str)
{
  int sizeStr = str->length();
  String preset = "";
  for (int i = 0; i < 12 - sizeStr; i++)
  {
    preset += "0";
  }
  *str = preset + *str;
}
void addZero(String* str)
{
  int sizeStr = str->length();
  String preset = "";
  for (int i = 0; i < 6 - sizeStr;i++)
  {
      preset += "0";
  }  
  *str = preset + *str;
}

void formatFrame(int inputFrame[])
{
  for (int i = 0; i < receiveFullFrameSize; i++)
  {
    Serial.print(inputFrame[i]);
  }
  Serial.println();
}
void clearInFrame()
{
  for (int i = 0; i < receiveFullFrameSize; i++)
  {
    test[i] = 0;
  }
}
int convertToInt(String str)
{
    String firstPart = "";
    String secPart = "";
    String thirdPart = "";
    Serial.print("str : ");
    Serial.println(str);
    for (int a = 0; a < 4; a++)
    {
      firstPart += str[a];
    }
    for (int b = 4; b < 8; b++)
    {
      secPart += str[b];
    }
    for (int c = 8; c < 12; c++)
    {
      thirdPart += str[c];
    }
    Serial.print("First part : ");
    Serial.println(firstPart);
    Serial.print("Second part : ");
    Serial.println(secPart);
    Serial.print("Third part : ");
    Serial.println(thirdPart);
    String total = String(strToInt(firstPart)) + String(strToInt(secPart)) + String(strToInt(thirdPart)); 
    Serial.print("total : ");
    Serial.println(total);
    return total.toInt();
  
    
    
}

int strToInt(String colorCode)
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
void calcXY()
{
  for (int i = 0; i < 2;i++)
  {
    for (int j = 0;j < 2;j++)
    {
      int q1_s[2] = {Y[i].toInt(),X[j].toInt()};
      int q2_s[2] = {Y[i].toInt(),X[j+2].toInt()};
      int q3_s[2] = {Y[i+2].toInt(),X[j].toInt()};
      int q4_s[2] = {Y[i+2].toInt(),X[j+2].toInt()};
      q1.push_back(q1_s);
      q2.push_back(q2_s);
      q3.push_back(q3_s);
      q4.push_back(q4_s);  
    }
  }
}
void displayPixel()
{  
  calcXY();
  Serial.println(" ---------- Quadrant 1 ---------- ");
  formatQ(q1,quadrant1[0], quadrant1[1], quadrant1[2], quadrant1[3]);
  Serial.println(" ---------- Quadrant 2 ---------- ");
  formatQ(q2,quadrant1[0], quadrant1[1], quadrant1[2], quadrant1[3]);
  Serial.println(" ---------- Quadrant 3 ---------- ");
  formatQ(q3,quadrant1[0], quadrant1[1], quadrant1[2], quadrant1[3]);
  Serial.println(" ---------- Quadrant 4 ---------- ");
  formatQ(q4,quadrant1[0], quadrant1[1], quadrant1[2], quadrant1[3]);
}

void formatQ(Vector<int> qa, String c1, String c2, String c3, String mean)
{
  Serial.print("At (");
  Serial.print(qa[0]);
  Serial.print(",");
  Serial.print(qa[1]);
  Serial.print(") : (");
  Serial.print(c1);
  Serial.print(", ");
  Serial.print(c2);
  Serial.print(", ");
  Serial.print(c3);
  Serial.print(")");
  Serial.print(" | Mean : ");
  Serial.println(mean);
}
void receiveFrame() {
  int tmp = analogRead(A0);
  //  Serial.println(tmp);

  if (tmp > r_slope and prev < r_slope and !check_amp ) // check amplitude
  {
    //    radio.setFrequency(frequency);
    //    delay(610);
    check_amp = true; // is first max amplitude in that baud
    if ( !check_baud )
    {
      baud_begin = micros();
      bit_check++;
    }
  }

  if (tmp < r_slope and check_baud) {
    if (micros() - baud_begin  > 9900) // full baud
    {
     
      if (count >= 4 && count < 9) count = 5;
      else if (count >= 9) count = 14;
      
      int onebit = ((count - 5) / 9);

      test[20 - baud_check] = onebit;
      //      uint32_t last = (((count - 5) / 9) & 1) << (bit_check);

      //      Serial.println(last,BIN);
      //      inFrame |= last;
      //      Serial.println(inFrame,BIN);
      
      baud_check++;
      if (baud_check == receiveFullFrameSize) // 21 bits
      {
        for (int i = 0; i < receiveFullFrameSize; i++)
        {
          inFrame <<= 1;
          inFrame |= test[i];
        }
        
        
        checkFrame();
        //        Serial.flush();
        //        radio.setFrequency(frequency);
        inFrame = 0;
        clearInFrame();
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

void timer()
{
  if (startTimer)
  {
    unsigned long long currentTime = millis();
    if (currentTime - startTime > timeOut)
    {
      startTime = currentTime;
      Serial.println("Retransmitting the frame ...");
      sendFrame(true);
    }
  }
}


void loop()
{

  receiveFrame();

  if (Serial.available() > 0 && canSend == true)
  {

    cmd = Serial.readString();
    if (initialize && cmd.equalsIgnoreCase("start") || cmd.equalsIgnoreCase("s"))
    {
      type = 'U';
      data = B00110;
      sendFrame(true);
      initialize = false;
    }
    else
    {
      if (mode == CAPTURE_MODE && cmd.equalsIgnoreCase("capture") || cmd.equalsIgnoreCase("c"))
      {
        data = B1111;
        mode = DISPLAY_MODE;
      }
      else if (mode == ANALYSIS_MODE)
      {
        // The most preformance code in the world.
        if (cmd == "0000") data = 0;
        else if (cmd == "0001") data = 1;
        else if (cmd == "0010") data = 2;
        else if (cmd == "0011") data = 3;
        else if (cmd == "0100") data = 4;
        else if (cmd == "0101") data = 5;
        else if (cmd == "0110") data = 6;
        else if (cmd == "0111") data = 7;
        else if (cmd == "1000") data = 8;
        else if (cmd == "1001") data = 9;
        else if (cmd == "1010") data = 10;
        else if (cmd == "1011") data = 11;
        else if (cmd == "1100") data = 12;
        else if (cmd == "1101") data = 13;
        else if (cmd == "1110") data = 14;
        else if (cmd == "1111") data = 15;
      }
      type = 'I';
      sendFrame(true);
      canSend = false;
      boolFrame = !boolFrame;
    }

  }
//  timer();

 

}
