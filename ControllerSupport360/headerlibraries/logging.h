//
//  Logging.h
//  ControllerSupport360App
//
//  Created by Skyler Calaman on 7/31/22.
//

#ifndef Logging_h
#define Logging_h

#include <os/log.h>
#include "logformatting.h"
#include "sharedstructs.h"

#ifdef DEBUG

#define DEBUG_LOG 1
#define RELEASE_LOG 1

#else /* #ifdef DEBUG */

#define DEBUG_LOG 0
#define RELEASE_LOG 1

#endif /* #ifdef DEBUG */


#define level_aware_log(LEVEL, ...) if (LEVEL) { os_log(OS_LOG_DEFAULT, __VA_ARGS__); }


void log_controller_state(ControllerState state) {
//    DECL_BUTTON(state, r3);
//    DECL_BUTTON(state, l3);
//
//    DECL_BUTTON(state, back);
//    DECL_BUTTON(state, start);
//
//    DECL_BUTTON_NESTED(state, dpad, dpadr);
//    DECL_BUTTON_NESTED(state, dpad, dpadl);
//    DECL_BUTTON_NESTED(state, dpad, dpadd);
//    DECL_BUTTON_NESTED(state, dpad, dpadu);
//
//    DECL_BUTTON_NESTED(state, main_buttons, y);
//    DECL_BUTTON_NESTED(state, main_buttons, x);
//    DECL_BUTTON_NESTED(state, main_buttons, b);
//    DECL_BUTTON_NESTED(state, main_buttons, a);
//
//    DECL_BUTTON(state, xbox);
//
//    DECL_BUTTON(state, r_bump);
//    DECL_BUTTON(state, l_bump);
    
    
    
//    level_aware_log(
//           DEBUG_LOG,
//           fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS,
//           r3, l3,
//           back, start,
//           dpad_dpadr, dpad_dpadl, dpad_dpadd, dpad_dpadu,
//           main_buttons_y, main_buttons_x, main_buttons_b, main_buttons_a,
//           xbox,
//           r_bump, l_bump);
    level_aware_log(
           DEBUG_LOG,
           fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS fmtPS,
           
           BTN_STR(state, l3), BTN_STR(state, r3),
           
           BTN_STR(state, back), BTN_STR(state, start),
                    
           BTN_STR_NST(state, dpad, dpadu), BTN_STR_NST(state, dpad, dpadd),
                    BTN_STR_NST(state, dpad, dpadl), BTN_STR_NST(state, dpad, dpadr),
           BTN_STR_NST(state, main_buttons, a), BTN_STR_NST(state, main_buttons, b),
                    BTN_STR_NST(state, main_buttons, x), BTN_STR_NST(state, main_buttons, y),
           
           BTN_STR(state, xbox),
           
           BTN_STR(state, l_bump), BTN_STR(state, r_bump));
    
    
    
    
    level_aware_log(
           DEBUG_LOG,
           "lt: 0x%{public}02x   rt: 0x%{public}02x   "
                    "ljoy: (" FORMAT_AXIS_STRING ", " FORMAT_AXIS_STRING ")   "
                    "rjoy: (" FORMAT_AXIS_STRING ", " FORMAT_AXIS_STRING ")   ",
           state.l_trigger,
           state.r_trigger,
           SPREAD_AXIS(state.l_joystick.x), SPREAD_AXIS(state.l_joystick.y),
           SPREAD_AXIS(state.r_joystick.x), SPREAD_AXIS(state.r_joystick.y));
}


#endif /* Logging_h */
