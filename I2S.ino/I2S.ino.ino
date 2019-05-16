#include <HiFi.h>
#include <delay.h>
#include <distortion.h>
#include <effect.h>
#include <ringModulator.h>
#include <ssc.h>

#define MAX_DELAY 40000

void TC4_Handler();
void codecTxReadyInterrupt(HiFiChannelID_t);
void codecRxReadyInterrupt(HiFiChannelID_t);
void Distortion_process_samples(float *inputbuffer);
void REVERB_process_samples(float *inputbuffer);
void DELAY_process_samples(float *inputbuffer);
void TREMOLO_process_samples(float *inputbuffer);

volatile char EFFECT;

static uint32_t ldat = 0;
static uint32_t rdat = 0;

int POT0, POT1, POT2, POT3;
int LED0 = 7;
int LED1 = 6;
int LED2 = 5;
int LED3 = 4;
int LED_pwr = 3;

void setup() {
  //////////////////////////  SET UP TIMER: ECHO   ////////////////////
  pmc_set_writeprotect(false);
  pmc_enable_periph_clk(ID_TC4);
 
  //we want wavesel 01 with RC 
  TC_Configure(TC1,1, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK2);
  TC_SetRC(TC1, 1, 238); // sets <> 44.1 Khz interrupt rate
  TC_Start(TC1, 1);
 
  // enable timer interrupts on the timer
  TC1->TC_CHANNEL[1].TC_IER=TC_IER_CPCS;
  TC1->TC_CHANNEL[1].TC_IDR=~TC_IER_CPCS;
 
  //Enable the interrupt in the nested vector interrupt controller 
  //TC4_IRQn where 4 is the timer number * timer channels (3) + the channel number 
  //(=(1*3)+1) for timer1 channel1 
  NVIC_EnableIRQ(TC4_IRQn);
 
  ///////////////////////     I2S COMMUNICATION      //////////////////
  // set codec into reset
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);
  pinMode(LED_pwr, OUTPUT);
  digitalWrite(LED_pwr, HIGH);

  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  HiFi.begin();

  // Configure transmitter for 2 channels, external TK/TF clocks, 32 bit per
  // channel (data less than 32-bit is left justified in the 32 bit word, but
  // codec config needs 32 clocks per channel).
  HiFi.configureTx(HIFI_AUDIO_MODE_STEREO, HIFI_CLK_MODE_USE_EXT_CLKS, 32);

  // Same config as above, except sync the receiver to transmitter (RK/RF
  // clock signals not needed)
  HiFi.configureRx(HIFI_AUDIO_MODE_STEREO, HIFI_CLK_MODE_USE_TK_RK_CLK, 32);
  HiFi.onTxReady(codecTxReadyInterrupt);
  HiFi.onRxReady(codecRxReadyInterrupt);

  // release codec from reset
  digitalWrite(7, HIGH);

  // Enable both receiver and transmitter.
  HiFi.enableRx(true);
  HiFi.enableTx(true);

  ///////////////////////     EFFECT CHANGE INTERRUPTS      //////////////////

  attachInterrupt(DISTORTION_pin,switchTo_DISTORTION,LOW);
  attachInterrupt(DELAY_pin,switchTo_ECHO,LOW);
  attachInterrupt(REVERB_pin,switchTo_REVERB,LOW);
  attachInterrupt(TREMOLO_pin,switchTo_TREMOLO,LOW);

  ///////////////////////     ADC POTENTIOMETERS      //////////////////
  //ADC Configuration
  ADC->ADC_MR |= 0x80;
  ADC->ADC_CR= 0x02;         // Starts ADC conversion.
  ADC->ADC_CHER= 0xF0;  // Enable ADC channels ch7-A0, ch6-A1, ch5-A2, ch4-A3  
  
void loop() {
  POT0=ADC->ADC_CDR[7];                // read data from ADC7        
  POT1=ADC->ADC_CDR[8];                // read data from ADC8   
  POT2=ADC->ADC_CDR[9];                // read data from ADC9
  POT3=ADC->ADC_CDR[10];                 // read data from ADC10  

  switch (EFFECT){
    case DISTORTION:
      left_out = DISTORTION_Process_Samples(*left_in);
      right_out = DISTORTION_Process_Samples(*right_in);
      
      //adjust the volume with POT1 -- 2^24 (input signal bit res.) mapped to 2^12 (adc is 12 bit res.)
      left_out=map(left_out,0,16777215,1,POT1);
      right_out=map(right_out,0,16777215‬,1,POT1);
      
    case DELAY:
//======================================================================================================//
//
// Echo's Function : 
//                  y(n) = x(n) + a.x(n-D)     [137]
//
//======================================================================================================//
     
      
      //adjust the volume with POT2
      left_out=map(left_in,0,4095,1,POT2);
      right_out=map(right_in,0,4095,1,POT2);
      
    case REVERB:

      //adjust the volume with POT2
      left_out=map(left_in,0,4095,1,POT2);
      right_out=map(right_in,0,4095,1,POT2);
      
    case TREMOLO:

      //adjust the volume with POT2
      left_out=map(left_in,0,4095,1,POT2);
      right_out=map(right_in,0,4095,1,POT2);
  }
}

void codecTxReadyInterrupt(HiFiChannelID_t channel)
{
  if (channel == HIFI_CHANNEL_ID_1)
  {
    // Left channel
    HiFi.write(left_out);  
  }
  else
  {
    // Right channel
    HiFi.write(right_out);
  }
}

void codecRxReadyInterrupt(HiFiChannelID_t channel)
{
  if (channel == HIFI_CHANNEL_ID_1)
  {
    // Left channel
    left_in = HiFi.read();
  }
  else
  {
    // Right channel
    right_in = HiFi.read();
  }
}

void switchTo_DISTORTION{
  digitalWrite(LED0, HIGH);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW); 
  EFFECT = DISTORTION;
  return EFFECT;
}

void switchTo_DELAY{
  digitalWrite(LED0, HIGH);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW); 
  EFFECT = ECHO;
  return EFFECT;
}

void switchTo_REVERB{
  digitalWrite(LED0, HIGH);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW); 
  EFFECT = REVERB;
  return EFFECT;
}

void switchTo_TREMOLO{
  digitalWrite(LED0, HIGH);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW); 
  EFFECT = TREMOLO;
  return EFFECT;
}

//Interrupt at 44.1KHz rate (every 22.6us)
void TC4_Handler()
{
  //Clear status allowing the interrupt to be fired again.
  TC_GetStatus(TC1, 1);
 
  //Store current readings  
  sDelayBuffer0[DelayCounter]  = (left_in + (sDelayBuffer0[DelayCounter]))>>1;
  sDelayBuffer1[DelayCounter]  = (right_in + (sDelayBuffer0[DelayCounter]))>>1;

  //Adjust Delay Depth based in pot0 position.
  Delay_Depth =map(POT4>>2,0,2047,1,MAX_DELAY);
 
  //Increse/reset delay counter.   
  DelayCounter++;
  if(DelayCounter >= Delay_Depth) DelayCounter = 0; 
  left_out = ((sDelayBuffer0[DelayCounter]));
 
  left_out=map(left_in,0,16777215,1,POT1);
  right_out=map(right_in,0,16777215‬,1,POT1);
}

// DISTORTION ALGORITHM_2
   /*
      // DISTORTION parameters
      upper_threshold=map(POT4,0,16777215,POT3,8388608);
      lower_threshold=map(POT4,0,16777215,POT2,8388608);

      // apply DISTORTION threshold
      if(left_in>=upper_threshold) ldat=upper_threshold; 
      else if(left_in<lower_threshold)  ldat=lower_threshold;
        
      if(right_in>=upper_threshold) right_in=upper_threshold;
      else if(right_in<lower_threshold)  right_in=lower_threshold;   
    */


 
