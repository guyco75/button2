#ifndef BUTTON_H
#define BUTTON_H

#define DEBOUNCE_INTERVAL_MS (40)             // Unstable/debounce duration
#define BTN_LONG_PRESS_THRESHOLD_MS (500)     // Below that it's considered a SHORT press; above it a LONG press

enum button_fsm_state {
  BUTTON_FSM_IDLE,
  BUTTON_FSM_ARMED,
  BUTTON_FSM_POST_SHORT,
  BUTTON_FSM_LONG,
};
static const char *button_fsm_state_names[] = {
  "BUTTON_FSM_IDLE",
  "BUTTON_FSM_ARMED",
  "BUTTON_FSM_POST_SHORT",
  "BUTTON_FSM_LONG",
};

enum button_event {
  BUTTON_EVENT_NONE,
  BUTTON_EVENT_ARMED,
  BUTTON_EVENT_SINGLE_CLICK,
  BUTTON_EVENT_SINGLE_CLICK_DONE,
  BUTTON_EVENT_DOUBLE_CLICK,
  BUTTON_EVENT_DOUBLE_CLICK_DONE,
  BUTTON_EVENT_TRIPLE_CLICK,
  BUTTON_EVENT_TRIPLE_CLICK_DONE,
  BUTTON_EVENT_LONG_CLICK,
  BUTTON_EVENT_UNDEF,
};
static const char *event_names[] = {
  "BUTTON_EVENT_NONE",
  "BUTTON_EVENT_ARMED",
  "BUTTON_EVENT_SINGLE_CLICK",
  "BUTTON_EVENT_SINGLE_CLICK_DONE",
  "BUTTON_EVENT_DOUBLE_CLICK",
  "BUTTON_EVENT_DOUBLE_CLICK_DONE",
  "BUTTON_EVENT_TRIPLE_CLICK",
  "BUTTON_EVENT_TRIPLE_CLICK_DONE",
  "BUTTON_EVENT_LONG_CLICK",
  "BUTTON_EVENT_UNDEF",
};

struct button {
  unsigned long last_change_ms;
  unsigned long action_start_time;
  enum button_fsm_state fsm_state;
  enum button_event last_event;

  volatile uint8_t *pin_port;
  uint8_t pin_bit;
  uint8_t clicks;
  bool pin_state;
  bool unstable_pin_state;

  void setup(uint8_t btn_pin) {
    this->pin_port = portInputRegister(digitalPinToPort(btn_pin));
    this->pin_bit = digitalPinToBitMask(btn_pin);
    pinMode(btn_pin, INPUT_PULLUP);

    pin_state = unstable_pin_state = 0;
    clicks = 0;
    last_change_ms = millis();
  }

  void change_fsm_state(enum button_fsm_state st) {
#if 0
    snprintf(btn_str, sizeof(btn_str), "%lu\t\t%s --> %s", millis(), button_fsm_state_names[fsm_state], button_fsm_state_names[st]);
    Serial.println(btn_str);
#endif
    fsm_state = st;
  }

  enum button_event fsm() {
    bool read_val = !(*pin_port & pin_bit);

    if (read_val != unstable_pin_state) {
      unstable_pin_state = read_val;
      last_change_ms = millis();
    } else {
      if (read_val != pin_state && (millis() - last_change_ms > DEBOUNCE_INTERVAL_MS)) {
        pin_state = read_val;
      }
    }

    switch(fsm_state) {
      case BUTTON_FSM_IDLE:
        if (pin_state) {
          action_start_time = millis();
          change_fsm_state(BUTTON_FSM_ARMED);
          clicks = 0;
          return BUTTON_EVENT_ARMED;
        }
        return BUTTON_EVENT_NONE;

      case BUTTON_FSM_ARMED:
        if (!pin_state) { // short click
          change_fsm_state(BUTTON_FSM_POST_SHORT);
          action_start_time = millis();
          return (enum button_event)(clicks < 3 ? BUTTON_EVENT_SINGLE_CLICK + 2*clicks : BUTTON_EVENT_UNDEF);
        } else if ((millis() - action_start_time) >= BTN_LONG_PRESS_THRESHOLD_MS) { // long click
          change_fsm_state(BUTTON_FSM_LONG);
          return BUTTON_EVENT_LONG_CLICK;
        }
        return BUTTON_EVENT_NONE;

      case BUTTON_FSM_POST_SHORT:
        if (pin_state) {
          action_start_time = millis();
          change_fsm_state(BUTTON_FSM_ARMED);
          clicks++;
          return BUTTON_EVENT_ARMED;
        } else if ((millis() - action_start_time) >= BTN_LONG_PRESS_THRESHOLD_MS) {
          change_fsm_state(BUTTON_FSM_IDLE);
          return (enum button_event)(clicks < 3 ? BUTTON_EVENT_SINGLE_CLICK_DONE + 2*clicks : BUTTON_EVENT_UNDEF);
        }
        return BUTTON_EVENT_NONE;

      case BUTTON_FSM_LONG:
        if (pin_state) {
          return BUTTON_EVENT_LONG_CLICK;
        }
        change_fsm_state(BUTTON_FSM_IDLE);
        return BUTTON_EVENT_NONE;
    }
  }
};

#endif

