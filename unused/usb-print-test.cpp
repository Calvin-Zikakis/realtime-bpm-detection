#include "daisy_pod.h"
#include <string.h>

using namespace daisy;

DaisyPod hw;

int main(void) {
    // Initialize hardware
    hw.Init();
    hw.seed.qspi.Init(); // Initialize QSPI Flash
    
    // Initialize USB
    hw.seed.usb_handle.Init(UsbHandle::FS_BOTH);
    
    // Wait for USB to be ready
    System::Delay(1000);
    
    // Send initial message
    char init_msg[] = "USB Print Test Starting\r\n";
    hw.seed.usb_handle.TransmitInternal((uint8_t*)init_msg, strlen(init_msg));
    
    // Counter and timing variables
    int counter = 0;
    uint32_t last_print_time = 0;
    
    // Main loop
    while(1) {
        // Process buttons to test input
        hw.ProcessDigitalControls();
        
        // Get current time
        uint32_t current_time = System::GetNow();
        
        // Print something every second
        if(current_time - last_print_time > 1000) {
            // Test 1: Print an integer
            char int_msg[64];
            sprintf(int_msg, "Counter: %d\r\n", counter);
            hw.seed.usb_handle.TransmitInternal((uint8_t*)int_msg, strlen(int_msg));

            System::Delay(1); // Small delay to avoid flooding the USB
            
            // Test 2: Print a float with different formats
            float test_float = 0.12345f;
            char float_msg[128];
            sprintf(float_msg, "Float tests: %.2f %.4f %.8f\r\n", test_float, test_float, test_float);
            hw.seed.usb_handle.TransmitInternal((uint8_t*)float_msg, strlen(float_msg));
            
            // Increment counter
            counter++;
            last_print_time = current_time;
        }
        
        // Flash LEDs to show the program is running
        bool led_on = (current_time % 500) < 250;
        hw.led1.Set(led_on ? 0.0f : 0.0f, led_on ? 1.0f : 0.0f, 0.0f);
        hw.led2.Set(led_on ? 1.0f : 0.0f, led_on ? 0.0f : 0.0f, 0.0f);
        hw.UpdateLeds();
        
        System::Delay(10);
    }
}