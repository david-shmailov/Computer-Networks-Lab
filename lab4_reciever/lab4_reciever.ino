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
#define CRC_SIZE 4 //4 byte
#define FRAME_HEADER_SIZE 4
#define MAX_FRAME_SIZE 100
char ACK_P = 0x6;
char* ACK =&ACK_P;


// usart_tx global variables
#define BIT_TIME          20
#define half_BIT_TIME     BIT_TIME >> 1
#define wait_max          100*BIT_TIME
#define wait_min          10*BIT_TIME
#define buffer_size       8

int sender=1;


struct Frame{
  uint8_t destination_adress;
  uint8_t source_adress;
  uint8_t frame_type;
  uint8_t length;
  uint8_t* payload;
  uint32_t crc;
};
struct Frame F1;
int rx_l2_count = 0;
struct Frame Rx_frame;
struct Frame Tx_frame;
uint8_t ACK_frame[MAX_FRAME_SIZE];

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
int num_bad_frame = 0;
int num_of_frame = 0;
int layer_1_tx_busy = 0;
int layer_2_tx_request =0 ;
typedef enum {FIRST, SECOND} l2_state_type;
l2_state_type half_state = FIRST;
int layer2_tx_buffer_counter = 0;
char l2_tx_buff [DATA_SIZE] = "DAVID_NERIYA";
typedef enum {TRANSMITTING, IDLE} l2_state_type;
typedef enum {DST_ADDR, SRC_ADDR, TYPE, LENGTH, PAYLOAD, CRC} l2_frame_state;
l2_state_type l2_tx_state = IDLE;
l2_frame_state l2_tx_frame_state = DST_ADDR;
int layer2_tx_counter = 0;
char *payload = "DAVID_NERIYA";
uint8_t array2send[MAX_FRAME_SIZE];
int L2_build_counter;
int frame_size;
int shift;




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
void build_ACK_frame(){
    ACK_frame.destination_address = 0x16;
    ACK_frame.source_address = 0x06;
    ACK_frame.frame_type = 0x00;
    ACK_frame.length = 1;
    ACK_frame.payload = ACK;
    build_array2send();
}
void build_array2send(){
    for (L2_build_counter = 0; L2_build_counter < FRAME_HEADER_SIZE; L2_build_counter++){
      array2send[L2_build_counter] = *((uint8_t *) &ACK_frame + L2_build_counter);
    }
    for (L2_build_counter = 0; L2_build_counter < ACK_frame.length; L2_build_counter++){
      array2send[L2_build_counter + FRAME_HEADER_SIZE] = ACK_frame.payload[L2_build_counter];
    }
    // calculate CRC
    frame_size = FRAME_HEADER_SIZE + ACK_frame.length;
    ACK_frame.crc = calculateCRC(array2send, frame_size-1);
    // add CRC to array2send
    for (L2_build_counter = 0; L2_build_counter<CRC_SIZE; L2_build_counter++){
        shift = 24 - L2_build_counter*8;
        array2send[frame_size+L2_build_counter] = (ACK_frame.crc >> shift ) & 0xFF; // take the byte from the crc
    }
    // update frame size
    frame_size += CRC_SIZE;
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
        else if ((l2_rx_counter<FRAME_HEADER_SIZE+Rx_frame.length)&&(l2_rx_counter>=FRAME_HEADER_SIZE))//payload
        {
          Rx_frame.payload[rx_payload_counter]=l1_rx_buffer;
          rx_payload_counter++;
          l2_rx_counter++;
        }
        else if ((l2_rx_counter>= FRAME_HEADER_SIZE + Rx_frame.length)&&(l2_rx_counter<Rx_frame.length + FRAME_HEADER_SIZE + CRC_SIZE))//CRC
        {
          if (rx_crc_counter<CRC_SIZE){
            Rx_frame.crc=l1_rx_buffer;
            Rx_frame.crc<<8;
            l2_rx_counter++;
            rx_crc_counter++;
          }
        }
        else if (l2_rx_counter==FRAME_HEADER_SIZE + Rx_frame.length + CRC_SIZE){//EXIT
          l2_rx_state = Check;
          l2_rx_counter = 0;
          rx_crc_counter = 0;
          rx_payload_counter = 0;
        }

        
      }
      
    case Check:
      uint8_t complete_array[100];
      complete_array[0] = Rx_frame.destination_adress;
      complete_array[1] = Rx_frame.source_adress;
      complete_array[2] = Rx_frame.frame_type;
      complete_array[3] = Rx_frame.length;
      for(int i = 0; i < Rx_frame.length; i++){
        complete_array[i+4] = Rx_frame.payload[i];
      }
      for(int i = 0; i < CRC_SIZE; i++){
        int shift = 24 - 8*i ;
        complete_array[i+FRAME_HEADER_SIZE+Rx_frame.length] = Rx_frame.crc>>(shift);
      }
      int32_t Res = calculateCRC(complete_array, FRAME_HEADER_SIZE + Rx_frame.length + CRC_SIZE);
      num_of_frame++;
      if (Res == 0){

      }
      else{
        num_bad_frame++;
      }
    case Ack_Send:
      // kaki
    case Idle:
      if(layer_1_rx_busy){
        l2_rx_state = Recieve;
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
  build_ACK_frame()
  
}

void loop()
{
  link_layer_tx();
  link_layer_rx();
  usart_tx();
  usart_rx();
}

