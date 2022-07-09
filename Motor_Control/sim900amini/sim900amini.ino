#include <SoftwareSerial.h>
#include <avr/wdt.h>

SoftwareSerial SIM900A(10,11);
#define RECEIVED 1
#define NOT_RECEIVED 0

#define DEFAULT_VALUE 0
#define SUCCESS 0
#define RELAY_POWER_ON_PIN 8
#define GPS_RX_BUFF_SIZE 256
#define GPS_RX_PH_NO_SIZE 16
#define MAX_FIXED_PHNOS 4
#define RESERVED_BITS 24
#define FIXED_PH_NO_INDEX_0 0
#define FIXED_PH_NO_INDEX_1 1
#define FIXED_PH_NO_INDEX_2 2
#define FIXED_PH_NO_INDEX_3 3
#define FIXED_PH_NO_0 "+919176987858"
#define FIXED_PH_NO_1 "+916369939554"
#define FIXED_PH_NO_2 "+91XXXXXXXXXX"
#define FIXED_PH_NO_3 "+91XXXXXXXXXX"
#define ACK_SEND_PHNO "+919442804249"
#define FIXED_REPLY_NO FIXED_PH_NO_1
// Set timer1_counter to the correct value for our interrupt interval
//timer1_counter = 64911;   // preload timer 65536-16MHz/256/100Hz
//timer1_counter = 65500;   // preload timer 65536-16MHz/256/100Hz
//timer1_counter = 64286;   // preload timer 65536-16MHz/256/50Hz
//timer1_counter = 34286;   // preload timer 65536-16MHz/256/2Hz
#define TIMER_COFIGURATION_VALUE 34286

#define BASE_ERROR -1000
#define LOAD_DEFAULT_BASE 100
#define TIMER_CONFIGURATION_BASE 200
#define PUT_RECEIVE_MSG_READ_MODE 300
#define COMMAND_PROCESSOR_ERROR_BASE 400

#define LOAD_DEFAULT_FAILURE (BASE_ERROR+LOAD_DEFAULT_BASE)

#define TIMER_CONFIGURAITON_FAILURE  (BASE_ERROR+TIMER_CONFIGURATION_BASE)

#define PUT_RECEIVE_MSG_READ_MODE_FAILURE (BASE_ERROR+PUT_RECEIVE_MSG_READ_MODE)

#define COMMAND_PROCESSOR_ERROR (BASE_ERROR+COMMAND_PROCESSOR_ERROR_BASE )

#define LOAD_DEFAULT_NULL_HANDLING LOAD_DEFAULT_FAILURE

#define LOAD_TIMER_CONFIGURATION_BASE_1 TIMER_CONFIGURAITON_FAILURE

#define PUT_RECEIVE_MSG_READ_MODE_FAILURE_1 PUT_RECEIVE_MSG_READ_MODE_FAILURE

#define COMMAND_PROCESSOR_ERROR_NULL_HANDLING COMMAND_PROCESSOR_ERROR

/* structure to read received msg from gps */
typedef struct
{
  char arrcSim900aReceivedString[GPS_RX_BUFF_SIZE];
  unsigned char arrMsgRxPhNo[GPS_RX_PH_NO_SIZE];
  unsigned int uiMsgRxPhNoCount;
  
}S_Rx_Msg;

/*Command processor related variables*/
typedef struct 
{
  unsigned char ucCommandRxState;
  unsigned char ucStringReceived;  
  unsigned char ucPowerOnCommandRx;
  unsigned char ucPowerOffCommandRx;
  unsigned char ucTempChar;
  unsigned char ucReserved;
  unsigned char ucReserved1;
  unsigned char ucReserved2;
    
}S_Command_Processor;

/*Fixed phone numbers structures*/
typedef struct 
{
  unsigned char arrFixedPhNos[GPS_RX_PH_NO_SIZE];
  unsigned char arrFixedReplyPhNo[GPS_RX_PH_NO_SIZE];
}S_arrFixedPhNos;

/*Relay related variables*/
typedef struct 
{
  int iRelayPin;
  
}S_Relay;

/*Timer related Variables*/
typedef struct
{
  int timer1_counter;
  
}S_Timer;

/*Global structures */
typedef struct 
{

  S_Rx_Msg s_Rx_Msg;
  S_Command_Processor s_Command_Processor;
  S_arrFixedPhNos s_arr_Fixed_PhNos[MAX_FIXED_PHNOS];
  S_Relay s_Relay;
  S_Timer s_Timer;
  
}G_Global_Handle;

G_Global_Handle g_Global_Handle;

/*  +CMT: "+917708720020","","22/07/02,23:38:49+22"\r\n<actual message> */
#define COMMAND_STATE_WAITING_FOR_1_PLUS        0
#define COMMAND_STATE_WAITING_FOR_C             1
#define COMMAND_STATE_WAITING_FOR_M             2
#define COMMAND_STATE_WAITING_FOR_T             3
#define COMMAND_STATE_WAITING_FOR_1_COLON       4      
#define COMMAND_STATE_WAITING_FOR_SPACE         5
#define COMMAND_STATE_WAITING_FOR_1_QUOTATION   6
#define COMMAND_STATE_WAITING_FOR_2_PLUS        7
#define COMMAND_STATE_WAITING_FOR_PHONE_NUMBER  8
#define COMMAND_STATE_WAITING_FOR_END_PH_NUMBER 9
#define COMMAND_STATE_WAITING_FOR_2_QUOTATION   10
#define COMMAND_STATE_WAITING_FOR_3_QUOTATION   11
#define COMMAND_STATE_WAITING_FOR_4_QUOTATION   12
#define COMMAND_STATE_WAITING_FOR_5_QUOTATION   13
#define COMMAND_STATE_WAITING_FOR_6_QUOTATION   14
#define COMMAND_STATE_WAITING_FOR_1_NEW_LINE    15
#define COMMAND_STATE_WAITING_FOR_CARRIAGE_RETURN 16
#define COMMAND_STATE_WAITING_FOR_1_DATA        17
#define COMMAND_STATE_WAITING_FOR_2_DATA        18
#define COMMAND_STATE_WAITING_FOR_3_DATA        19


void setup()
{
  short sRetVal = SUCCESS;
  sRetVal = Load_Default_Values(&g_Global_Handle);
  if(sRetVal != SUCCESS)
  {
    /*Failure_Handling()*/
  }

 // wdt_enable(WDTO_2S);
  
  // Setting the baud rate of GSM Module
  SIM900A.begin(9600);     
  
  // Setting the baud rate of Serial Monitor (Arduino)
  Serial.begin(9600);  
  delay(100);  
  
  Serial.println("SIM900A Ready");  
  Serial.println ("Type s to send message or r to receive message");  

  //Set the relay pin mode to output
  pinMode(g_Global_Handle.s_Relay.iRelayPin,OUTPUT);
  
  //sRetval = TimerConfiguration(g_Global_Handle.s_Timer.timer1_counter);
  if(sRetVal != SUCCESS)
  {
    /*Failure_Handling()*/
  }
  
}
void loop()
{
  short sRetVal = SUCCESS;
  
  sRetVal = RecieveMessage();
  if(sRetVal != SUCCESS)
  {
    /*Failure_Handling()*/
  }    
    
  //S_arrFixedPhNos s_arrFixedPhNos = {DEFAULT_VALUE};

  //main 24/7 started working here onwards
  
  while(1)
  {
    /* This should be enabled for manual testing*/
    /*if (Serial.available()>0)
     switch(Serial.read())
    {
      case 's':
        SendMessage();        
        break;
      /*case 'r':
        RecieveMessage();
        break;     
    }*/
        
    CommandRxAndFormation(&g_Global_Handle.s_Rx_Msg, &g_Global_Handle.s_Command_Processor); 
    if(( g_Global_Handle.s_Command_Processor.ucPowerOnCommandRx == 1 ) && ( g_Global_Handle.s_Command_Processor.ucCommandRxState == COMMAND_STATE_WAITING_FOR_1_PLUS))
    {
      Serial.println("command received\n");  
      digitalWrite(g_Global_Handle.s_Relay.iRelayPin,LOW);
      SendMessage(g_Global_Handle.s_Command_Processor.ucPowerOnCommandRx,g_Global_Handle.s_Command_Processor.ucPowerOffCommandRx);
      g_Global_Handle.s_Command_Processor.ucPowerOnCommandRx = 0;
      
    }
    if(( g_Global_Handle.s_Command_Processor.ucPowerOffCommandRx == 1 ) && ( g_Global_Handle.s_Command_Processor.ucCommandRxState == COMMAND_STATE_WAITING_FOR_1_PLUS))
    {
      digitalWrite(g_Global_Handle.s_Relay.iRelayPin,HIGH);
      SendMessage(g_Global_Handle.s_Command_Processor.ucPowerOnCommandRx,g_Global_Handle.s_Command_Processor.ucPowerOffCommandRx);
      g_Global_Handle.s_Command_Processor.ucPowerOffCommandRx = 0;
    }
   // wdt_reset();   
  }
     
}

short Load_Default_Values(G_Global_Handle *Out_g_Global_Handle)
{
  short sRetVal = SUCCESS;
  if(Out_g_Global_Handle == NULL)
  {
    sRetVal = LOAD_DEFAULT_NULL_HANDLING;
  }
  if(sRetVal == SUCCESS)
  {
    memset(Out_g_Global_Handle,0,sizeof(G_Global_Handle));
    
    memcpy(&Out_g_Global_Handle->s_arr_Fixed_PhNos[0].arrFixedPhNos[FIXED_PH_NO_INDEX_0],FIXED_PH_NO_0,GPS_RX_PH_NO_SIZE);
    memcpy(&Out_g_Global_Handle->s_arr_Fixed_PhNos[1].arrFixedPhNos[FIXED_PH_NO_INDEX_1],FIXED_PH_NO_1,GPS_RX_PH_NO_SIZE);
    memcpy(&Out_g_Global_Handle->s_arr_Fixed_PhNos[2].arrFixedPhNos[FIXED_PH_NO_INDEX_2],FIXED_PH_NO_2,GPS_RX_PH_NO_SIZE);
    memcpy(&Out_g_Global_Handle->s_arr_Fixed_PhNos[3].arrFixedPhNos[FIXED_PH_NO_INDEX_3],FIXED_PH_NO_3,GPS_RX_PH_NO_SIZE);
    memcpy(&Out_g_Global_Handle->s_arr_Fixed_PhNos[0].arrFixedReplyPhNo[FIXED_PH_NO_INDEX_0],FIXED_REPLY_NO,GPS_RX_PH_NO_SIZE);
  
    Out_g_Global_Handle->s_Relay.iRelayPin = RELAY_POWER_ON_PIN;
  
    Out_g_Global_Handle->s_Timer.timer1_counter = TIMER_COFIGURATION_VALUE;

    Out_g_Global_Handle->s_Command_Processor.ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_PLUS;
  }
  return sRetVal;
}

/*+CMT: "+917708720020","","22/07/02,23:38:49+22"\r\n<actual message>*/
short CommandRxAndFormation(S_Rx_Msg *Out_s_Rx_Msg ,S_Command_Processor *Out_s_Command_Processor)
{  
  short sRetVal = SUCCESS;

unsigned char uctmp =0 ;
  if((Out_s_Rx_Msg == NULL ) || (Out_s_Command_Processor == NULL))
  {
    sRetVal = COMMAND_PROCESSOR_ERROR_NULL_HANDLING;
  }

  if(sRetVal == SUCCESS)
  {
    if(SIM900A.available()>0)
    {
      Out_s_Command_Processor->ucTempChar =  SIM900A.read();
      //uctmp = SIM900A.read();
      //Serial.println("Available\n");
      Serial.write(Out_s_Command_Processor->ucTempChar);
      //Serial.write(uctmp);
    }
    else
    {
      return sRetVal;
    }    
    switch(Out_s_Command_Processor->ucCommandRxState)
    {
      Serial.println("inside switch\n");
      case COMMAND_STATE_WAITING_FOR_1_PLUS:              
        if( Out_s_Command_Processor->ucTempChar == '+' )
        {
          Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_C;
          Serial.println ("Plus\n");
        }
        else
        {
          Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_PLUS;
        }
        break;
      case COMMAND_STATE_WAITING_FOR_C:      
          if( Out_s_Command_Processor->ucTempChar == 'C' )
          {
            Serial.println ("cccc\n");
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_M;
          }
          else
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_PLUS;
          }      
        break;
      case COMMAND_STATE_WAITING_FOR_M:      
          if( Out_s_Command_Processor->ucTempChar == 'M' )
          {
            Serial.println ("mmmm\n");
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_T;
          }
          else
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_PLUS;
          }
        
        break;
      case COMMAND_STATE_WAITING_FOR_T:      
          if( Out_s_Command_Processor->ucTempChar == 'T' )
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_COLON;
            Serial.println ("ttttt\n");
          }
          else
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_PLUS;
          }
        
        break;
     case COMMAND_STATE_WAITING_FOR_1_COLON:      
          if( Out_s_Command_Processor->ucTempChar == ':' )
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_SPACE;
            Serial.println ("colunm\n");
          }
          else
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_PLUS;
          }      
        break;
  
     case COMMAND_STATE_WAITING_FOR_SPACE:      
          if( Out_s_Command_Processor->ucTempChar == ' ' )
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_QUOTATION;
            Serial.println ("space\n");
          }
          else
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_PLUS;
          }      
        break;
     case COMMAND_STATE_WAITING_FOR_1_QUOTATION:      
          if( Out_s_Command_Processor->ucTempChar == '"' )
          {
            Serial.println ("1st:\n");
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_2_PLUS;
          }             
        break;
      case COMMAND_STATE_WAITING_FOR_2_PLUS:      
          if( Out_s_Command_Processor->ucTempChar == '+' )
          {
            Serial.println ("1st:\n");
            Out_s_Rx_Msg->arrMsgRxPhNo[Out_s_Rx_Msg->uiMsgRxPhNoCount++] = Out_s_Command_Processor->ucTempChar;
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_2_QUOTATION;
          }              
        break;      
     case COMMAND_STATE_WAITING_FOR_2_QUOTATION:      
          if( Out_s_Command_Processor->ucTempChar == '"' )
          {
            Serial.println ("2st:\n");
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_3_QUOTATION;
          }
          else
          {
            Out_s_Rx_Msg->arrMsgRxPhNo[Out_s_Rx_Msg->uiMsgRxPhNoCount++] = Out_s_Command_Processor->ucTempChar; 
          }              
        break;
      case COMMAND_STATE_WAITING_FOR_3_QUOTATION:      
          if( Out_s_Command_Processor->ucTempChar == '"' )
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_4_QUOTATION;
            Serial.println ("3st:\n");
          }        
        break;
      case COMMAND_STATE_WAITING_FOR_4_QUOTATION:      
          if( Out_s_Command_Processor->ucTempChar == '"' )
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_5_QUOTATION;
            Serial.println ("4st:\n");
          }        
        break;
     case COMMAND_STATE_WAITING_FOR_5_QUOTATION:      
          if( Out_s_Command_Processor->ucTempChar == '"' )
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_6_QUOTATION;
            Serial.println ("5st:\n");
          }        
        break;
        
     case COMMAND_STATE_WAITING_FOR_6_QUOTATION:      
          if( Out_s_Command_Processor->ucTempChar == '"' )
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_CARRIAGE_RETURN;
            Serial.println ("6st:\n");
          }        
        break;
  
  
     case COMMAND_STATE_WAITING_FOR_CARRIAGE_RETURN:      
          if( Out_s_Command_Processor->ucTempChar == 13 )
          {                  
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_NEW_LINE;
            Serial.println ("carriage return\n");
          }
        break;
  
     case COMMAND_STATE_WAITING_FOR_1_NEW_LINE:      
          if( Out_s_Command_Processor->ucTempChar == 10 )
          {                  
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_DATA;
            Serial.println ("newline\n");
          }
       break;
  
     case COMMAND_STATE_WAITING_FOR_1_DATA:      
          if( Out_s_Command_Processor->ucTempChar == '@' )
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_2_DATA;
            Serial.println ("at:\n");
          }
          else
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_PLUS;
          }
        break;
     case COMMAND_STATE_WAITING_FOR_2_DATA:      
          if( Out_s_Command_Processor->ucTempChar == '#' )
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_3_DATA;
            Serial.println ("hash:\n");
          }
          else
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_PLUS;
          }
        break;
     case COMMAND_STATE_WAITING_FOR_3_DATA:      
          if( Out_s_Command_Processor->ucTempChar == 'T' )
          {
            Out_s_Command_Processor->ucPowerOnCommandRx = 1;
            Serial.println ("start:\n");
            
          }
          else if( Out_s_Command_Processor->ucTempChar == 'P' )
          {
            Serial.println ("stop:\n");
            Out_s_Command_Processor->ucPowerOffCommandRx = 1;          
          }
          else
          {
            Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_PLUS;
          }
          Serial.println ("ready for new command:\n");
          Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_PLUS;
        break;
        default:   
        Serial.println ("default:\n");
        Out_s_Command_Processor->ucCommandRxState = COMMAND_STATE_WAITING_FOR_1_PLUS;
        break;
    }
  }
  return sRetVal;
}

void SendMessage(unsigned char ucPowerOn, unsigned char ucPowerOff)
{
  Serial.println ("Sending Message");
  SIM900A.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(1000);
  Serial.println ("Set SMS Number");
  SIM900A.println("AT+CMGS=\"+916369939554\"\r"); //Mobile phone number to send message
  delay(1000);
  Serial.println ("Set SMS Content");
  if( ucPowerOn == 1)
  {
    SIM900A.println("Powered On");// Messsage content
  }
  if( ucPowerOff == 1)
  {
    SIM900A.println("Powered Off");// Messsage content
  }
  
  delay(100);
  Serial.println ("Finish");
  SIM900A.println((char)26);// ASCII code of CTRL+Z
  delay(1000);
  Serial.println ("Message has been sent ->SMS Selesai dikirim");
  RecieveMessage();
}

short RecieveMessage()
{
  short sRetVal = SUCCESS;
  
  Serial.println ("SIM900A Membaca SMS");
  delay (1000);
  
  SIM900A.println("AT+CNMI=2,2,0,0,0"); // AT Command to receive a live SMS
  delay(1000);
  
  Serial.write ("Unread Message done");
  
  return sRetVal;
 }

short TimerConfiguration(int In_timer1_counter)
{
  short sRetVal = SUCCESS;
  // initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;

  TCNT1 = In_timer1_counter;   
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
  interrupts();             // enable all interrupts

  return sRetVal;
}

ISR(TIMER1_OVF_vect)        // interrupt service routine 
{
  TCNT1 = g_Global_Handle.s_Timer.timer1_counter;   // preload timer  
  if (SIM900A.available()>0)
  {
    Serial.println("Interupt\n");    
  }
}



/********* Other function in feature may required *********/
#if 0

void SendMessage()
{

  Serial.println("inside send");
  mySerial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(1000);

  mySerial.println("AT+CMGF=?");    //Sets the GSM Module in Text Mode
  delay(1000);  

  mySerial.println("AT+CMGF?");    //Sets the GSM Module in Text Mode
  delay(1000);
 
  mySerial.println("AT+CMGS=\"7708720020\"\r"); // Replace x with mobile number  
  delay(1000);
  
  mySerial.println("sim900a sms");// The SMS text you want to send
  delay(100);
  
  mySerial.println((char)26);// ASCII code of CTRL+Z
  delay(1000);
}

void ReceiveMessage()
{
  char buf[256] = {0};
  char *bufp = NULL;
  mySerial.println("AT+CNMI=2,2,0,0,0"); // AT Command to recieve a live SMS
  delay(1000);
  if (mySerial.available()>0)
  {
    msg=mySerial.read();
    Serial.print(msg);
  }
  while(1)
  {
    if (mySerial.available()>0)
    {      
      /*mySerial.read();
      memcpy(buf,bufp,256);
      Serial.write(buf);*/
    }
    wdt_reset();
  }  
}

void MakeCall()
{
  mySerial.println("ATD9442804249;"); // ATDxxxxxxxxxx; -- watch out here for semicolon at the end!!
  Serial.println("Calling  "); // print response over serial port
  delay(1000);
}


void HangupCall()
{
  mySerial.println("ATH");
  Serial.println("Hangup Call");
  delay(1000);
}

void ReceiveCall()
{
  mySerial.println("ATA");
  delay(1000);
  {
    call=mySerial.read();
    Serial.print(call);
  }
}

void RedialCall()
{
  mySerial.println("ATDL");
  Serial.println("Redialing");
  delay(1000);
}

#endif
