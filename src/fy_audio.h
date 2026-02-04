//
// Created by Liam Conrad on 2/4/26.
//

#ifndef FLOCK_YOU_FY_AUDIO_H
#define FLOCK_YOU_FY_AUDIO_H

#include <Arduino.h>

// Hardware Configuration
#define BUZZER_PIN 3  // GPIO3 (D2) - PWM capable pin on Xiao ESP32 S3

// Audio Configuration
#define LOW_FREQ 200      // Boot sequence - low pitch
#define HIGH_FREQ 800     // Boot sequence - high pitch & detection alert
#define DETECT_FREQ 1000  // Detection alert - high pitch (faster beeps)
#define HEARTBEAT_FREQ 600 // Heartbeat pulse frequency
#define BOOT_BEEP_DURATION 300   // Boot beep duration
#define DETECT_BEEP_DURATION 150 // Detection beep duration (faster)
#define HEARTBEAT_DURATION 100   // Short heartbeat pulse

// ============================================================================
// AUDIO SYSTEM
// ============================================================================

void beep(int frequency, int duration_ms);
void boot_beep_sequence();
void flock_detected_beep_sequence(void (*set_detected)());
void heartbeat_pulse();


#endif //FLOCK_YOU_FY_AUDIO_H
