// C code
//
#define TX_PIN 2
#define RX_PIN 3

#define SAMP_TIME 3


typedef enum {EVEN=0, ODD=1} parity_type;
const parity_type PARITY = ODD;
const unsigned long BIT_TIME = 400;
const int STOP_BIT_TIME = 1; //todo what happens in 1.5?
const unsigned long half_BIT_TIME = BIT_TIME >> 1;
const unsigned long quarter_BIT_TIME = BIT_TIME >> 2;
const unsigned long wait_max = 100*BIT_TIME;
const int buffer_size = 8;
long FRAME_TIME = (1 + buffer_size + 1 + STOP_BIT_TIME)*BIT_TIME;


// uart_tx global variables
typedef enum {ACTIVE, COML2} tx_state_type;
unsigned long tx_start_time = 0;
unsigned long tx_curr_time = 0;
unsigned long delta_t;
int tx_buff = 0b01010101;
int tx_count = 0;
int tx_bit;
long tx_wait;
int tx_parity = PARITY; // will it work?
tx_state_type tx_state = ACTIVE;





// uart_Rx global variables
const int rx_buffer_size = buffer_size + 1; // +1 for parity bit
typedef enum {IDLE, DATA_BITS, STOP_BIT, ERROR} rx_state_type;
rx_state_type rx_state = IDLE;

int rx_sample;
int rx_parity = 0;
//char rx_buff [buffer_size];
int rx_buff;
unsigned int rx_count = 0;
unsigned int sample_count = 0;
unsigned long rx_curr_time = 0;
unsigned long rx_start_time = 0;
unsigned long rx_delta_t = 0;
unsigned long next_sample_time = quarter_BIT_TIME; // initial state need to be fixed
int rx_sample_result;
short sample_buffer = 0x0;



void uart_tx(){
  tx_curr_time = millis();
  delta_t = tx_curr_time - tx_start_time;
  switch(tx_state){
    case ACTIVE:
   		if (delta_t >= BIT_TIME){
        //Start bit
        if (tx_count==0){
          digitalWrite(TX_PIN,LOW);
          tx_count++;
          tx_start_time=millis();
        }
        //Data
        else if (tx_count<=buffer_size){
          tx_bit = bitRead(tx_buff, tx_count-1);
          digitalWrite(TX_PIN,tx_bit);
          tx_start_time=millis();
          tx_parity = tx_parity^tx_bit;
          tx_count++;
        }
        //Parity
        else if (tx_count==buffer_size+1){
          //tx_bit=bitRead(tx_parity,0);
          digitalWrite(TX_PIN,tx_parity);
          tx_start_time=millis();
          tx_parity=PARITY;
          tx_count++;
        }
        //Stop bit 
        else if (tx_count==buffer_size+2){
          digitalWrite(TX_PIN,HIGH);
          tx_start_time=millis()+(STOP_BIT_TIME-1)*BIT_TIME;
          tx_count++;
        }
        //Idle/COML2
        else{
          tx_wait = random(wait_max);
          tx_start_time=millis();
          tx_state = COML2;
          tx_count = 0;
        }
        break;
      }   
    case COML2:
      if (delta_t > tx_wait){
        tx_start_time = millis()-BIT_TIME;
        tx_state = ACTIVE;  
      }
      break;
    default:
      //print("Error!"); // todo
      break;
  }

}

void uart_rx(){
  rx_curr_time = millis();
  switch (rx_state)
  {
    case IDLE:
      rx_sample_result = sample();
      if (rx_sample_result == 0){
        rx_state = DATA_BITS;
      }else if (rx_sample_result==-1){
        rx_state = ERROR;
      }
      break;
    case DATA_BITS:
      rx_sample_result = sample();
      if (rx_sample_result != 2){
        if (rx_sample_result == -1){
          rx_state = ERROR; // todo add computation of remaining wait time
          break; // test this break
        }   
        rx_parity = rx_parity ^ rx_sample_result;
        bitWrite(rx_buff, rx_count, rx_sample_result);
        rx_count++;
        if (rx_count > rx_buffer_size){
          rx_state = STOP_BIT;
          if (rx_parity != PARITY){
            rx_state = ERROR;// todo add computation of remaining wait time
          }
        }
      }
    case STOP_BIT:
      rx_sample_result = sample();
      if (rx_sample_result == 1){
        rx_parity = 0;
        rx_state = IDLE;
        rx_count = 0;
      }
    case ERROR:
      rx_delta_t = rx_curr_time - rx_start_time;
      if (rx_delta_t > FRAME_TIME-1 ){ // -1 due to start bit already happened
        rx_parity = 0;
        rx_state = IDLE;
        rx_start_time = millis();
      }
    default:
      break;
  }
}

int sample(){
  // rx_curr_time = millis(); // not required
  rx_delta_t = rx_curr_time - rx_start_time;
  if (rx_delta_t > next_sample_time){
    if (sample_count < 3){
      rx_sample = digitalRead(RX_PIN);
      bitWrite(sample_buffer, sample_count, rx_sample);
      next_sample_time = (sample_count+1)*quarter_BIT_TIME;
      sample_count++;
    }
    else{
      sample_count = 0;
      next_sample_time = quarter_BIT_TIME;
      if (sample_buffer == 0x7){
        rx_start_time = millis();
        return 1; // consider char cast
      }
      else if (sample_buffer == 0x0){
        rx_start_time = millis();
        return 0;
      }
      else{
        rx_start_time = millis();
        return -1;
      }
    }
  }
  else{
    return 2;
  }
}

void setup()
{
  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT);
}

void loop()
{
  uart_tx();
  uart_rx();
}


