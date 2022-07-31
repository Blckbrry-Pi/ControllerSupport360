//
//  ControllerSupport360.cpp
//  ControllerSupport360
//
//  Created by Skyler Calaman on 6/23/22.
//

#include "ControllerSupport360.h"
#include "ControllerDevice.h"

#include <os/log.h>
#include "logging.h"

#include "sharedstructs.h"
#include "usbutils.h"

#include <DriverKit/IOUserServer.h>
#include <DriverKit/IOLib.h>
#include <USBDriverKit/IOUSBHostInterface.h>
#include <USBDriverKit/USBDriverKitDefs.h>
#include <USBDriverKit/AppleUSBDescriptorParsing.h>


#define LOG_CURR_FUNC(LEVEL) level_aware_log(LEVEL, "%{public}s", __FUNCTION__)

#define RETURN_TRAP(VARIABLE, TARG_VALUE, STATEMENT) do { \
    if (VARIABLE != TARG_VALUE) { \
        STATEMENT; \
        level_aware_log(DEBUG_LOG, "%{public}s on line: %{public}d", "returning...", __LINE__); \
        return VARIABLE; \
    }\
} while (0)


#define STOP Stop(provider, SUPERDISPATCH);
#define FREE_CONTROL_INPUT_PIPE

#define EXIT_WITH_TARGET(TARGET) { \
    free_ivars(this, ivars); \
    Stop(this); \
    level_aware_log(DEBUG_LOG, "%{public}s on line: %{public}d", "returning...", __LINE__); \
    return TARGET; \
}

#define EXIT_UNSPEC_TARGET() { \
    free_ivars(this, ivars); \
    Stop(this); \
}


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


struct ControllerSupport360_IVars {
    IOUSBHostDevice *device;
    
    IOUSBHostInterface *control_data_interface;
    IOUSBHostInterface *headset_interface;
    
    ControlDataInput main_data_input;
    
    IOUserHIDEventService *hid_device;
};





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
    
    LOG_CURR_FUNC(DEBUG_LOG);
    
    result = super::init();
    RETURN_TRAP(result, true, {});
    
    ivars = IONewZero(ControllerSupport360_IVars, 1);
    RETURN_TRAP(ivars == NULL, false, {});
    
    return result;
}

kern_return_t IMPL(ControllerSupport360, Start) {
    kern_return_t ret;
    
    
    LOG_CURR_FUNC(RELEASE_LOG);
    
    ret = Start(provider, SUPERDISPATCH);
    RETURN_TRAP(ret, kIOReturnSuccess, Stop(provider, SUPERDISPATCH));
        
    ivars->device = OSDynamicCast(IOUSBHostDevice, provider);
    RETURN_TRAP(ivars->device == NULL, false, EXIT_WITH_TARGET(kIOReturnNoDevice));
    
    ret = ivars->device->Open(this, 0, NULL);
    RETURN_TRAP(ret, kIOReturnSuccess, Stop(provider, SUPERDISPATCH));
    
    auto device_descriptor = ivars->device->CopyDeviceDescriptor();
    RETURN_TRAP(device_descriptor == NULL, false, EXIT_WITH_TARGET(kIOReturnIOError));
    level_aware_log(DEBUG_LOG,
           "product: 0x%{public}04x" "   "
           "device class: 0x%{public}02x" "   "
           "manufacturer: 0x%{public}04x",
           device_descriptor->idProduct,
           device_descriptor->bDeviceClass,
           device_descriptor->idVendor);
    IOUSBHostFreeDescriptor(device_descriptor);
    
    ret = ivars->device->SetConfiguration(1, false);
    RETURN_TRAP(ret, kIOReturnSuccess, EXIT_UNSPEC_TARGET());
    
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
    RETURN_TRAP(ret, kIOReturnSuccess, EXIT_WITH_TARGET(kIOReturnError));
    
    
    ret = RegisterService();
    RETURN_TRAP(ret, kIOReturnSuccess, EXIT_UNSPEC_TARGET());
    
    return kIOReturnSuccess;
}




void IMPL(ControllerSupport360, ReadControllerInput) {
    kern_return_t ret;
    IOAddressSegment segment;
    
//    if (!ivars->hid_device) NewUserClient();
    
//    if (!++ivars->main_data_input.iters) {
//        uint64_t new_time = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
//        uint64_t diff = new_time - ivars->main_data_input.prev_nanos;
//        ivars->main_data_input.prev_nanos = new_time;
//        uint64_t fps = 1000000000ull * 256ull / diff;
//        os_log(OS_LOG_DEFAULT, "predicted FPS: 0x%{public}llu", fps);
//    }
    
    // Get the address and length of the read data.
    ret = ivars->main_data_input.in_data->GetAddressRange(&segment);
    if (ret != kIOReturnSuccess) return;
    if ((void *) segment.address == NULL || segment.length < sizeof(ControllerData)) return;
    
    // C hack to copy the data to a pass-by-value struct correctly.
    ControllerData data = *(ControllerData *) (void *) (segment.address);
    
    ControllerState state = controller_data_to_state(data);

    log_controller_state(state);
    
    ret = ivars->main_data_input.in_pipe->AsyncIO(ivars->main_data_input.in_data, ivars->main_data_input.max_packet_size, ivars->main_data_input.handler, 0);
    if (ret != kIOReturnSuccess) {
        free_ivars(this, ivars);
        Stop(this);
    }
    
}

kern_return_t IMPL(ControllerSupport360, Stop) {
    LOG_CURR_FUNC(RELEASE_LOG);
    
    kern_return_t ret = STOP;

    return ret;
}

void ControllerSupport360::free() {
    LOG_CURR_FUNC(DEBUG_LOG);
    free_ivars(this, ivars);
    super::free();
}



kern_return_t IMPL(ControllerSupport360, NewUserClient) {
    kern_return_t ret;
    IOService *child_service;
    IOUserHIDEventService *controller_hid_system_interface;
    
    level_aware_log(DEBUG_LOG, "imagine if this just worked 2 %{public}s", __FUNCTION__);
    
    ret = Create(this, "GamepadClientProperties", &child_service);
    RETURN_TRAP(ret, kIOReturnSuccess, EXIT_WITH_TARGET(ret));

    controller_hid_system_interface = OSDynamicCast(IOUserHIDEventService, child_service);
    RETURN_TRAP(controller_hid_system_interface == NULL, false, {
        child_service->release();
        free_ivars(this, ivars);
        Stop(this);
    });

    ivars->hid_device = controller_hid_system_interface;

    return kIOReturnSuccess;
}
