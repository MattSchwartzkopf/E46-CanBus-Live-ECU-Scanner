#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include <SPI.h>

/*SAMD core*/
#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
  #define SERIAL SerialUSB
#else
  #define SERIAL Serial
#endif

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9; //MAYBE CHANGE TO 9, 11, 12, 13

const int Low_analogIn = 1; //Connect current sensor with A1 of Arduino
const int Drl_analogIn = 0; //Connect current sensor with A0 of Arduino
const int Turn_analogIn = 2; //Connect current sensor with A2 of Arduino
const int High_analogIn = 3; //Connect current sensor with A2 of Arduino
const int Sublow_analogIn = 4; //Connect current sensor with A2 of Arduino

int Analogue[5] = { Drl_analogIn, Low_analogIn, Turn_analogIn, High_analogIn, Sublow_analogIn };

unsigned int RawValue[5] = {0,0,0,0,0};
float Voltage[5] = {0,0,0,0,0};
float Amps[5] = {0,0,0,0,0};

int mVperAmp = 185; //Use 185 for 5A Sensor
int ACoffset = 2500;

/*Maximum Payload 8Bytes*/
typedef union
{
 unsigned int in_vol[5];
 uint8_t bytes[10];
} INTUNION_t;

INTUNION_t myint;

/*Drl Low Turn High SubLow Pstn */ 
unsigned char invol_arr[10] = {0,0,0,0,0,0,0,0,0,0};
unsigned char invol_arr1[2] = {0,0};
unsigned int output_var;

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

void setup()
{
    SERIAL.begin(115200);

    while (CAN_OK != CAN.begin(CAN_100KBPS))              // init can bus : baudrate = 500k
    {
        SERIAL.println("CAN BUS Shield init fail");
        SERIAL.println(" Init CAN BUS Shield again");
        delay(100);
    }
    SERIAL.println("CAN BUS Shield init ok!");
}

void loop()
{
    static int ibuf=0;
    unsigned int i;
    int AvgSnsrData[6]={0};   

    /*
     * Read ADC Values from pins0~4
     * Store in RawValue Array 
     * Calculate Voltage
     * Calculate Current
     * Save Voltage into Union Variable
     * Get the byte values for Rawvalues From the same memory space using Unions
     * Send them in Message ID's 0x300(8Bytes) and 0x301(2Bytes)
    */
    
    for(i=0;i<5;i++)
    {
      AvgSnsrData[i] = analogRead(Analogue[i]);
      delay(0.1);

      RawValue[i] = AvgSnsrData[i];
      Voltage[i] = (RawValue[i] / 1024.0) * 5000; // Gets you mV
      Amps[i] = ((Voltage[i] - ACoffset) / mVperAmp);
      delay(0.1);
      
      myint.in_vol[i] = RawValue[i];  //Assign input voltage digital value
      
      for(int j = i;j < i+1 ;j++)
      {
          invol_arr[j*2] = myint.bytes[j*2];
          invol_arr[(j*2)+1] = myint.bytes[(j*2)+1];
      }
      
     }
    
    Serial.print(Amps[0]);
    invol_arr1[0] = invol_arr[9];
    invol_arr1[1] = invol_arr[10];

    /*Shifting bytes to get original input voltage digital value*/
    output_var = ((myint.bytes[1]<<8)|(myint.bytes[0]<<0)) ;

    /*
     * Current Value in Uint16 Data Type
     * Messsage Id:0x300
     * Length :2Bytes
     */
    CAN.sendMsgBuf(0x300,0,8, invol_arr);
    CAN.sendMsgBuf(0x301,0,2, invol_arr1);
    
    Serial.print('\n');
   
    delay(10);                       // send data per 10ms
}

// END FILE
