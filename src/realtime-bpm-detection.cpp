#include "daisy_pod.h"
extern "C" {
    #include "BTT.h"
}

using namespace daisy;

DaisyPod hw;
BTT* btt;

// Constants
constexpr size_t kSampleRate = 48000;
constexpr size_t kBufferSeconds = 10;
constexpr size_t kBufferSize = kSampleRate * kBufferSeconds;
constexpr size_t kSecondsPerUpdate = 1;
constexpr size_t kSamplesPerUpdate = kSampleRate * kSecondsPerUpdate;
constexpr size_t kNumBpmValues = 10; // Number of BPM values to average

// SDRAM allocation for large audio buffer (uninitialized data)
float DSY_SDRAM_BSS audioBuffer[kBufferSize];
size_t bufferIndex = 0;
size_t totalSamples = 0;
uint32_t lastUpdateTime = 0;

// Array to store recent BPM values in SDRAM (uninitialized data)
float DSY_SDRAM_BSS bpmValues[kNumBpmValues];
size_t bpmIndex = 0;
bool bpmArrayFilled = false;

// Define strings in regular flash memory (not in BSS section)
const char bpm_format_string[] = "BPM: %.1f (Certainty: %.2f)";
const char startup_message[] = "BPM Detection Starting...";
const char error_message[] = "BTT initialization failed!";

// Calculate average BPM from stored values
float CalculateAverageBpm() {
    float sum = 0.0f;
    size_t count = bpmArrayFilled ? kNumBpmValues : bpmIndex;
    
    if (count == 0) return 0.0f;
    
    for (size_t i = 0; i < count; i++) {
        sum += bpmValues[i];
    }
    
    return sum / count;
}

// Process BPM using BTT library - place in DTCM for stack-heavy function
__attribute__((section(".dtcmram"))) void ProcessBpm() {
    // Only process when we have enough data
    if (totalSamples >= kBufferSize) {
        // Process the audio buffer with BTT
        btt_process(btt, audioBuffer, kBufferSize);
        
        // Get the BPM from the BTT library
        float bpm = btt_get_tempo_bpm(btt);
        
        // Store BPM value
        bpmValues[bpmIndex] = bpm;
        bpmIndex = (bpmIndex + 1) % kNumBpmValues;
        if (bpmIndex == 0) bpmArrayFilled = true;
        
        // Calculate and display average BPM
        float avgBpm = CalculateAverageBpm();
        hw.seed.PrintLine(bpm_format_string, avgBpm, btt_get_tempo_certainty(btt));
    }
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    for(size_t i = 0; i < size; i++) {
        // Average the stereo inputs to mono
        float sample = (in[0][i] + in[1][i]) * 0.5f;
        
        // Store in the circular buffer
        audioBuffer[bufferIndex] = sample;
        bufferIndex = (bufferIndex + 1) % kBufferSize;
        
        // Pass through audio to outputs
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
    }
    
    // Increment total sample count (capped at kBufferSize)
    totalSamples = std::min(totalSamples + size, kBufferSize);
    
    // Check if it's time to update BPM calculation (every 1 second)
    uint32_t currentTime = System::GetNow();
    if (currentTime - lastUpdateTime >= 1000) {
        ProcessBpm();
        lastUpdateTime = currentTime;
    }
}

int main(void) {
    // Initialize Daisy Pod hardware
    hw.Init();
    hw.seed.usb_handle.Init(UsbHandle::FS_BOTH);
    
    // Clear the audio buffer
    for (size_t i = 0; i < kBufferSize; i++) {
        audioBuffer[i] = 0.0f;
    }
    
    // Initialize BTT using the correct function
    btt = btt_new_default();
    if (btt == nullptr) {
        hw.seed.StartLog();
        hw.seed.PrintLine(error_message);
        while(1) {
            hw.led1.Set(1.0f, 0.0f, 0.0f);
            hw.UpdateLeds();
            System::Delay(100);
            hw.led1.Set(0.0f, 0.0f, 0.0f);
            hw.UpdateLeds();
            System::Delay(100);
        }
    }
    
    // Set BTT parameters - use reduced parameters to save memory
    btt_set_num_tempo_candidates(btt, 10); // Reduced from 20
    btt_set_min_tempo(btt, 30);
    btt_set_max_tempo(btt, 180);
    
    // Initialize BPM array
    for (size_t i = 0; i < kNumBpmValues; i++) {
        bpmValues[i] = 0.0f;
    }
    
    // Set up audio
    hw.SetAudioBlockSize(64);
    hw.StartAudio(AudioCallback);
    
    // Initialize the update timer
    lastUpdateTime = System::GetNow();
    
    // Start USB serial for output
    hw.seed.StartLog();
    hw.seed.PrintLine(startup_message);
    
    // Main loop
    while(1) {
        // Update LEDs to indicate activity
        static bool led_state = false;
        led_state = !led_state;
        hw.led1.Set(led_state ? 0.8f : 0.0f, 0.0f, 0.0f); // Red LED flash
        hw.UpdateLeds(); // Required to actually update the LED state
        
        System::Delay(500);
    }
}