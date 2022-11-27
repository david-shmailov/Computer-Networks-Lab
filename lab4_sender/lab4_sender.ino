// C code
//
#include "EthernetLab.h"
#define TX_PIN 2
#define RX_PIN 3
#define CLK_OUT_PIN 4
#define CLK_IN_PIN 5


#define with_error 0
#define num_of_errors 1

//L2
#define DATA_SIZE 13



// usart_tx global variables
#define BIT_TIME          20
#define half_BIT_TIME     BIT_TIME >> 1
#define wait_max          100*BIT_TIME
#define wait_min          10*BIT_TIME
#define buffer_size       8

int sender=1;


typedef struct Frame{
  uint8_t destination_adress;
  uint8_t source_adress;
  uint8_t frame_type;
  uint8_t length;
  uint8_t* payload;
  uint32_t crc;
} Frame;
Frame F1;
int rx_l2_count = 0;
Frame Rx_frame;


typedef enum {ACTIVE, PASSIVE} state_type;
unsigned long start_time = 0;
unsigned long curr_time = 0;
unsigned long delta_t;
int l1_tx_buffer;
int l1_tx_count = 0;
long l1_tx_wait = 0;
int l1_tx_bit;
state_type l1_tx_state = PASSIVE;


// usart_Rx global variables
int layer_1_rx_busy =0;
int clk_in_prev = 0;
int clk_in_curr = 0;
int l1_rx_bit;
int l1_rx_buffer = 0;
int l1_rx_count = 0; 

//layer2_tx global variables
int layer_1_tx_busy = 0;
int layer_2_tx_request =0 ;
typedef enum {FIRST, SECOND} l2_state_type;
l2_state_type half_state = FIRST;
int layer2_tx_buffer_counter = 0;
char l2_tx_buff [DATA_SIZE] = "DAVID_NERIYA";


// L2 RX global variables
typedef enum {Recieve, Check, Ack_Send,Idle} Rx_type;
Rx_type l2_rx_state = Recieve;
int rx_payload_counter = 0;
int rx_crc_counter = 0;
int layer_1_rx_busy_prev;
int l2_rx_counter = 0;
int l2_buffer_pos;
char l2_rx_buff [DATA_SIZE];
int rx_error_flag=0;



void usart_tx(){
  curr_time = millis();
  delta_t = curr_time - start_time;
  switch(l1_tx_state){
    case ACTIVE:
   		if (delta_t >= BIT_TIME){
          digitalWrite(CLK_OUT_PIN,HIGH);
          l1_tx_bit = bitRead(l1_tx_buffer, l1_tx_count);
          digitalWrite(TX_PIN,l1_tx_bit);
          l1_tx_count ++;
          start_time = millis();
          if (l1_tx_count == buffer_size){
            l1_tx_wait = random(wait_min , wait_max);
            l1_tx_state = PASSIVE;
            l1_tx_count = 0;
          }
        } else if (delta_t >= half_BIT_TIME){
          digitalWrite(CLK_OUT_PIN,LOW);
        }
        break;
    case PASSIVE:
      if (delta_t > l1_tx_wait)
        layer_1_tx_busy = 0;
        if(  layer_2_tx_request == 1){
          l1_tx_state = ACTIVE;
          start_time = millis();
          layer_2_tx_request = 0;
          layer_1_tx_busy = 1;
      }
      break;
  }
}

void usart_rx(){
  clk_in_curr = digitalRead(CLK_IN_PIN);
  if (clk_in_prev > clk_in_curr){
    layer_1_rx_busy = 1;
    l1_rx_bit = digitalRead(RX_PIN);
    bitWrite(l1_rx_buffer, l1_rx_count, l1_rx_bit);
    l1_rx_count ++;
    if (l1_rx_count == buffer_size){
      l1_rx_count = 0;
      layer_1_rx_busy = 0;
    }
  }
  clk_in_prev = clk_in_curr;
  
}
  

void link_layer_tx(){
  
}


void link_layer_rx(){
  switch(l2_rx_state){
    case Recieve: //save each segment in the designated space in Rx_frame
      if(!layer_1_rx_busy){
        if(l2_rx_counter==0){//destination adress
          Rx_frame.destination_adress=l1_rx_buffer;
          l2_rx_counter++;
        }
        else if (l2_rx_counter==1)//source adress
        {
          Rx_frame.source_adress=l1_rx_buffer;
          l2_rx_counter++;
        }
        else if (l2_rx_counter==2)//frame type
        {
          Rx_frame.frame_type=l1_rx_buffer;
          l2_rx_counter++;
        }
        else if (l2_rx_counter==3)//payload length
        {
          Rx_frame.length=l1_rx_buffer;
          l2_rx_counter++;
        }
        else if ((l2_rx_counter<4+Rx_frame.length)&&(l2_rx_counter>=4))//payload
        {
          Rx_frame.payload[rx_payload_counter]=l1_rx_buffer;
          rx_payload_counter++;
          l2_rx_counter++;
        }
        else if (l2_rx_counter>= 4 + Rx_frame.length)//CRC
        {
          if (rx_crc_counter<4){
            Rx_frame.crc=l1_rx_buffer;
            Rx_frame.crc<<8;
            l2_rx_counter++;
            rx_crc_counter++;
          }
        }
        
      }
      

  }
  

}


void setup()
{
  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT);
  pinMode(CLK_OUT_PIN, OUTPUT);
  pinMode(CLK_IN_PIN, INPUT);
  Serial.begin(9600);
  if (sender){
    F1.destination_adress=0x16;
    F1.source_adress=0x6;
  }
  else{
    F1.destination_adress=0x6;
    F1.source_adress=0x16;
  }
}

void loop()
{
  link_layer_tx();
  link_layer_rx();
  usart_tx();
  usart_rx();
}







