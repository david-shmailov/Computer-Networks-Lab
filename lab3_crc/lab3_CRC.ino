// C code
//
#define TX_PIN 2
#define RX_PIN 3
#define CLK_OUT_PIN 4
#define CLK_IN_PIN 5
#define L2_BUFFER_SIZE 13 


// usart_tx global variables
const int BIT_TIME = 20;
const int half_BIT_TIME = BIT_TIME >> 1;
const int wait_max = 100*BIT_TIME;
const int buffer_size = 12;

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
int16_t rx_buff = 0;
int rx_count = 0; 


//layer2 tx global variables
int tx_busy = 1;
int layer_2_tx_request = 0;
int layer2_tx_buffer_counter = 0;
char l2_tx_buff [L2_BUFFER_SIZE] = "DAVID_NERIYA";
int16_t tx_byte_crc = 0;

//layer2 Rx global variables
int layer_1_rx_busy;
int l2_rx_counter = 0;
int l2_buffer_pos;
char l2_rx_buff [L2_BUFFER_SIZE];
int rx_error_flag=0;



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
    layer_1_rx_busy=1;
    rx_bit = digitalRead(RX_PIN);
    bitWrite(rx_buff, rx_count, rx_bit);
    rx_count ++;
    clk_in_prev = clk_in_curr;
    if (rx_count == buffer_size){
      rx_count = 0;
      layer_1_rx_busy=0;
    }
  }else{
    clk_in_prev = clk_in_curr;
  }
}
  

void link_layer_tx(){
  if (tx_busy == 0){
    if (layer2_tx_buffer_counter<=L2_BUFFER_SIZE){
      layer_2_tx_request=1;
      tx_byte_crc = l2_tx_buff[layer2_tx_buffer_counter];
      tx_buff = CRC4_tx();
    }
  }
}

char CRC4_tx(){
  int16_t tx_crc_temp = tx_byte_crc;
  tx_crc_temp<<4;
  int16_t tx_denom =0b0000100110000000;
  for (int crc_loop_count = 0;crc_loop_count<=6;crc_loop_count++){
    if (bitRead(tx_byte_crc,11-crc_loop_count)==1){
      tx_crc_temp = tx_crc_temp^tx_denom;
      tx_denom>>1;
    }
    else{
      tx_denom >>1;
    }
  }
  return (tx_byte_crc<<4)|tx_crc_temp;
}

void link_layer_rx(){
  if (layer_1_rx_busy==0){
    if (l2_rx_counter<13){
      int L2_rx_temp =CRC4_rx();
      if (rx_error_flag==1){
        Serial.print("Error in char #");
        Serial.print(l2_rx_counter);
        Serial.print("\n");
        l2_rx_buff[l2_rx_counter]=rx_buff;
        rx_error_flag=0;
      }
      else{
        l2_rx_buff[l2_rx_counter]=rx_buff;
      }
      l2_rx_counter++;
    }
    else{
      Serial.print("The Recieved string is:");
      Serial.print(l2_rx_buff);
      Serial.print("\n");
    }
  }
    
}
char CRC4_rx(){
  int16_t rx_crc_temp = rx_buff;
  int16_t rx_denom =0b0000100110000000;
  for (int crc_loop_count = 0;crc_loop_count<=6;crc_loop_count++){
    if (bitRead(rx_buff,11-crc_loop_count)==1){
      rx_crc_temp = rx_crc_temp^rx_denom;
      rx_denom>>1;
    }
    else{
      rx_denom >>1;
    }
  }
  if (rx_crc_temp==0){
    return rx_buff;
  }
  else{
    rx_error_flag=1;
    return 0;
  }
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
  link_layer_tx();
  link_layer_rx();
  usart_tx();
  usart_rx();
}







