//define the global variable would be used
int LDR_Pin = A0;     //ADC input read from photodiode
int analog_value;       //analog value read from ADC
const int threshold = 200;    //analog threshold for determine 1 or 0

volatile String buf;       //buffer the sampled bit
String packet;    //byte unit

//for loop function, each loop to check if in same packet.
//when finish process a packet then set to false;
//bool is_packet = false;

int seq = 0;
int data_size = 0;


int delay_ms = 10;        //one smaple ms


void setup()
{
  Serial.begin(9600);
  // initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12);    // Prescale value: 256

  TIMSK1 |= (1 << OCIE1A);   // enable timer compare interrupt

  //time count = clk freq(16Mhz) / 256 / target frequency
  OCR1A = 62500/100;               // now frequency is 1 Hz(62500)
  OCR1B = 0;


  TCNT1 = 0;            // Reset Timer
  interrupts();         // enable all interrupts
  
}

ISR(TIMER1_COMPA_vect)        // interrupt service routine
{
  buf += getBit();

}

void loop()
{
//    Serial.println("wait for com");
    startCom();
//    Serial.println("start com complete");
    delay(delay_ms);
    syn();
//    Serial.println("syn complete");
    
    procHeader();
    
    procData();

  //no need to clear buf, we have start and syn handle
}

//function to get processed adc value;
char getBit(){
  analog_value = analogRead(LDR_Pin);
  //print out value for get threshold
//  Serial.println(analog_value);
  if(analog_value > threshold)
    return '1';
    else return '0';
}

/* CRC-8 */
unsigned char checksum(const String data, unsigned int len) {
  const unsigned char polynomial = 0x1D;
  unsigned char res = 0;

  for (unsigned int i = 0; i < len; i++) {
    res ^= data[i];
    for (unsigned int j = 0; j < 8; j++) {
      if ((res & 0x80) != 0) {
        res = (unsigned char) ((res << 1) ^ polynomial);
      } else {
        res <<= 1;
      }
    }
  }

  return res;
}

//wait until get the start
void startCom(){
  //start check

  while(1){
    
  while(buf.length() < 4)
    delay(delay_ms);
  
  if(buf.startsWith("1111")){
          return true;
  }
  else {
        buf.remove(0,4);
        continue; 
  } 
  
  }
}

bool syn(){   
    while(buf[0] != '0'){
//      Serial.println("remove 1");
      buf.remove(0);
      delay(delay_ms);
    }
    //get rid of 1 before syn pattern
    
    while(buf.length() < 8)
      delay(delay_ms);
      
    if(buf.startsWith("01010101")){
//        Serial.println("syn done");
        buf.remove(0,8);
        return true;
    }  
      else{
        //clear all received msg, start over agian
//        Serial.println("syn failed");
//        Serial.println(buf.substring(0,8));
        buf.remove(0,buf.length()-1);
        return false;  
    }
 
}

void procHeader(){
    while(buf.length() < 16)
      delay(delay_ms);
    //processing packet header
    
    //substring [a,b)
    
    seq = binaryInt(buf.substring(0,8).c_str());
    packet += char(seq);
    buf.remove(0,8);
    

    //size
    data_size = binaryInt(buf.substring(0,8).c_str());
    packet += char(data_size);
    buf.remove(0,8); 
    
//    Serial.println("proc header info");
//    Serial.println(seq);    
//    Serial.println(data_size);
}

void procData(){
    //processing the data part
    while(packet.length() < data_size + 2){
      while(buf.length() < 8)
        delay(delay_ms);
      
      int data = binaryInt(buf.substring(0,8).c_str());
//      Serial.print("data:");
//      Serial.println(char(data));
      packet += char(data);
      buf.remove(0,8);
         
    }
    
    //do the crc checksum
    if(packet.length() == data_size +2){
//        Serial.println("Size correct");
        char c = checksum(packet, data_size+2);
        packet += c; 

        //send to pc by serial port
        Serial.write(packet.c_str());
              
//        Serial.print("checksum");
//        Serial.println(c);
    }
    //clear packet
    packet.remove(0);
}

int binaryInt(char *c){
  int ret = 0;
  for (int i = 0; i < 8; ++i )
    ret |= (c[i] == '1') << (7 - i);
   return ret;
  
}

