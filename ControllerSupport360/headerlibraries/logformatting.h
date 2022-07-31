//
//  logformatting.h
//  ControllerSupport360App
//
//  Created by Skyler Calaman on 7/31/22.
//

#ifndef logformatting_h
#define logformatting_h

#include <stdlib.h>

#define FORMAT_AXIS_STRING "%{public}c0x%{public}04x"
#define SPREAD_AXIS(AXIS) AXIS < 0 ? '-' : ' ', abs(AXIS)

#define fmtPS "%{public}s"

#define BTN_STR(STATE, PROPERTY) STATE.PROPERTY ? #PROPERTY ", " : ""
#define DEC_BTN_STR(STATE, PROPERTY) const char *PROPERTY = FMT_BUTTON(STATE, PROPERTY)


#define BTN_STR_NST(STATE, PROPERTY, SUB_PROP) STATE.PROPERTY.SUB_PROP ? #PROPERTY ">" #SUB_PROP ", " : ""
#define DEC_BTN_STR_NST(STATE, PROPERTY, SUB_PROP) const char *PROPERTY ## _ ## SUB_PROP = FMT_BUTTON_NESTED(STATE, PROPERTY, SUB_PROP)


#endif /* logformatting_h */
