//inclusion of appropriate libraries
#include <M5Core2.h>
#include <driver/i2s.h>

//setting up of software for beep sound output
extern const unsigned char previewR[120264];

#define CONFIG_I2S_BCK_PIN 12 
#define CONFIG_I2S_LRCK_PIN 0
#define CONFIG_I2S_DATA_PIN 2
#define CONFIG_I2S_DATA_IN_PIN 34

#define Speak_I2S_NUMBER I2S_NUM_0  

#define MODE_MIC 0  
#define MODE_SPK 1
#define DATA_SIZE 1024

//setting up of software for IMU input
float pitch = 0.0;
float roll  = 0.0;
float yaw   = 0.0;
float standard = 361.0;

//declaration of functions required for beep sound output
bool InitI2SSpeakOrMic(int mode){  
    esp_err_t err = ESP_OK;

    i2s_driver_uninstall(Speak_I2S_NUMBER); 
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),  
        .sample_rate = 44100, 
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, 
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, 
        .communication_format = I2S_COMM_FORMAT_I2S,  
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, 
        .dma_buf_count = 2, 
        .dma_buf_len = 128, 
    };
    if (mode == MODE_MIC){
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    }else{
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        i2s_config.use_apll = false;  
        i2s_config.tx_desc_auto_clear = true; 
    }
    
    err += i2s_driver_install(Speak_I2S_NUMBER, &i2s_config, 0, NULL);

    i2s_pin_config_t tx_pin_config;
    tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;  
    tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;  
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;  
    tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN; 
    err += i2s_set_pin(Speak_I2S_NUMBER, &tx_pin_config); 
    err += i2s_set_clk(Speak_I2S_NUMBER, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO); 
    return true;

}

void SpeakInit(void) { 
  M5.Axp.SetSpkEnable(true);  
  InitI2SSpeakOrMic(MODE_SPK);
}

void DingDong(void) {
  size_t bytes_written = 0;
  i2s_write(Speak_I2S_NUMBER, previewR, 120264, &bytes_written, portMAX_DELAY);
}

//setting up of software for real-time counter
RTC_TimeTypeDef TimeStruct;

void setup(){

    M5.begin(true, true, true); //initializes M5Stack device
    //formatting of M5Stack display screen 
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextWrap(true, true);
    M5.Lcd.setCursor(25,20);
    //printing of title & appropriate instructions to go with project 
    M5.Lcd.print("Correct Posture");
    M5.Lcd.setCursor(70,45);
    M5.Lcd.print("Electronic");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextWrap(true, true);
    M5.Lcd.setCursor(0,120);
    M5.Lcd.print("Sit to be in your ideal   position and press a      button after the beep.");
    delay(10000);
    SpeakInit();
    DingDong();
    M5.IMU.Init();  
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextSize(2);
    //resetting of real-time counter within M5Stack 
    TimeStruct.Seconds = 0;
    M5.Rtc.SetTime(&TimeStruct);
}

void loop() {
  
  M5.IMU.getAhrsData(&pitch,&roll,&yaw); //getting appropriate data from IMU sensor to within the program 
  M5.Rtc.GetTime(&TimeStruct); //getting appropriate data from real-time counter to within the program

  //printing of “roll” value from IMU sensor onto M5Stack display screen
  M5.Lcd.setCursor(0, 20);
  M5.Lcd.printf("angle");
  M5.Lcd.setCursor(0, 40);
  M5.Lcd.printf("%5.2f", roll);
  
  //get corresponding “roll” value for default position of user
  if (standard == 361) {
    M5.update(); //M5Stack ready to detect pressing of either of its buttons
    if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
      //store current “roll” value from IMU sensor into “standard” variable and print the same
      standard=roll; 
      M5.Lcd.setCursor(0,100);
      M5.Lcd.printf("%5.2f", standard);
      delay(5000);
  }
  } 

  else { 

  //if the “roll” value of the user’s current position is within acceptable range
    if(roll<(standard+10) && roll>(standard-25)){
      M5.Lcd.fillScreen(GREEN); //display a green screen
      //resetting of real-time counter within M5Stack 
      TimeStruct.Seconds = 0;
      M5.Rtc.SetTime(&TimeStruct);
    }

    //if the “roll” value of the user’s current position is outside acceptable range
    else{
      M5.Lcd.fillScreen(RED); //display a red screen
      if (TimeStruct.Seconds >= 5) { //check if number of seconds on real-time counter of M5Stack is equal to five seconds
        //perform vibration of M5Stack device and output a beep sound 
        M5.Axp.SetLDOEnable(3,true);   
        delay(500);
        M5.Axp.SetLDOEnable(3,false);
        M5.Axp.SetLDOEnable(3,true);  
        delay(100);
        M5.Axp.SetLDOEnable(3,false);
        DingDong();
        //resetting of real-time counter within M5Stack
        TimeStruct.Seconds = 0;
        M5.Rtc.SetTime(&TimeStruct);
      }
    }

  }
  
  delay(10);
 }