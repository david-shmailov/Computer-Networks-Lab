// C code
//
#include "EthernetLab.h"
#include "lab4.h"
#define TX_PIN 2
#define RX_PIN 3
#define CLK_OUT_PIN 4
#define CLK_IN_PIN 5


#define with_error 0
#define num_of_errors 1

//L2
#define MAX_FRAME_SIZE 100
#define FRAME_HEADER_SIZE 4
#define CRC_SIZE 4


// usart_tx global variables
#define BIT_TIME          20
#define half_BIT_TIME     BIT_TIME >> 1
#define wait_max          100*BIT_TIME
#define wait_min          10*BIT_TIME
#define buffer_size       8

struct Frame{
  uint8_t destination_address;
  uint8_t source_address;
  uint8_t frame_type;
  uint8_t length;
  uint8_t* payload;
  uint32_t crc;
};

int rx_l2_count = 0;
struct Frame TX_frame;
struct Frame RX_frame;
int sender=1;

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
typedef enum {TRANSMITTING, IDLE} l2_state_type;
typedef enum {DST_ADDR, SRC_ADDR, TYPE, LENGTH, PAYLOAD, CRC} l2_frame_state;
l2_state_type l2_tx_state = IDLE;
l2_frame_state l2_tx_frame_state = DST_ADDR;
int layer2_tx_counter = 0;

uint8_t array2send[MAX_FRAME_SIZE];
int L2_build_counter;
int frame_size;
int shift;
char *payload = "DAVID_NERIYA";


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

void build_tx_frame(){
    TX_frame.destination_address = 0x06;
    TX_frame.source_address = 0x16;
    TX_frame.frame_type = 0x00;
    TX_frame.length = strlen(payload);
    
    TX_frame.payload = payload;
    build_array2send();
}


void build_array2send(){
    for (L2_build_counter = 0; L2_build_counter < FRAME_HEADER_SIZE; L2_build_counter++){
      array2send[L2_build_counter] = *((uint8_t *) &TX_frame + L2_build_counter);
    }
    for (L2_build_counter = 0; L2_build_counter < TX_frame.length; L2_build_counter++){
      array2send[L2_build_counter + FRAME_HEADER_SIZE] = TX_frame.payload[L2_build_counter];
    }
    // calculate CRC
    frame_size = FRAME_HEADER_SIZE + TX_frame.length;
    TX_frame.crc = calculateCRC(array2send, frame_size);
    // add CRC to array2send
    for (L2_build_counter = 0; L2_build_counter<CRC_SIZE; L2_build_counter++){
        shift = 24 - L2_build_counter*8;
        array2send[frame_size+L2_build_counter] = (TX_frame.crc >> shift ) & 0xFF; // take the byte from the crc
    }
    // update frame size
    frame_size += CRC_SIZE;
}



void link_layer_tx(){
    switch (l2_tx_state)
    {
        case IDLE:
            /* code */
            break;
        case TRANSMITTING:
            if (!layer_1_tx_busy && !layer_2_tx_request){
                if (layer2_tx_counter < frame_size){
                    l1_tx_buffer = array2send[layer2_tx_counter];
                    layer2_tx_counter++;
                    layer_2_tx_request = 1;
                } else {
                    l2_tx_state = IDLE;
                    layer2_tx_counter = 0;
                }
            }
            break;
        default:
            break;
    }
}


void link_layer_rx(){
    
}


void setup()
{
  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT);
  pinMode(CLK_OUT_PIN, OUTPUT);
  pinMode(CLK_IN_PIN, INPUT);
  Serial.begin(115200);
//   if (sender){
//     F1.destination_adress=0x16;
//     F1.source_adress=0x6;
//   }
//   else{
//     F1.destination_adress=0x6;
//     F1.source_adress=0x16;
//   }
}

void loop()
{
//   link_layer_tx();
//   link_layer_rx();
//   usart_tx();
//   usart_rx();
    build_tx_frame();
    Serial.print("TX ARRAY: \n");
    for (int i=0; i<frame_size; i++){
        Serial.print( array2send[i]);
        Serial.print(" ");
    }
    Serial.print("\n");
    Serial.print("TX CRC: ");
    Serial.print(TX_frame.crc, HEX);
    Serial.print("\n");
    // uint32_t crc = calculateCRC(array2send, frame_size-CRC_SIZE);
    // Serial.print("RX ARRAY: \n");
    // for (int i=0; i<frame_size; i++){
    //     Serial.print(array2send[i], HEX);
    //     Serial.print(" ");
    // }
    // Serial.print("\n");
    // Serial.print("RX CRC: ");
    // Serial.print(crc, HEX);
    // Serial.print("\n");
}







