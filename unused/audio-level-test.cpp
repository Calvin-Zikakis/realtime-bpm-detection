#include "daisy_pod.h"

using namespace daisy;

DaisyPod hw;

// Global variables
float current_level = 0.0f;    // Smoothed audio level

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    // Get gain control from POT1 (0.1x to 10.0x)
    float gain = hw.knob1.Process() * 9.9f + 0.1f;
    
    float block_peak = 0.0f;
    
    // Process audio
    for(size_t i = 0; i < size; i++) {
        // Convert input samples to float for processing
        float left_in = in[0][i] * 0.00003051757f;   // Convert to -1.0 to 1.0 range
        float right_in = in[1][i] * 0.00003051757f;
        
        // Apply gain
        float left_out = left_in * gain;
        float right_out = right_in * gain;
        
        // Convert back to int32 for output
        out[0][i] = left_out * 32767.0f;
        out[1][i] = right_out * 32767.0f;
        
        // Track peak level AFTER gain is applied
        float left_abs = fabsf(left_out);
        float right_abs = fabsf(right_out);
        float sample_peak = fmaxf(left_abs, right_abs);
        
        if(sample_peak > block_peak) {
            block_peak = sample_peak;
        }
    }
    
    // Update level with fast attack, slow decay
    if(block_peak > current_level) {
        current_level = block_peak;
    } else {
        current_level = current_level * 0.99f;
    }
}

int main(void) {
    // Initialize hardware
    hw.Init();
    
    // Start audio processing
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    
    // Main loop
    while(1) {
        hw.ProcessAnalogControls();
        hw.ProcessDigitalControls();
        
        // Apply sensitivity boost for better visualization
        float level = fmin(current_level * 5.0f, 1.0f);
        
        // LED color interpolation from green to yellow to red
        // Green intensity decreases as level rises
        float green = 1.0f - (level > 0.8f ? (level - 0.8f) * 5.0f : 0.0f);
        green = fmax(0.0f, green);
        
        // Red intensity increases with level
        float red = level;
        
        // Set both LEDs to show the same color
        hw.led1.Set(red, green, 0.0f);
        hw.led2.Set(red, green, 0.0f);
        
        hw.UpdateLeds();
        System::Delay(10);
    }
}