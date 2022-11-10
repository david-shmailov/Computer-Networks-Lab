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
int l1_tx_buffer = 0b01010101;
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
int layer_2_tx_request = 0;
int layer2_tx_buffer_counter = 0;
char l2_tx_buff [L2_BUFFER_SIZE] = "DAVID_NERIYA";
int tx_hamming_counter =0;
int tx_byte_hamming = 0;
int P1= 0;
int P2= 0;
int P3= 0;


// L2 RX global variables
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
  switch(l1_tx_state){
    case ACTIVE:
   		if (delta_t >= BIT_TIME){
          digitalWrite(CLK_OUT_PIN,HIGH);
          l1_tx_bit = bitRead(l1_tx_buffer, l1_tx_count);
          digitalWrite(TX_PIN,l1_tx_bit);
          l1_tx_count ++;
          start_time = millis();
          if (l1_tx_count == buffer_size){
            l1_tx_wait = random(wait_max);
            l1_tx_state = PASSIVE;
            l1_tx_count = 0;
            layer_1_rx_busy = 0;
          }
        } else if (delta_t >= half_BIT_TIME){
          digitalWrite(CLK_OUT_PIN,LOW);
        }
        break;
    case PASSIVE:
      if (delta_t > l1_tx_wait && layer_2_tx_request == 1){
        l1_tx_state = ACTIVE;
        start_time = millis();
        layer_2_tx_request = 0;
        layer_1_rx_busy = 1;
      }
      break;
  }
}

void usart_rx(){
  clk_in_curr = digitalRead(CLK_IN_PIN);
  if (clk_in_prev > clk_in_curr){
    l1_rx_bit = digitalRead(RX_PIN);
    bitWrite(l1_rx_buffer, l1_rx_count, l1_rx_bit);
    l1_rx_count ++;
    clk_in_prev = clk_in_curr;
    if (l1_rx_count == buffer_size){
      l1_rx_count = 0;
    }
  }else{
    clk_in_prev = clk_in_curr;
  }
}
  

void link_layer_tx(){
  if (layer_1_tx_busy == 0){
    if (layer2_tx_buffer_counter<=L2_BUFFER_SIZE){
      layer_2_tx_request=1;
      if (tx_hamming_counter==0){
        tx_byte_hamming = l2_tx_buff[layer2_tx_buffer_counter];
        l1_tx_buffer = Hamming47_tx();
        tx_hamming_counter=1;
      }
      else{
        tx_byte_hamming = l2_tx_buff[layer2_tx_buffer_counter]>>4;
        l1_tx_buffer = Hamming47_tx();
        tx_hamming_counter = 0;
        layer2_tx_buffer_counter++;
      }
    }
  }
}

void link_layer_rx(){

  if (layer_1_rx_busy < layer_1_rx_busy_prev){ // falling edge of rx_busy flag
    if (l2_rx_counter % 2 == 0){ // even number of bits
      half_byte_lsb = Hamming47_rx();
    }else{ // odd number of bits
      half_byte_msb = Hamming47_rx();
      l2_buffer_pos = l2_rx_counter >> 1;
      l2_rx_buff[l2_buffer_pos] = (half_byte_msb << 4) | half_byte_lsb;
      Serial.print("Received char: %c\n", l2_rx_buff[l2_buffer_pos]);
    }
    l2_rx_counter ++;
    if (l2_rx_counter == L2_COUNT_LIMIT){
      l2_rx_counter = 0;
      Serial.print("Received String: %\n", l2_rx_buff);
    }
  }
  layer_1_rx_busy_prev = layer_1_rx_busy; 
}

char Hamming47_tx(){
      char temp_l2=0;
      P1 = bitRead(tx_byte_hamming,3) ^ bitRead(tx_byte_hamming,2) ^ bitRead(tx_byte_hamming,0);
      P2 = bitRead(tx_byte_hamming,3) ^ bitRead(tx_byte_hamming,1) ^ bitRead(tx_byte_hamming,0);
      P3 = bitRead(tx_byte_hamming,2) ^ bitRead(tx_byte_hamming,1) ^ bitRead(tx_byte_hamming,0);
      bitWrite(temp_l2,P1_bit,P1);
      bitWrite(temp_l2,P2_bit,P2);
      bitWrite(temp_l2,P3_bit,P3);
      bitWrite(temp_l2,D1_bit,bitRead(tx_byte_hamming,3));
      bitWrite(temp_l2,D2_bit,bitRead(tx_byte_hamming,2));
      bitWrite(temp_l2,D3_bit,bitRead(tx_byte_hamming,1));
      bitWrite(temp_l2,D4_bit,bitRead(tx_byte_hamming,0));
      Serial.print("Sending byte: %x\n", temp_l2);
      return temp_l2;
}

char Hamming47_rx(){
  char temp_rx=l1_rx_buffer;
  Serial.print("Recieved byte: %x\n", temp_rx);
  S1 = bitRead(temp_rx, P1_bit) ^ bitRead(temp_rx, D1_bit) ^ bitRead(temp_rx, D2_bit) ^ bitRead(temp_rx, D4_bit);
  S2 = bitRead(temp_rx, P2_bit) ^ bitRead(temp_rx, D1_bit) ^ bitRead(temp_rx, D3_bit) ^ bitRead(temp_rx, D4_bit);
  S3 = bitRead(temp_rx, P3_bit) ^ bitRead(temp_rx, D2_bit) ^ bitRead(temp_rx, D3_bit) ^ bitRead(temp_rx, D4_bit);
  int S = (S3 << 2) | (S2 << 1) | S1;
  if (S>0){
    int index = 7-(S);
    temp_rx = temp_rx ^ (1 << index);
    Serial.print("Fixed byte: %x\n", temp_rx);
  }
  char data = (bitRead(temp_rx, D1_bit) << 3) | (bitRead(temp_rx, D2_bit) << 2) | (bitRead(temp_rx, D3_bit) << 1) | bitRead(temp_rx, D4_bit);
  return data;
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


