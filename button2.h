#ifndef BUTTON_H
#define BUTTON_H

//#include "utils.h"
#define assert(test, str1, str2)

char btn_str[120];

#define DEBOUNCE_INTERVAL_MS (40)             // Unstable/debounce duration
#define BTN_LONG_PRESS_THRESHOLD_MS (500)     // Below that it's considered a SHORT press; above it a LONG press

enum scene_fsm_state {
  SCENE_FSM_IDLE,
  SCENE_FSM_ARMED,
  SCENE_FSM_POST_SHORT,
  SCENE_FSM_LONG,
};
static const char *scene_fsm_state_names[] = {
  "SCENE_FSM_IDLE",
  "SCENE_FSM_ARMED",
  "SCENE_FSM_POST_SHORT",
  "SCENE_FSM_LONG",
};

enum scene {
  SCENE_NONE,
  SCENE_ARMED,
  SCENE_SINGLE_CLICK,
  SCENE_SINGLE_CLICK_DONE,
  SCENE_DOUBLE_CLICK,
  SCENE_DOUBLE_CLICK_DONE,
  SCENE_TRIPLE_CLICK,
  SCENE_TRIPLE_CLICK_DONE,
  SCENE_LONG_CLICK,
  SCENE_UNDEF,
};
static const char *scene_names[] = {
  "SCENE_NONE",
  "SCENE_ARMED",
  "SCENE_SINGLE_CLICK",
  "SCENE_SINGLE_CLICK_DONE",
  "SCENE_DOUBLE_CLICK",
  "SCENE_DOUBLE_CLICK_DONE",
  "SCENE_TRIPLE_CLICK",
  "SCENE_TRIPLE_CLICK_DONE",
  "SCENE_LONG_CLICK",
  "SCENE_UNDEF",
};

struct button {
  unsigned long last_change_ms;
  unsigned long action_start_time;
  enum scene_fsm_state scene_fsm;

  volatile uint8_t *pin_port;
  uint8_t pin_bit;
  uint8_t clicks;
  bool state;
  bool unstable_state;

  void setup(volatile uint8_t *pin_port, uint8_t pin_bit) {
    this->pin_port = pin_port;
    this->pin_bit = pin_bit;
    state = unstable_state = 0;
    clicks = 0;
    last_change_ms = millis();
  }

  void change_fsm_state(enum scene_fsm_state st) {
#if 0
    snprintf(btn_str, sizeof(btn_str), "%lu\t\t%s --> %s", millis(), scene_fsm_state_names[scene_fsm], scene_fsm_state_names[st]);
    Serial.println(btn_str);
#endif
    scene_fsm = st;
  }

  enum scene read_state() {
    bool read_val = !(*pin_port & _BV(pin_bit));

    if (read_val != unstable_state) {
      unstable_state = read_val;
      last_change_ms = millis();
    } else {
      if (read_val != state && (millis() - last_change_ms > DEBOUNCE_INTERVAL_MS)) {
        state = read_val;
      }
    }

    switch(scene_fsm) {
      case SCENE_FSM_IDLE:
        if (state) {
          action_start_time = millis();
          change_fsm_state(SCENE_FSM_ARMED);
          clicks = 0;
          return SCENE_ARMED;
        }
        return SCENE_NONE;

      case SCENE_FSM_ARMED:
        if (!state) { // short click
          change_fsm_state(SCENE_FSM_POST_SHORT);
          action_start_time = millis();
          return (enum scene)(clicks < 3 ? SCENE_SINGLE_CLICK + 2*clicks : SCENE_UNDEF);
        } else if ((millis() - action_start_time) >= BTN_LONG_PRESS_THRESHOLD_MS) { // long click
          change_fsm_state(SCENE_FSM_LONG);
          return SCENE_LONG_CLICK;
        }
        return SCENE_NONE;

      case SCENE_FSM_POST_SHORT:
        if (state) {
          action_start_time = millis();
          change_fsm_state(SCENE_FSM_ARMED);
          clicks++;
          return SCENE_ARMED;
        } else if ((millis() - action_start_time) >= BTN_LONG_PRESS_THRESHOLD_MS) {
          change_fsm_state(SCENE_FSM_IDLE);
          return (enum scene)(clicks < 3 ? SCENE_SINGLE_CLICK_DONE + 2*clicks : SCENE_UNDEF);
        }
        return SCENE_NONE;

      case SCENE_FSM_LONG:
        if (state) {
          return SCENE_LONG_CLICK;
        }
        change_fsm_state(SCENE_FSM_IDLE);
        return SCENE_NONE;
    }
    assert(false, "invalid scene_fsm state", scene_fsm);
  }
};

#endif

