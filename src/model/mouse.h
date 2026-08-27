#pragma once

typedef enum { PI_CTRL_MOUSE_LEFT = 0, PI_CTRL_MOUSE_RIGHT = 1 } MouseButton;

typedef enum {
  PI_CTRL_MOUSE_UP = 0,
  PI_CTRL_MOUSE_DOWN = 1,
} MouseClick;

typedef struct {
  MouseButton btn;
  MouseClick click;
} MouseBtnStatus;

typedef struct {
  int x, y;
} MouseCoord;
