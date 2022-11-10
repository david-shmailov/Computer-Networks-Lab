// C code
//
#define TX_PIN 2
#define RX_PIN 3
#define CLK_OUT_PIN 4
#define CLK_IN_PIN 5
#define L2_BUFFER_SIZE 16
#define L2_COUNT_LIMIT L2_BUFFER_SIZE<<1

#define P1_bit 6
#define P2_bit 5
#define D1_bit 4
#define P3_bit 3
#define D2_bit 2
#define D3_bit 1
#define D4_bit 0


// usart_tx global variables
const int BIT_TIME = 20;
const int half_BIT_TIME = BIT_TIME >> 1;
const int wait_max = 100*BIT_TIME;
const int buffer_size = 8;

typedef enum {ACTIVE, PASSIVE} state_type;
unsigned long start_time = 0;
unsigned long curr_time = 0;
unsigned long delta_t;
int tx_buff = 0b01010101;
int tx_count = 0;
long wait;
int tx_bit;
state_type state = ACTIVE;

// usart_Rx global variables
int clk_in_prev = 0;
int clk_in_curr = 0;
int rx_bit;
int rx_buff = 0;
int rx_count = 0; 

//layer2_tx global variables
typedef enum {FIRST, SECOND} l2_state_type;
l2_state_type half_state = FIRST;
int tx_busy = 1;
int layer2_tx_buffer_counter = 0;
char l2_tx_buff [L2_BUFFER_SIZE] = "DAVIDNERIYA";


// L2 RX global variables
int layer_1_rx_busy;
int layer_1_rx_busy_prev;
int l2_rx_counter = 0;
int l2_buffer_pos;
char l2_rx_buff [L2_BUFFER_SIZE];
char half_byte_lsb;
char half_byte_msb;

int S1;
int S2;
int S3;

void usart_tx(){
  curr_time = millis();
  delta_t = curr_time - start_time;
  switch(state){
    case ACTIVE:
   		if (delta_t >= BIT_TIME){
          digitalWrite(CLK_OUT_PIN,HIGH);
          tx_bit = bitRead(tx_buff, tx_count);
          digitalWrite(TX_PIN,tx_bit);
          tx_count ++;
          start_time = millis();
          if (tx_count == buffer_size){
            wait = random(wait_max);
            state = PASSIVE;
            tx_count = 0;
          }
        } else if (delta_t >= half_BIT_TIME){
          digitalWrite(CLK_OUT_PIN,LOW);
        }
        break;
    case PASSIVE:
      if (delta_t > wait){
        state = ACTIVE;
        start_time = millis();
      }
      break;
  }
}

void usart_rx(){
  clk_in_curr = digitalRead(CLK_IN_PIN);
  if (clk_in_prev > clk_in_curr){
    rx_bit = digitalRead(RX_PIN);
    bitWrite(rx_buff, rx_count, rx_bit);
    rx_count ++;
    clk_in_prev = clk_in_curr;
    if (rx_count == buffer_size){
      rx_count = 0;
    }
  }else{
    clk_in_prev = clk_in_curr;
  }
}
  

void link_layer_tx(){
  
}

void link_layer_rx(){

  if (layer_1_rx_busy < layer_1_rx_busy_prev){ // falling edge of rx_busy flag
    if (l2_rx_counter % 2 == 0){ // even number of bits
      half_byte_lsb = Hamming47_rx();
    }else{ // odd number of bits
      half_byte_msb = Hamming47_rx();
      l2_buffer_pos = l2_rx_counter >> 1;
      l2_rx_buff[l2_buffer_pos] = (half_byte_msb << 4) | half_byte_lsb;
      Serial.println("Received char: %c", l2_rx_buff[l2_buffer_pos]);
    }
    l2_rx_counter ++;
    if (l2_rx_counter == L2_COUNT_LIMIT){
      l2_rx_counter = 0;
      Serial.println("Received String: %s", l2_rx_buff);
    }
  }
  layer_1_rx_busy_prev = layer_1_rx_busy; 
}

void encode_hamming(){

}

char Hamming47_rx(){
  Serial.println("Recieved byte: %x", rx_buff);
  S1 = bitRead(rx_buff, P1_bit) ^ bitRead(rx_buff, D1_bit) ^ bitRead(rx_buff, D2_bit) ^ bitRead(rx_buff, D4_bit);
  S2 = bitRead(rx_buff, P2_bit) ^ bitRead(rx_buff, D1_bit) ^ bitRead(rx_buff, D3_bit) ^ bitRead(rx_buff, D4_bit);
  S3 = bitRead(rx_buff, P3_bit) ^ bitRead(rx_buff, D2_bit) ^ bitRead(rx_buff, D3_bit) ^ bitRead(rx_buff, D4_bit);
  int S = (S3 << 2) | (S2 << 1) | S1;
  if (S>0){
    int index = 7-(S);
    rx_buff = rx_buff ^ (1 << index);
    Serial.println("Fixed byte: %x", rx_buff);
  }
  char data = (bitRead(rx_buff, D1_bit) << 3) | (bitRead(rx_buff, D2_bit) << 2) | (bitRead(rx_buff, D3_bit) << 1) | bitRead(rx_buff, D4_bit);
  return data;
}

void setup()
{
  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT);
  pinMode(CLK_OUT_PIN, OUTPUT);
  pinMode(CLK_IN_PIN, INPUT);
}

void loop()
{
  usart_tx();
  usart_rx();
}


