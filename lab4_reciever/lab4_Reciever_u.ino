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
char * names = "David_Neriya";
char *Ack_num;
int layer_1_tx_busy = 0;
int layer_2_tx_request =0 ;
typedef enum {L2_IDLE,TRANSMITTING,} l2_state_type;
typedef enum {DST_ADDR, SRC_ADDR, TYPE, LENGTH, PAYLOAD, CRC} l2_frame_state;
l2_state_type l2_tx_state = L2_IDLE;
l2_frame_state l2_tx_frame_state = DST_ADDR;
int layer2_tx_counter = 0;

uint8_t array2send[MAX_FRAME_SIZE];
int L2_build_counter;
int frame_size;
int shift;
char payload_array [MAX_FRAME_SIZE-FRAME_HEADER_SIZE-CRC_SIZE];
uint8_t l2_frame_num = 0;
char *payload;
char *Ack_payload;

// L2 RX global variables
typedef enum {L2_WAIT, GOOD_FRAME, BAD_FRAME} l2_rx_received_frame_type;
l2_rx_received_frame_type l2_rx_received_frame = L2_WAIT;
typedef enum {Recieve, Check, Ack_Send,Idle} Rx_type;
Rx_type l2_rx_state = Recieve;
int rx_payload_counter = 0;
int rx_crc_counter = 0;
int layer_1_rx_busy_prev;
int l2_rx_counter = 0;
int l2_buffer_pos;
char l2_rx_buff [MAX_FRAME_SIZE];
int l2_frame_corrupted=0;
int l2_frame_ack;
uint8_t complete_array[MAX_FRAME_SIZE];
char ACK_P = 0x6;


// L2 stop and wait global variables Reciever
int l2_ack_counter = 0;
typedef enum {IDLE,WAIT} SNW_state_type;

SNW_state_type l2_snw_state = IDLE;
int num_of_frame = 0;
int num_bad_frame = 0;


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

//void build_tx_frame(){
  //TX_frame.destination_address = 0x06;
  //TX_frame.source_address = 0x16;
  //TX_frame.frame_type = 0x00;
  //TX_frame.length = strlen(payload);
  //TX_frame.payload = payload_array;
  //build_array2send();
//}
void build_Ack_frame(){
  TX_frame.destination_address = 0x16;
  TX_frame.source_address = 0x06;
  TX_frame.frame_type = 0x00;
  TX_frame.length = strlen(Ack_payload);
  TX_frame.payload = Ack_payload;
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

int test_RX_frame(){
    for (L2_build_counter = 0; L2_build_counter < FRAME_HEADER_SIZE; L2_build_counter++){
      complete_array[L2_build_counter] = *((uint8_t *) &RX_frame + L2_build_counter);
    }
    for (L2_build_counter = 0; L2_build_counter < RX_frame.length; L2_build_counter++){
      complete_array[L2_build_counter + FRAME_HEADER_SIZE] = RX_frame.payload[L2_build_counter];
    }
    // calculate CRC
    frame_size = FRAME_HEADER_SIZE + RX_frame.length;
    uint32_t crc = calculateCRC(complete_array, frame_size);
    if (crc != RX_frame.crc){
      return 0;
    }else {
      return 1;
    }
    
}

void link_layer_tx(){
    switch (l2_tx_state){
      case L2_IDLE:
          /* code */
          break;
      case TRANSMITTING:
          if (!layer_1_tx_busy && !layer_2_tx_request){
              if (layer2_tx_counter < frame_size){
                  l1_tx_buffer = array2send[layer2_tx_counter];
                  layer2_tx_counter++;
                  layer_2_tx_request = 1;
              } else {
                  l2_tx_state = L2_IDLE;
                  layer2_tx_counter = 0;
              }
          }
          break;
      default:
          break;
    }
}

void link_layer_rx(){
    switch(l2_rx_state){//
    case Recieve: //save each segment in the designated space in RX_frame
      if(layer_1_rx_busy < layer_1_rx_busy_prev){
        if(l2_rx_counter==0){//destination adress
          RX_frame.destination_address=l1_rx_buffer;
          l2_rx_counter++;
        }
        else if (l2_rx_counter==1)//source adress
        {
          RX_frame.source_address=l1_rx_buffer;
          l2_rx_counter++;
        }
        else if (l2_rx_counter==2)//frame type
        {
          RX_frame.frame_type=l1_rx_buffer;
          l2_rx_counter++;
        }
        else if (l2_rx_counter==3)//payload length
        {
          RX_frame.length=l1_rx_buffer;
          l2_rx_counter++;
        }
        else if ((l2_rx_counter<FRAME_HEADER_SIZE+RX_frame.length)&&(l2_rx_counter>=FRAME_HEADER_SIZE))//payload
        {
          RX_frame.payload[rx_payload_counter]=l1_rx_buffer;
          rx_payload_counter++;
          l2_rx_counter++;
        }
        else if ((l2_rx_counter>= FRAME_HEADER_SIZE + RX_frame.length)&&(l2_rx_counter<RX_frame.length + FRAME_HEADER_SIZE + CRC_SIZE))//CRC
        {
          if (rx_crc_counter<CRC_SIZE){
            RX_frame.crc=l1_rx_buffer;
            RX_frame.crc<<8;
            l2_rx_counter++;
            rx_crc_counter++;
          }
        }
        else if (l2_rx_counter==FRAME_HEADER_SIZE + RX_frame.length + CRC_SIZE){//EXIT
          l2_rx_state = Check;
          l2_rx_counter = 0;
          rx_crc_counter = 0;
          rx_payload_counter = 0;
        }

        layer_1_rx_busy_prev = layer_1_rx_busy;
      }
      break;
    case Check:
      num_of_frame++;
      if (test_RX_frame()){
        l2_rx_received_frame = GOOD_FRAME;
        l2_frame_corrupted = 0;
        l2_rx_state = Idle;
      } else {
        l2_rx_received_frame = BAD_FRAME;
        l2_frame_corrupted = 1;
        l2_rx_state = Idle;
        num_bad_frame++;
      }
      break;

    case Idle:
      if(layer_1_rx_busy){
        l2_rx_state = Recieve;
      }
      break;
  }
}

void unpack_payload(){
    l2_ack_counter = RX_frame.payload[0]; // first byte of payload is the ack counter
    uint8_t l2_ack = RX_frame.payload[1]; // second byte of payload is the ack
    if (l2_ack == ACK_P){
        Serial.print("Received ACK: ");
        Serial.println(l2_ack_counter);
    }
}
void pack_payload(){
    int len = strlen(names);
    payload_array[0] = l2_frame_num;
    for (int i = 0; i < len; i++){
        payload_array[i+1] = names[i];
    }
    build_tx_frame();
    Serial.println("Sending frame number: ");
    Serial.println(l2_frame_num);
    // // for debug:
    // Serial.print(TX_frame.payload[0], HEX);
    // for (int i = 0; i < len; i++){
    //     Serial.print((char)TX_frame.payload[i+1]);
    //     // Serial.print(names[i]);
    // }
    // Serial.println("\nend");
    
}
void pack_Ack_payload(){
    //int len = strlen(names);
    payload_array[0] = l2_ack_counter;
    //for (int i = 0; i < len; i++){
    //  payload_array[i+1] = names[i];
  //}
    build_Ack_frame();
    Serial.println("Sending frame number: ");
    Serial.println(l2_frame_num);
    // // for debug:
    // Serial.print(TX_frame.payload[0], HEX);
    // for (int i = 0; i < len; i++){
    //     Serial.print((char)TX_frame.payload[i+1]);
    //     // Serial.print(names[i]);
    // }
    // Serial.println("\nend");
    
}
void stop_and_wait(){
  if (l2_snw_state == WAIT){
      //waiting for ACK
      if (l2_rx_received_frame == GOOD_FRAME){
        unpack_payload();
        l2_frame_num++;
        pack_payload();
        build_Ack_frame();
        l2_tx_state = TRANSMITTING;
       // if ack num not match, ignore ack
      }// if frame is bad frame, ignore the ack
      l2_rx_received_frame = L2_WAIT; // confirm that l2_rx_received_frame has been read
  }
  else if (l2_snw_state == IDLE){
    // do nothing
  }else{
    // do nothing
  }
   
}

void setup()
{
  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT);
  pinMode(CLK_OUT_PIN, OUTPUT);
  pinMode(CLK_IN_PIN, INPUT);
  Serial.begin(115200);
  pack_Ack_payload(); // debug
}

void loop()
{ 
  
  // for (int i = 0; i < 15000; i++){
  //   print("s");
  // }
 link_layer_tx();
 link_layer_rx();
 usart_tx();
 usart_rx();
    
}