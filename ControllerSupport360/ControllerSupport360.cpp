//
//  ControllerSupport360.cpp
//  ControllerSupport360
//
//  Created by Skyler Calaman on 6/23/22.
//

#include <os/log.h>
#include <chrono>

#include <DriverKit/IOUserServer.h>
#include <DriverKit/IOLib.h>
#include <USBDriverKit/IOUSBHostInterface.h>
#include <USBDriverKit/USBDriverKitDefs.h>
#include <USBDriverKit/AppleUSBDescriptorParsing.h>

#include "ControllerSupport360.h"

#define LOG_CURR_FUNC() os_log(OS_LOG_DEFAULT, "%{public}s", __FUNCTION__)

#define RETURN_TRAP(VARIABLE, TARG_VALUE, STATEMENT) do { \
    if (VARIABLE != TARG_VALUE) { \
        STATEMENT; \
        os_log(OS_LOG_DEFAULT, "%{public}s on line: %{public}d", "returning...", __LINE__); \
        return VARIABLE; \
    }\
} while (0)


#define STOP Stop(provider, SUPERDISPATCH);
#define FREE_CONTROL_INPUT_PIPE

#define EXIT_WITH_TARGET(TARGET) { \
    free_ivars(this, ivars); \
    STOP; \
    ret = TARGET; \
}

#define EXIT_UNSPEC_TARGET() { \
    free_ivars(this, ivars); \
    STOP; \
}

#define OS_LOG_BASIC_STRING(STRING)

typedef struct ControlPipes {
    uint8_t input;
    uint16_t input_max_packet_size;
    uint8_t output;
    uint16_t output_max_packet_size;
} ControlPipes;


typedef struct ControlDataInput {
    IOUSBHostPipe            *in_pipe;
    IOBufferMemoryDescriptor *in_data;
    OSAction                 *handler;
    uint16_t                  max_packet_size;
    uint64_t                  prev_nanos;
    uint8_t                   iters;
} ControlDataInput;

typedef struct ControlDataOutput {
    IOUSBHostPipe            *out_pipe;
} ControlDataOutput;

typedef struct ControllerData { unsigned char data[0x14]; } ControllerData;

typedef struct DPadState {
    bool dpadr;
    bool dpadl;
    bool dpadd;
    bool dpadu;
} DPadState;

typedef struct MainButtonState {
    bool y;
    bool x;
    bool b;
    bool a;
} MainButtonState;


typedef struct ControllerState {
    bool r3;
    bool l3;
    bool back;
    bool start;
    DPadState dpad;
    MainButtonState main_buttons;
    bool unused;
    bool xbox;
    bool r_bump;
    bool l_bump;
} ControllerState;

struct ControllerSupport360_IVars {
    IOUSBHostDevice *device;
    
    IOUSBHostInterface *control_data_interface;
    IOUSBHostInterface *headset_interface;
    
    ControlDataInput main_data_input;
};

#define DECL_BUTTON(STATE, PROPERTY) const char *PROPERTY = STATE.PROPERTY ? #PROPERTY ", " : ""

#define DECL_BUTTON_NESTED(STATE, PROPERTY, SUB_PROP) \
const char *PROPERTY ## _ ## SUB_PROP = STATE.PROPERTY.SUB_PROP ? #PROPERTY ">" #SUB_PROP ", " : ""

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
    
    return state;
}

void log_controller_state(ControllerState state) {
    DECL_BUTTON(state, r3);
    DECL_BUTTON(state, l3);
    
    DECL_BUTTON(state, back);
    DECL_BUTTON(state, start);
    
    DECL_BUTTON_NESTED(state, dpad, dpadr);
    DECL_BUTTON_NESTED(state, dpad, dpadl);
    DECL_BUTTON_NESTED(state, dpad, dpadd);
    DECL_BUTTON_NESTED(state, dpad, dpadu);
    
    DECL_BUTTON_NESTED(state, main_buttons, y);
    DECL_BUTTON_NESTED(state, main_buttons, x);
    DECL_BUTTON_NESTED(state, main_buttons, b);
    DECL_BUTTON_NESTED(state, main_buttons, a);
    
    DECL_BUTTON(state, xbox);
    
    DECL_BUTTON(state, r_bump);
    DECL_BUTTON(state, l_bump);
    
    
    os_log(
           OS_LOG_DEFAULT,
           "%{public}s%{public}s%{public}s%{public}s%{public}s%{public}s%{public}s%{public}s%{public}s%{public}s%{public}s%{public}s%{public}s%{public}s%{public}s",
           r3, l3,
           back, start,
           dpad_dpadr, dpad_dpadl, dpad_dpadd, dpad_dpadu,
           main_buttons_y, main_buttons_x, main_buttons_b, main_buttons_a,
           xbox,
           r_bump, l_bump);
}

ControlPipes apply_control_data_interface_endpoints(const IOUSBConfigurationDescriptor *configurationDescriptor, const IOUSBInterfaceDescriptor *interfaceDescriptor);


void free_ivars(ControllerSupport360 *provider, ControllerSupport360_IVars *ivars) {
    if (ivars->main_data_input.in_data) {
        ivars->main_data_input.in_data->free();
        ivars->main_data_input.in_data = NULL;
    }
    if (ivars->main_data_input.in_pipe) {
        ivars->main_data_input.in_pipe->free();
        ivars->main_data_input.in_pipe = NULL;
    }
    if (ivars->control_data_interface) {
        ivars->control_data_interface->Close(provider, 0);
        ivars->control_data_interface = NULL;
    }
    if (ivars->headset_interface) {
        ivars->headset_interface->Close(provider, 0);
        ivars->headset_interface = NULL;
    }
    if (ivars->device) {
        ivars->device->Close(provider, 0);
        ivars->device = NULL;
    }
}


bool ControllerSupport360::init() {
    bool result = false;
    
    LOG_CURR_FUNC();
    
    result = super::init();
    RETURN_TRAP(result, true, {});
    
    ivars = IONewZero(ControllerSupport360_IVars, 1);
    RETURN_TRAP(ivars == NULL, false, {});
    
    return result;
}

kern_return_t IMPL(ControllerSupport360, Start) {
    kern_return_t ret;
    
    
    LOG_CURR_FUNC();
    
    ret = Start(provider, SUPERDISPATCH);
    RETURN_TRAP(ret, kIOReturnSuccess, Stop(provider, SUPERDISPATCH));
        
    ivars->device = OSDynamicCast(IOUSBHostDevice, provider);
    RETURN_TRAP(ivars->device == NULL, false, EXIT_WITH_TARGET(kIOReturnNoDevice));
    
    ret = ivars->device->Open(this, 0, NULL);
    RETURN_TRAP(ret, kIOReturnSuccess, Stop(provider, SUPERDISPATCH));
    
    auto device_descriptor = ivars->device->CopyDeviceDescriptor();
    RETURN_TRAP(device_descriptor == NULL, false, EXIT_WITH_TARGET(kIOReturnIOError));
    os_log(OS_LOG_DEFAULT,
           "product: 0x%{public}04x" "   "
           "device class: 0x%{public}02x" "   "
           "manufacturer: 0x%{public}04x",
           device_descriptor->idProduct,
           device_descriptor->bDeviceClass,
           device_descriptor->idVendor);
    IOUSBHostFreeDescriptor(device_descriptor);
    
    ret = ivars->device->SetConfiguration(1, false);
    RETURN_TRAP(ret, kIOReturnSuccess, EXIT_UNSPEC_TARGET());
    os_log(OS_LOG_DEFAULT, "imagine if this just worked");
    
    auto config_descriptor = ivars->device->CopyConfigurationDescriptor(this);
    RETURN_TRAP(config_descriptor == NULL, false, EXIT_WITH_TARGET(kIOReturnIOError));
    
    
    uintptr_t iterator_ref;
    ret = ivars->device->CreateInterfaceIterator(&iterator_ref);
    RETURN_TRAP(ret, kIOReturnSuccess, EXIT_UNSPEC_TARGET());
    
    IOUSBHostInterface *curr_interface;
    
    while (!(ivars->control_data_interface && ivars->headset_interface)) {
        ret = ivars->device->CopyInterface(iterator_ref, &curr_interface);
        RETURN_TRAP(ret, kIOReturnSuccess, EXIT_UNSPEC_TARGET());
        RETURN_TRAP(curr_interface == NULL, false, EXIT_WITH_TARGET(kIOReturnUnsupported));
        
        switch (curr_interface->GetInterfaceDescriptor(config_descriptor)->bInterfaceProtocol) {
            case 0x01:
                ivars->control_data_interface = curr_interface;
                
                ret = ivars->control_data_interface->Open(this, 0, NULL);
                RETURN_TRAP(ret, kIOReturnSuccess, EXIT_WITH_TARGET(kIOReturnUnsupported));
                break;
                
            case 0x03:
                ivars->headset_interface = curr_interface;
                
                ret = ivars->headset_interface->Open(this, 0, NULL);
                RETURN_TRAP(ret, kIOReturnSuccess, EXIT_WITH_TARGET(kIOReturnUnsupported));
                break;
                
            default:
                break;
        }
        
    }
    
    ControlPipes cp = apply_control_data_interface_endpoints(config_descriptor, ivars->control_data_interface->GetInterfaceDescriptor(config_descriptor));
    
    ret = ivars->control_data_interface->CopyPipe(cp.input, &ivars->main_data_input.in_pipe);
    RETURN_TRAP(ret, kIOReturnSuccess, EXIT_WITH_TARGET(kIOReturnError));
    ret = ivars->control_data_interface->CreateIOBuffer(kIOMemoryDirectionIn, cp.input_max_packet_size, &ivars->main_data_input.in_data);
    RETURN_TRAP(ret, kIOReturnSuccess, EXIT_WITH_TARGET(kIOReturnError));
    
    ret = OSAction::Create(this, ControllerSupport360_ReadControllerInput_ID, IOUSBHostPipe_CompleteAsyncIO_ID, 0, &ivars->main_data_input.handler);
    RETURN_TRAP(ret, kIOReturnSuccess, EXIT_WITH_TARGET(kIOReturnError));
    
    ivars->main_data_input.max_packet_size = cp.input_max_packet_size;
    
    ret = ivars->main_data_input.in_pipe->AsyncIO(ivars->main_data_input.in_data, ivars->main_data_input.max_packet_size, ivars->main_data_input.handler, 0);
    
    return ret;
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
                os_log(OS_LOG_DEFAULT, "Expected 1 input endpoint (0x%{public}02x), but also got 0x%{public}02x", result.input, currEndpoint->bEndpointAddress);
            } else {
                os_log(OS_LOG_DEFAULT, "Input endpoint get! 0x%{public}02x", currEndpoint->bEndpointAddress);
            }
            result.input = currEndpoint->bEndpointAddress;
            // Storing this because it's annoying to get from the endpoint later.
            result.input_max_packet_size = currEndpoint->wMaxPacketSize;
        } else {
            if (result.output) {
                // 2 output endpoints on the first interface for some reason?
                os_log(OS_LOG_DEFAULT, "Expected 1 output endpoint (0x%{public}02x), but also got 0x%{public}02x", result.output, currEndpoint->bEndpointAddress);
            } else {
                os_log(OS_LOG_DEFAULT, "Output endpoint get! 0x%{public}02x", currEndpoint->bEndpointAddress);
            }
            result.output = currEndpoint->bEndpointAddress;
            // Storing this because it's annoying to get from the endpoint later.
            result.output_max_packet_size = currEndpoint->wMaxPacketSize;
        }
    }
    
    return result;
}

void IMPL(ControllerSupport360, ReadControllerInput) {
    kern_return_t ret;
    IOAddressSegment segment;
    
    if (!++ivars->main_data_input.iters) {
        uint64_t new_time = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
        uint64_t diff = new_time - ivars->main_data_input.prev_nanos;
        ivars->main_data_input.prev_nanos = new_time;
        uint64_t fps = 1000000000ull * 256ull / diff;
        os_log(OS_LOG_DEFAULT, "predicted FPS: 0x%{public}llu", fps);
    }
    
    // Get the address and length of the read data.
    ret = ivars->main_data_input.in_data->GetAddressRange(&segment);
    if (ret != kIOReturnSuccess) return;
    if ((void *) segment.address == NULL || segment.length < sizeof(ControllerData)) return;
    
    // C hack to copy the data to a pass-by-value struct correctly.
    ControllerData data = *(ControllerData *) (void *) (segment.address);
    
    ControllerState state = controller_data_to_state(data);

    
    
    ret = ivars->main_data_input.in_pipe->AsyncIO(ivars->main_data_input.in_data, ivars->main_data_input.max_packet_size, ivars->main_data_input.handler, 0);
    if (ret != kIOReturnSuccess) {
        free_ivars(this, ivars);
        Stop(this);
    }
    
}

kern_return_t IMPL(ControllerSupport360, Stop) {
    LOG_CURR_FUNC();
    
    kern_return_t ret = Stop(provider, SUPERDISPATCH);

    return ret;
}

void ControllerSupport360::free() {
    LOG_CURR_FUNC();
    free_ivars(this, ivars);
}

