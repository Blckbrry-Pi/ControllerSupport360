//
//  sharedstructs.h
//  ControllerSupport360App
//
//  Created by Skyler Calaman on 7/31/22.
//

#ifndef sharedstructs_h
#define sharedstructs_h


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

typedef struct JoystickState {
    int16_t x;
    int16_t y;
} JoystickState;

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
    
    uint8_t l_trigger;
    uint8_t r_trigger;
    
    JoystickState l_joystick;
    JoystickState r_joystick;
} ControllerState;

typedef struct ControllerData { unsigned char data[0x14]; } ControllerData;


typedef struct ControlPipes {
    uint8_t input;
    uint16_t input_max_packet_size;
    uint8_t output;
    uint16_t output_max_packet_size;
} ControlPipes;

#endif /* sharedstructs_h */
