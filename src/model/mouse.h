#pragma once

typedef enum { PI_CTRL_MOUSE_LEFT = 0, PI_CTRL_MOUSE_RIGHT = 1 } PiCtrlMouseBtn;

typedef enum {
  PI_CTRL_MOUSE_UP = 0,
  PI_CTRL_MOUSE_DOWN = 1,
} PiCtrlMouseClick;

typedef struct {
  PiCtrlMouseBtn btn;
  PiCtrlMouseClick click;
} PiCtrlMouseBtnStatus;

typedef struct {
  int x, y;
} PiCtrlMouseCoord;
