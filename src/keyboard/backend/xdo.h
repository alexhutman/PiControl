#pragma once

#include "model/mouse.h"
#include "model/protocol.h"

#include <xdo.h>

xdo_t *pictrl_xdo_backend_new();
void pictrl_xdo_backend_free(xdo_t *backend);

void picontrol_xdo_click_mouse(xdo_t *xdo, PiCtrlMouseBtnStatus status);
void picontrol_xdo_move_mouse_rel(xdo_t *xdo, PiCtrlMouseCoord coords);
void picontrol_xdo_print_str(xdo_t *xdo, const RawPiCtrlMessage *msg);
void picontrol_xdo_type_keysym(xdo_t *xdo, const RawPiCtrlMessage *msg);
