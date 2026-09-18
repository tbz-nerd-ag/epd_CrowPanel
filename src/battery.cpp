#include <Arduino.h>
#include <esp_adc_cal.h>

// ── Pins & Konstanten ─────────────────────────────────────
#define BAT_PIN        8
#define BAT_SAMPLES    20        // Mittelwert über 20 Messungen

// !! Hier den echten Teiler eintragen (Schaltplan prüfen)
// Wenn R1=100k, R2=100k → 2.0f
// Wenn R1=100k, R2=47k  → 3.13f  usw.
#define DIVIDER_RATIO  2.105f   // war: 2.105f

// ── ADC-Kalibrierung ──────────────────────────────────────
static esp_adc_cal_characteristics_t adc_chars;

void initBatteryADC() {
    // Vref aus eFuse lesen (falls vorhanden), sonst Fallback auf 1100mV
    esp_adc_cal_value_t cal_type = esp_adc_cal_characterize(
        ADC_UNIT_1,
        ADC_ATTEN_DB_11,
        ADC_WIDTH_BIT_12,
        1100,
        &adc_chars
    );

    if (cal_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.println("[BAT] Kalibrierung: eFuse Vref");
    } else if (cal_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        Serial.println("[BAT] Kalibrierung: eFuse Two Point");
    } else {
        Serial.println("[BAT] Kalibrierung: Standard (1100mV)");
    }

    // ADC-Pin konfigurieren
    analogSetPinAttenuation(BAT_PIN, ADC_11db);
}

// Liefert die rohe, gemittelte Batteriespannung in Volt
float readBatteryVoltage() {
    // Mehrfachmessung und Mittelwert
    uint32_t sum = 0;
    for (int i = 0; i < BAT_SAMPLES; i++) {
        int raw = analogRead(BAT_PIN);
        sum += esp_adc_cal_raw_to_voltage(raw, &adc_chars);
        delay(5);
    }
    float vPin_mv = sum / (float)BAT_SAMPLES;
    float vPin    = vPin_mv / 1000.0f;
    float vBat    = vPin * DIVIDER_RATIO;

    // Serial.printf("[BAT] ADC: %.0f mV → Pin: %.3f V → Bat: %.3f V\n",
         //         vPin_mv, vPin, vBat);
    return vBat;
}