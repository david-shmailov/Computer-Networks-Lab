// C code
//
#include "EthernetLab.h"
#define TX_PIN 2
#define RX_PIN 3
#define CLK_OUT_PIN 4
#define CLK_IN_PIN 5
#define L2_BUFFER_SIZE 13 
#define L2_COUNT_LIMIT L2_BUFFER_SIZE<<1
#define P1_bit 6
#define P2_bit 5
#define D1_bit 4
#define P3_bit 3
#define D2_bit 2
#define D3_bit 1
#define D4_bit 0
#define polynom 0b100110000000
#define poly_degree  4

#define with_error 0
#define num_of_errors 1

// usart_tx global variables
#define BIT_TIME          20
#define half_BIT_TIME     BIT_TIME >> 1
#define wait_max          100*BIT_TIME
#define wait_min          10*BIT_TIME
#define buffer_size       12
#define data_size         buffer_size - poly_degree

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
char l2_tx_buff [L2_BUFFER_SIZE] = "DAVID_NERIYA";
int tx_byte_crc = 0;


// L2 RX global variables
int layer_1_rx_busy_prev;
int l2_rx_counter = 0;
int l2_buffer_pos;
char l2_rx_buff [L2_BUFFER_SIZE];
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
  if (!layer_1_tx_busy && !layer_2_tx_request){
    if (layer2_tx_buffer_counter<=L2_BUFFER_SIZE){
      tx_byte_crc = l2_tx_buff[layer2_tx_buffer_counter];
      l1_tx_buffer = CRC4_tx();
      layer2_tx_buffer_counter++;
      layer_2_tx_request=1;
    }
  }
}


void link_layer_rx(){
  if (layer_1_rx_busy < layer_1_rx_busy_prev){
    if (l2_rx_counter < 12){
      char L2_rx_temp = CRC4_rx();
      if (rx_error_flag){
        Serial.print("Received char: ");
        Serial.print(L2_rx_temp);
        Serial.print("\n");
        Serial.print("CRC ERROR #");
        Serial.print(l2_rx_counter);
        Serial.print("\n");
        rx_error_flag=0;
      }else{
        Serial.print("Received char: ");
        Serial.print(L2_rx_temp);
        Serial.print("\n");
        Serial.print("CRC Pass #");
        Serial.print(l2_rx_counter);
        Serial.print("\n");
      }
      l2_rx_buff[l2_rx_counter]=L2_rx_temp;
      l2_rx_counter++;
    }
    else{
      Serial.print("The Recieved string is: ");
      Serial.print(l2_rx_buff);
      Serial.print("\n");
    }
  }
  layer_1_rx_busy_prev = layer_1_rx_busy; 
}

int CRC4_tx(){
  int tx_crc_temp = tx_byte_crc;
  int dividend = tx_byte_crc;
  tx_crc_temp = tx_crc_temp << poly_degree;
  int tx_denom = polynom;
  for (int crc_loop_count = 0; dividend != 0 ;crc_loop_count++){
    if (bitRead(tx_crc_temp, buffer_size -1 -crc_loop_count)==1){
      tx_crc_temp = tx_crc_temp^tx_denom;
      dividend = tx_crc_temp >> poly_degree;
      tx_denom = tx_denom >> 1;
    }
    else{
      tx_denom = tx_denom >> 1;
    }

  }
  int result = (tx_byte_crc << poly_degree) | tx_crc_temp;
  if (with_error){
    int errors = 0;
    for (int i = 0; i < num_of_errors; i++){
      int bit_to_flip = random(0,buffer_size-1);
      int current_bit = bitRead(errors,bit_to_flip);
      while (current_bit == 1){
        bit_to_flip = random(0,buffer_size-1);
        current_bit = bitRead(errors,bit_to_flip);
      }
      bitSet(errors, bit_to_flip);
    }
    result = result ^ errors;
  }
  return result;
}

char CRC4_rx(){
  int rx_crc_temp = l1_rx_buffer;
  int dividend = l1_rx_buffer >> poly_degree;
  int rx_denom = polynom;
  for (int crc_loop_count = 0; dividend != 0;crc_loop_count++){
    if (bitRead(rx_crc_temp,11-crc_loop_count)==1){
      rx_crc_temp = rx_crc_temp^rx_denom;
      dividend = rx_crc_temp >> poly_degree;
      rx_denom = rx_denom>>1;
    }
    else{
      rx_denom = rx_denom >>1;
    }
  }
  if (rx_crc_temp != 0){
    rx_error_flag=1;
  }
  return (l1_rx_buffer >> poly_degree);
}


void setup()
{
  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT);
  pinMode(CLK_OUT_PIN, OUTPUT);
  pinMode(CLK_IN_PIN, INPUT);
  Serial.begin(9600);
}

void loop()
{
  link_layer_tx();
  link_layer_rx();
  usart_tx();
  usart_rx();
}







