//
//  datahandling.h
//  ControllerSupport360App
//
//  Created by Skyler Calaman on 7/31/22.
//

#ifndef datahandling_h
#define datahandling_h

#include "sharedstructs.h"

#include <USBDriverKit/AppleUSBDescriptorParsing.h>

ControllerState controller_data_to_state(ControllerData data_struct) {
    unsigned char *data = data_struct.data;
    ControllerState state = ControllerState{
        0,0,0,0,
        DPadState { 0,0,0,0, },
        MainButtonState { 0,0,0,0, },
        0,0,0,0,
    };
    
    state.r3         = (data[2] & 0b10000000) != 0;
    state.l3         = (data[2] & 0b01000000) != 0;
    state.back       = (data[2] & 0b00100000) != 0;
    state.start      = (data[2] & 0b00010000) != 0;
    state.dpad.dpadr = (data[2] & 0b00001000) != 0;
    state.dpad.dpadl = (data[2] & 0b00000100) != 0;
    state.dpad.dpadd = (data[2] & 0b00000010) != 0;
    state.dpad.dpadu = (data[2] & 0b00000001) != 0;
    
    state.main_buttons.y = (data[3] & 0b10000000) != 0;
    state.main_buttons.x = (data[3] & 0b01000000) != 0;
    state.main_buttons.b = (data[3] & 0b00100000) != 0;
    state.main_buttons.a = (data[3] & 0b00010000) != 0;
    state.unused         = (data[3] & 0b00001000) != 0;
    state.xbox           = (data[3] & 0b00000100) != 0;
    state.r_bump         = (data[3] & 0b00000010) != 0;
    state.l_bump         = (data[3] & 0b00000001) != 0;
    
    state.l_trigger = data[4];
    state.r_trigger = data[5];
    
    state.l_joystick.x = (data[ 7] << 8) + data[ 6];
    state.l_joystick.y = (data[ 9] << 8) + data[ 8];
    state.r_joystick.x = (data[11] << 8) + data[10];
    state.r_joystick.y = (data[13] << 8) + data[12];
    
    return state;
}


ControlPipes apply_control_data_interface_endpoints(const IOUSBConfigurationDescriptor *configurationDescriptor, const IOUSBInterfaceDescriptor *interfaceDescriptor) {
    ControlPipes result = ControlPipes {
        0,
        0,
        0,
        0
    };
    IOUSBEndpointDescriptor const *currEndpoint = NULL;
    while (true) {
        currEndpoint = IOUSBGetNextEndpointDescriptor(configurationDescriptor, interfaceDescriptor, (const IOUSBDescriptorHeader *) currEndpoint);
        if (currEndpoint == NULL) {
            break;
        }
        // All input endpoints have the highest bit set, and vice versa for outputs.
        if (currEndpoint->bEndpointAddress & 0x80) {
            if (result.input) {
                // 2 input endpoints on the first interface for some reason?
                level_aware_log(RELEASE_LOG, "Expected 1 input endpoint (0x%{public}02x), but also got 0x%{public}02x", result.input, currEndpoint->bEndpointAddress);
            } else {
                level_aware_log(DEBUG_LOG, "Input endpoint get! 0x%{public}02x", currEndpoint->bEndpointAddress);
            }
            result.input = currEndpoint->bEndpointAddress;
            // Storing this because it's annoying to get from the endpoint later.
            result.input_max_packet_size = currEndpoint->wMaxPacketSize;
        } else {
            if (result.output) {
                // 2 output endpoints on the first interface for some reason?
                level_aware_log(RELEASE_LOG, "Expected 1 output endpoint (0x%{public}02x), but also got 0x%{public}02x", result.output, currEndpoint->bEndpointAddress);
            } else {
                level_aware_log(DEBUG_LOG, "Output endpoint get! 0x%{public}02x", currEndpoint->bEndpointAddress);
            }
            result.output = currEndpoint->bEndpointAddress;
            // Storing this because it's annoying to get from the endpoint later.
            result.output_max_packet_size = currEndpoint->wMaxPacketSize;
        }
    }
    
    return result;
}

#endif /* datahandling_h */
