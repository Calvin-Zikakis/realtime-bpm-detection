#include "daisy_pod.h"
extern "C" {
    #include "BTT.h"
}

using namespace daisy;

DaisyPod hw;
Parameter p_knob1, p_knob2;
BTT* btt;

// Global variables for audio level monitoring
float current_audio_level = 0.0f;
float raw_input_level = 0.0f;

float last_displayed_tempo = 0.0f;
uint32_t last_tempo_display_time = 0;

// Callback functions for BTT
void onset_detected_callback(void* SELF, unsigned long long sample_time) {
    DaisyPod* hw_ptr = (DaisyPod*)SELF;
    hw_ptr->led2.Set(1, 0, 0); // Flash red on onset
}

void beat_detected_callback(void* SELF, unsigned long long sample_time) {
    DaisyPod* hw_ptr = (DaisyPod*)SELF;
    hw_ptr->led2.Set(0, 1, 0); // Flash green on beat
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    // Increase MINIMUM gain (1.0 to 50.0) - much higher sensitivity
    float gain = hw.knob1.Process() * 49.0f + 1.0f;
    
    float input_buffer[size];
    float peak_level = 0.0f;
    
    // Convert input samples to mono and normalize with gain control
    for(size_t i = 0; i < size; i++) {
        // Average both channels and normalize from int32 to float
        input_buffer[i] = (in[0][i] + in[1][i]) * 0.5f * 0.00003051757f * gain;
        
        // Track peak level for visualization
        float abs_sample = fabsf(input_buffer[i]);
        if(abs_sample > peak_level) {
            peak_level = abs_sample;
        }
    }
    float raw_peak = 0.0f;
    for(size_t i = 0; i < size; i++) {
        float raw_sample = fabsf((in[0][i] + in[1][i]) * 0.5f * 0.00003051757f);
        if(raw_sample > raw_peak) {
            raw_peak = raw_sample;
        }
    }
    raw_input_level = raw_input_level * 0.7f + raw_peak * 0.3f;
    
    // Update the audio level (with smoothing)
    current_audio_level = current_audio_level * 0.7f + peak_level * 0.3f;
    

    // Process audio through BTT
    btt_process(btt, input_buffer, size);

    // Pass through audio to outputs
    for(size_t i = 0; i < size; i++) {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
    }
}

int main(void) {
    hw.Init();
    hw.seed.usb_handle.Init(UsbHandle::FS_BOTH);
    
    // Add a startup message
    hw.seed.usb_handle.TransmitInternal((uint8_t*)"Realtime BPM Detection started!\r\n", 33);
    hw.seed.usb_handle.TransmitInternal((uint8_t*)"Waiting for audio input...\r\n", 28);
  
    
    // Initialize parameters - no longer using Parameter wrapper for hw.knob1
    p_knob2.Init(hw.knob2, 0, 1, Parameter::LINEAR); // Reserved for future use
    
    hw.StartAdc();

    // Initialize BTT with callback functions
    btt = btt_new_default();
    if (btt == NULL) {
        while(1) {
            hw.led1.Set(1, 0, 0);
            hw.UpdateLeds();
        }
    }

    // Set up BTT callbacks
    btt_set_onset_tracking_callback(btt, onset_detected_callback, &hw);
    btt_set_beat_tracking_callback(btt, beat_detected_callback, &hw);

    // After BTT initialization, add a message
    if (btt != NULL) {
        hw.seed.usb_handle.TransmitInternal((uint8_t*)"BTT initialized successfully!\r\n", 31);
    }
    
    // Start audio
    hw.StartAudio(AudioCallback);

    uint32_t last_heartbeat_time = 0;

    while(1) {
        hw.ProcessDigitalControls();
        hw.ProcessAnalogControls();
        
        // Get knob1 value for LED intensity (0.0 to 1.0)
        float knob1_value = hw.knob1.Process();
        
        // Get current tempo estimation
        float tempo = btt_get_tempo_bpm(btt);
        
        // Display tempo regularly (every 500ms) or when it changes significantly
        uint32_t current_time = System::GetNow();
        if ((current_time - last_tempo_display_time > 500) || 
            (fabsf(tempo - last_displayed_tempo) > 1.0f)) {
            
            // Only display non-zero tempo values
            if (tempo > 0) {
                char text[64];
                sprintf(text, "Tempo: %.1f BPM\r\n", tempo);
                hw.seed.usb_handle.TransmitInternal((uint8_t*)text, strlen(text));
                
                last_displayed_tempo = tempo;
                last_tempo_display_time = current_time;
            }
        }

        if (current_time - last_heartbeat_time > 2000) {
            char heartbeat[64];
            sprintf(heartbeat, "System running... Raw level: %.6f, Processed level: %.6f\r\n", 
                raw_input_level, current_audio_level);
            hw.seed.usb_handle.TransmitInternal((uint8_t*)heartbeat, strlen(heartbeat));
            last_heartbeat_time = current_time;
        }
        
        // Rest of your existing LED control code
        if (hw.button1.Pressed()) {
            // Button press overrides other LED behavior
            hw.led2.Set(knob1_value, 0, 0);  // Red with intensity from knob
        } else if (tempo > 0) {
            // When tempo detected, show pure green (no mixing with other colors)
            hw.led1.Set(0, 1.0f, 0);
            hw.led2.Set(0, 1.0f, 0);  // Make sure it's full brightness
        } else {
            // No tempo detected, show blue on LED1 proportional to audio level
            float blue_intensity = current_audio_level * 20.0f; // Increased multiplier from 5.0f to 20.0f
            blue_intensity = fmax(0.05f, fmin(blue_intensity, 1.0f));
            hw.led1.Set(0, 0, blue_intensity);
            hw.led2.Set(0, 0, 0);
        }
        
        hw.UpdateLeds();
        System::Delay(10);
    }
}