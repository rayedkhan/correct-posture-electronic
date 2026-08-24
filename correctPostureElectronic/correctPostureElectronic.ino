#include <M5Core2.h>
#include <driver/i2s.h>

// Alert beep, stored as raw PCM in beep_audio.c.
extern const unsigned char previewR[120264];

#define CONFIG_I2S_BCK_PIN 12
#define CONFIG_I2S_LRCK_PIN 0
#define CONFIG_I2S_DATA_PIN 2
#define CONFIG_I2S_DATA_IN_PIN 34

#define Speak_I2S_NUMBER I2S_NUM_0

#define MODE_MIC 0
#define MODE_SPK 1

#define VIBRATION_LDO 3

// Roll is read off the IMU in degrees. The baseline is whatever the user was
// sitting at when they pressed a button, so the tolerance is relative to that
// rather than to any fixed "correct" angle.
const float TOLERANCE_ABOVE = 10.0;
const float TOLERANCE_BELOW = 25.0;
const uint8_t ALERT_AFTER_SECONDS = 5;

// Sentinel: no baseline recorded yet. Outside the range roll can return.
const float NO_BASELINE = 361.0;

float pitch = 0.0;
float roll = 0.0;
float yaw = 0.0;
float baselineRoll = NO_BASELINE;

RTC_TimeTypeDef TimeStruct;

void InitI2SSpeakOrMic(int mode) {
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
    if (mode == MODE_MIC) {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    } else {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        i2s_config.use_apll = false;
        i2s_config.tx_desc_auto_clear = true;
    }

    i2s_driver_install(Speak_I2S_NUMBER, &i2s_config, 0, NULL);

    i2s_pin_config_t tx_pin_config;
    tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
    tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
    i2s_set_pin(Speak_I2S_NUMBER, &tx_pin_config);
    i2s_set_clk(Speak_I2S_NUMBER, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

void SpeakInit(void) {
    M5.Axp.SetSpkEnable(true);
    InitI2SSpeakOrMic(MODE_SPK);
}

void DingDong(void) {
    size_t bytes_written = 0;
    i2s_write(Speak_I2S_NUMBER, previewR, sizeof(previewR), &bytes_written, portMAX_DELAY);
}

void Buzz(void) {
    M5.Axp.SetLDOEnable(VIBRATION_LDO, true);
    delay(500);
    M5.Axp.SetLDOEnable(VIBRATION_LDO, false);
    M5.Axp.SetLDOEnable(VIBRATION_LDO, true);
    delay(100);
    M5.Axp.SetLDOEnable(VIBRATION_LDO, false);
}

// The real-time clock doubles as the how-long-have-you-been-slouching timer.
void ResetTimer(void) {
    TimeStruct.Seconds = 0;
    M5.Rtc.SetTime(&TimeStruct);
}

void setup() {
    M5.begin(true, true, true);

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextWrap(true, true);
    M5.Lcd.setCursor(25, 20);
    M5.Lcd.print("Correct Posture");
    M5.Lcd.setCursor(70, 45);
    M5.Lcd.print("Electronic");

    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextWrap(true, true);
    M5.Lcd.setCursor(0, 120);
    M5.Lcd.print("Sit to be in your ideal   position and press a      button after the beep.");

    delay(10000);
    SpeakInit();
    DingDong();

    M5.IMU.Init();
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextSize(2);
    ResetTimer();
}

void loop() {
    M5.IMU.getAhrsData(&pitch, &roll, &yaw);
    M5.Rtc.GetTime(&TimeStruct);

    M5.Lcd.setCursor(0, 20);
    M5.Lcd.printf("angle");
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.printf("%5.2f", roll);

    if (baselineRoll == NO_BASELINE) {
        M5.update();
        // Any of the three buttons records the current angle as the reference.
        if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
            baselineRoll = roll;
            M5.Lcd.setCursor(0, 100);
            M5.Lcd.printf("%5.2f", baselineRoll);
            delay(5000);
        }
    } else if (roll < (baselineRoll + TOLERANCE_ABOVE) && roll > (baselineRoll - TOLERANCE_BELOW)) {
        M5.Lcd.fillScreen(GREEN);
        ResetTimer();
    } else {
        M5.Lcd.fillScreen(RED);
        // Red is immediate, the buzz and beep only if it is held.
        if (TimeStruct.Seconds >= ALERT_AFTER_SECONDS) {
            Buzz();
            DingDong();
            ResetTimer();
        }
    }

    delay(10);
}
