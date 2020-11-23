


#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <TEA5767.h>
/********************************************FSK********************************************/

#define r_slope 400
// Config later
#define defaultFreq 1700
#define f0 500
#define f1 800
#define f2 1100
#define f3 1400

#define N 4

#define IFrame 0
#define SFrame 2
#define UFrame 3

#define fullFrameSize 12

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

int delay0, delay1, delay2, delay3;
char type = 'O';
String cmd = "";
int mode = 0;
unsigned int frameNo = 0, ackNo = 0;
int countPic = 0;
int picture[3] = { -1, -1, -1};

unsigned long long startTime = 0, timeOut = 10000;
uint16_t outFrame = 0;
int prev = 0;
int count = 0;

uint16_t baud_check = 0;
uint32_t data = 0;
uint16_t bit_check = -1;

int test[32];
uint32_t inFrame = 0;
bool check_amp = false;
bool check_baud = false;

const float frequency = 102.4; //Enter your own Frequency

uint32_t baud_begin = 0;


void setup() {
  Serial.begin(115200);
  dac.begin(0x64);
  Wire.begin();
  for (int i = 0; i < 32; i++)
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
/*
    Frame
        - type: U-frame, S-Frame, I-frame
        - frameSize: 7bits

    U-Frame = [   00    |  00000 ]
                   Type    cmd
    S-Frame = [   01    |  0000 | 0 ]
                 Type      cmd  ackNo
    I-Frame = [   11    |  0  | 0000 ]
                 Type    FrameNo  Data
*/


void sendFrame(bool isFrame)
{
  makeFrame();
  Serial.print("Sending Frame: ");
  Serial.println(outFrame, BIN);
  if (isFrame)
  {
    startTimer = true;
  }
  // Frame มีทั้งหมด 7 (ขนาดเฟรม) + 5 (บิต CRC) = 12
  for (int i = 0; i < fullFrameSize; i += 1)
  {
    int out = outFrame & 1;
    if (out == 0) sendFSK(f0, delay0);
    //        else if (out == B01) sendFSK(f1, delay1);
    //        else if (out == B10) sendFSK(f2, delay2);
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
    //        case 800:
    //            cyc = 8;
    //            break;
    //        case 1100:
    //            cyc = 11;
    //            break;
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
    Frame
    type: U-frame, S-Frame, I-frame
    U-Frame = [   00    |  00000 ]
                 Type    cmd
    S-Frame = [   01    |  0000 | 0 ]
               Type      cmd  ackNo
    I-Frame = [   11    |  0  | 0000 ]
               Type    FrameNo  Data
  */

  /*
    Frame + CRC
    Frame size: 12 bits
    type: U-frame, S-Frame, I-frame
    U-Frame = [   00    |  00000 | 00000]
                 Type    cmd      CRC
    S-Frame = [   01    |  0000 | 0 | 00000]
               Type      cmd  ackNo  CRC
    I-Frame = [   11    |   0    | 0000 | 00000]
               Type    FrameNo  Data     CRC
  */
  switch (type)
  {
    case 'I':
      outFrame = IFrame;
      outFrame <<= 1;
      outFrame |= frameNo;
      outFrame <<= 4;
      outFrame |= data;

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
  CRC();

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
    int inFrameType = inFrame >> 30;
    Serial.println("No error");
    if (inFrameType == SFrame)
    {
      uint32_t tmp = (inFrame >> 5) & 0x1ffffff;
      Serial.println(tmp,HEX);
      if (tmp == 0x1555555)
      {
        Serial.println("Received ack.");
        startTimer = false;
        canSend = true;
        if (mode == CAPTURE_MODE)
        {
          Serial.println("/*--------------------- The connection has been established. ---------------------*/");
          Serial.println("Type \"capture\" or \"c\" to start Capturing.");
          mode = DISPLAY_MODE;
        }
        else if (mode == DISPLAY_MODE)
        {
          Serial.println("Type \"Display\" or \"d\" to start Display in 3 directions(-45,0,+45).");
          mode = ANALYSIS_MODE; 
        }
        else if (mode == ANALYSIS_MODE)
        {
          
        }
      }

      
    }
    if (inFrameType == UFrame)
    {
      // For terminated connection .
      uint32_t tmp = (inFrame >> 5) & 0x1ffffff;
      

    }
    else if (inFrameType == IFrame)
    {

      int tmpAck = (inFrame >> 15) & 1;

      if (tmpAck == ackNo)
      {
        //Scan mode
        if (mode == 1)
        {

        }
        //Analysis mode
        else if (mode == 2)
        {

        }

        if (ackNo) ackNo = 0;
        else ackNo = 1;
        data = 0; // <--- define data
        receivedAck = true;
      }
      else
      {
        receivedAck = false;
      }
      type = 'S';
      data = 0x1ffffff;
      sendFrame(false);
      delay(1000);
      if (receivedAck)
      {
        if (startCapture)
        {
          // Display value from camera
          Serial.println("/*--------------------- Display ---------------------*/");
          //                    Serial.println("Left :", );
          //                    Serial.println("Center :", );
          //                    Serial.println("Right :", );
        }
      }
    }
  }
  else
  {
    Serial.println("Error detected in data");
  }
}

void formatFrame(int inputFrame[])
{
  for (int i = 0; i < 32; i++)
  {
    Serial.print(inputFrame[i]);
  }
  Serial.println();
}
void clearInFrame()
{
  for (int i = 0; i < 32; i++)
  {
    test[i] = 0;
  }
}


void receiveFrame() {
  int tmp = analogRead(A0);
  //  Serial.println(tmp);

  if (tmp > r_slope and prev < r_slope and !check_amp ) // check amplitude
  {
    //    radio.setFrequency(frequency);
    //    delay(610);
    //Serial.println(tmp);
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
      //Serial.println(count);
      //      if (count > 1 && count < 7) {
      //        count = 5 ;
      //      }
      //      else if (count >= 7 && count < 10) {
      //        count = 8;
      //      }
      //      else if (count >= 10 && count < 13) {
      //        count = 11;
      //      }
      //      else if (count >= 13) {
      //        count = 14;
      //      }
      if (count >= 4 && count < 9)
      {
        count = 5;
      }
      else if (count >= 9)
      {
        count = 14;
      }
      //      Serial.println(count);

      int onebit = ((count - 5) / 9);


      test[31 - baud_check] = onebit;
      //      uint32_t last = (((count - 5) / 9) & 1) << (bit_check);

      //      Serial.println(last,BIN);
      //      inFrame |= last;
      //      Serial.println(inFrame,BIN);
      baud_check++;

      if (baud_check == 32) // 32 bits
      {
        //        Serial.print("Receive Frame : ");
        //        formatFrame(test);
        for (int i = 0; i < 32; i++)
        {
          inFrame <<= 1;
          inFrame |= test[i];
        }
        Serial.print("InFrame: ");
        Serial.println(inFrame, BIN);
        checkFrame();
        //        Serial.flush();
        //        radio.setFrequency(frequency);
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
    //Serial.println(tmp);
    check_baud = true;
    check_amp = false;
  }
  prev = tmp;
  //Serial.println(micros() - baud_begin);
}


void loop()
{

  receiveFrame();

  if (Serial.available() > 0 && canSend == true)
  {

    cmd = Serial.readString();
    Serial.println(cmd);
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
        type = 'I';
        data = B1111;
        mode = 1;
        // ทำให้ โหมด เป็นอันเดียวกัน
      }
      else if (mode == 1)
      {
        if (cmd.equalsIgnoreCase("left") || cmd.equalsIgnoreCase("l"))
        {
          data = B0000;
        }
        else if (cmd.equalsIgnoreCase("right") || cmd.equalsIgnoreCase("r"))
        {
          data = B0010;
        }
        else if (cmd.equalsIgnoreCase("top") || cmd.equalsIgnoreCase("t"))
        {
          data = B0100;
        }
        else if (cmd.equalsIgnoreCase("bottom") || cmd.equalsIgnoreCase("b"))
        {
          data = B1000;
        }

      }
      type = 'I';
      sendFrame(true);
      canSend = false;
    }

  }

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
