#include "input.h"

#include <SDL.h>

#include "screen.h"

/* A touch is a tap if the finger wanders no further than this many pixels and
   lifts within this many milliseconds. */
#define TAP_MAX_PIXELS 12.0f
#define TAP_MAX_MS 300
/* How much one notch of the mouse wheel spreads the two pretend fingers. */
#define WHEEL_SPREAD 1.15f
/* The name the mouse goes by among the fingers. */
#define MOUSE_FINGER ((SDL_FingerID) - 2)
/* While nothing moves, the motion sensor is still looked at this often, in milliseconds. */
#define TURN_CHECK_MS 250
/* The Vita counts as upright when gravity pulls along the screen's long side
   at least this many times as hard as along its short side, and the other way
   round for the usual way of holding it. In between, at a slant, nothing
   changes, so the picture does not flip to and fro. */
#define TURN_CLEAR_RATIO 2.0f
/* Gravity must also pull along that side at least this hard, in metres per
   second squared; its full pull is 9.8. Lying flat, the Vita feels it along
   neither side, and the picture stays as it is. */
#define TURN_MIN_PULL 3.0f
/* Which way the sensor counts along the long side: 1 if it reads a positive
   number when the Vita's right end is up, -1 if a negative one. The Vita's
   sensor gives the direction gravity pulls in: held the usual way it reads
   about -9 along the short side, and with the right end up a negative number
   along the long side. */
#define TURN_SENSOR_SIGN -1

/* A finger on the screen. Two are followed; any more are ignored. */
typedef struct {
    int down;
    SDL_FingerID id;
    float x;
    float y;
} Finger;

static Finger fingers[2];
/* About the touch that began when the first finger came down: */
static Uint32 touch_start;
static float touch_travel; /* how far the fingers have moved, in pixels */
static int touch_moved;    /* 1 once it can no longer be a tap */

static SDL_Sensor *motion_sensor; /* NULL where there is none, as on a desktop */
static int turn;                  /* how the Vita is held; see input_turn */

void input_open(void)
{
    /* The Vita's buttons arrive through SDL's game controller interface. */
    if (SDL_NumJoysticks() > 0) {
        SDL_GameControllerOpen(0);
    }
    /* SDL also reports every touch as a mouse click. One report is enough. */
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");

    for (int i = 0; i < SDL_NumSensors(); i++) {
        if (SDL_SensorGetDeviceType(i) == SDL_SENSOR_ACCEL) {
            motion_sensor = SDL_SensorOpen(i);
            break;
        }
    }
    /* The sensor reports many times a second. Those reports are not wanted as
       events, which would keep the reader awake; it is asked when needed. With
       the events off, SDL no longer fetches the readings by itself, so
       turn_changed tells it to. */
    SDL_EventState(SDL_SENSORUPDATE, SDL_IGNORE);
    SDL_Log("Motion sensor: %s", motion_sensor != NULL ? "found" : "none");
}

int input_turn(void)
{
    return turn;
}

/* Asks the motion sensor how the Vita is held now. Returns 1 if that changed. */
static int turn_changed(void)
{
    float pull[3];
    if (motion_sensor == NULL) {
        return 0;
    }
    SDL_SensorUpdate();
    if (SDL_SensorGetData(motion_sensor, pull, 3) != 0) {
        return 0;
    }
    float along_long_side = SDL_fabsf(pull[0]);
    float along_short_side = SDL_fabsf(pull[1]);
    int now = turn;
    if (along_long_side > TURN_MIN_PULL && along_long_side > TURN_CLEAR_RATIO * along_short_side) {
        now = (pull[0] > 0 ? 1 : -1) * TURN_SENSOR_SIGN;
    } else if (along_short_side > TURN_MIN_PULL &&
               along_short_side > TURN_CLEAR_RATIO * along_long_side) {
        now = 0;
    }
    if (now == turn) {
        return 0;
    }
    SDL_Log("The Vita is held %s: the sensor reads %.1f, %.1f, %.1f",
            now == 0  ? "the usual way"
            : now > 0 ? "upright, right end up"
                      : "upright, left end up",
            pull[0], pull[1], pull[2]);
    turn = now;
    return 1;
}

static Action action_for_key(SDL_Keycode key)
{
    switch (key) {
    case SDLK_RIGHT:
        return ACTION_RIGHT;
    case SDLK_LEFT:
        return ACTION_LEFT;
    case SDLK_UP:
        return ACTION_UP;
    case SDLK_SPACE:
    case SDLK_RETURN:
        return ACTION_CONFIRM;
    case SDLK_BACKSPACE:
        return ACTION_BACK;
    case SDLK_ESCAPE:
        return ACTION_QUIT;
    default:
        return ACTION_NONE;
    }
}

/* SDL names buttons after an Xbox pad: its A is the Vita's Cross, B is Circle
   and Y is Triangle. */
static Action action_for_button(Uint8 button)
{
    switch (button) {
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
        return ACTION_RIGHT;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
        return ACTION_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
    case SDL_CONTROLLER_BUTTON_Y:
        return ACTION_UP;
    case SDL_CONTROLLER_BUTTON_A:
    case SDL_CONTROLLER_BUTTON_START:
        return ACTION_CONFIRM;
    case SDL_CONTROLLER_BUTTON_B:
        return ACTION_BACK;
    default:
        return ACTION_NONE;
    }
}

static Input plain(Action action)
{
    Input input = {action, 0, 0, 0, 0, 1};
    return input;
}

/* The Tab key, in a desktop window: as if the Vita were turned on by a quarter. */
static Input pretend_turn(void)
{
    turn = turn == 0 ? 1 : turn == 1 ? -1 : 0;
    return plain(ACTION_TURN);
}

static Finger *find_finger(SDL_FingerID id)
{
    for (int i = 0; i < 2; i++) {
        if (fingers[i].down && fingers[i].id == id) {
            return &fingers[i];
        }
    }
    return NULL;
}

static int fingers_down(void)
{
    return fingers[0].down + fingers[1].down;
}

/* The point halfway between the fingers that are down, and how far apart they are. */
static void measure_fingers(float *x, float *y, float *apart)
{
    const Finger *a = fingers[0].down ? &fingers[0] : &fingers[1];
    const Finger *b = fingers[1].down ? &fingers[1] : &fingers[0];
    *x = (a->x + b->x) / 2;
    *y = (a->y + b->y) / 2;
    *apart = SDL_sqrtf((a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y));
}

static Input finger_down(SDL_FingerID id, float x, float y)
{
    Finger *finger = !fingers[0].down ? &fingers[0] : !fingers[1].down ? &fingers[1] : NULL;
    if (finger == NULL || find_finger(id) != NULL) {
        return plain(ACTION_NONE);
    }
    if (fingers_down() == 0) {
        touch_start = SDL_GetTicks();
        touch_travel = 0;
        touch_moved = 0;
    } else {
        /* A second finger: this is no tap. */
        touch_moved = 1;
    }
    *finger = (Finger){1, id, x, y};
    return plain(ACTION_NONE);
}

static Input finger_moved(SDL_FingerID id, float x, float y)
{
    Finger *finger = find_finger(id);
    if (finger == NULL) {
        return plain(ACTION_NONE);
    }
    float old_x, old_y, old_apart;
    measure_fingers(&old_x, &old_y, &old_apart);
    finger->x = x;
    finger->y = y;
    Input input = plain(ACTION_NONE);
    float apart;
    measure_fingers(&input.x, &input.y, &apart);
    input.dx = input.x - old_x;
    input.dy = input.y - old_y;

    touch_travel += SDL_fabsf(input.dx) + SDL_fabsf(input.dy);
    if (touch_travel > TAP_MAX_PIXELS) {
        touch_moved = 1;
    }
    if (!touch_moved) {
        /* Still close enough to where it came down to become a tap. */
        return plain(ACTION_NONE);
    }
    if (fingers_down() == 2) {
        input.action = ACTION_PINCH;
        /* Fingers that touch each other give no useful distance. */
        input.spread = old_apart > 1 ? apart / old_apart : 1;
    } else {
        input.action = ACTION_DRAG;
    }
    return input;
}

static Input finger_up(SDL_FingerID id)
{
    Finger *finger = find_finger(id);
    if (finger == NULL) {
        return plain(ACTION_NONE);
    }
    finger->down = 0;
    if (fingers_down() > 0) {
        /* The other finger carries on, as a drag. */
        return plain(ACTION_NONE);
    }
    int is_tap = !touch_moved && SDL_GetTicks() - touch_start <= TAP_MAX_MS;
    Input input = plain(is_tap ? ACTION_TAP : ACTION_RELEASE);
    input.x = finger->x;
    input.y = finger->y;
    return input;
}

/* Is this a screen that is touched where the picture is? The Vita's rear
   touch pad and a laptop's trackpad are not, and are left out. */
static int is_touch_screen(SDL_TouchID touch)
{
    SDL_TouchDeviceType type = SDL_GetTouchDeviceType(touch);
    return type != SDL_TOUCH_DEVICE_INDIRECT_ABSOLUTE && type != SDL_TOUCH_DEVICE_INDIRECT_RELATIVE;
}

static Input input_for_wheel(int notches)
{
    int x, y;
    SDL_GetMouseState(&x, &y);
    Input input = plain(ACTION_PINCH);
    input.x = (float)x;
    input.y = (float)y;
    input.spread = SDL_powf(WHEEL_SPREAD, (float)notches);
    return input;
}

Input input_next(int something_is_moving)
{
    if (turn_changed()) {
        return plain(ACTION_TURN);
    }
    SDL_Event event;
    int got_event;
    if (something_is_moving) {
        got_event = SDL_PollEvent(&event);
    } else if (motion_sensor != NULL) {
        /* Wake up now and then to look at the motion sensor. */
        got_event = SDL_WaitEventTimeout(&event, TURN_CHECK_MS);
    } else {
        got_event = SDL_WaitEvent(&event);
    }
    if (!got_event) {
        return plain(ACTION_NONE);
    }
    switch (event.type) {
    case SDL_QUIT:
        return plain(ACTION_QUIT);
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_TAB) {
            return pretend_turn();
        }
        return plain(action_for_key(event.key.keysym.sym));
    case SDL_CONTROLLERBUTTONDOWN:
        return plain(action_for_button(event.cbutton.button));

    /* SDL gives a finger's place as a share of the screen, from 0 to 1. */
    case SDL_FINGERDOWN:
    case SDL_FINGERMOTION:
    case SDL_FINGERUP: {
        if (!is_touch_screen(event.tfinger.touchId)) {
            return plain(ACTION_NONE);
        }
        float x = event.tfinger.x * SCREEN_WIDTH;
        float y = event.tfinger.y * SCREEN_HEIGHT;
        if (event.type == SDL_FINGERDOWN) {
            return finger_down(event.tfinger.fingerId, x, y);
        }
        if (event.type == SDL_FINGERMOTION) {
            return finger_moved(event.tfinger.fingerId, x, y);
        }
        return finger_up(event.tfinger.fingerId);
    }

    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
            return finger_down(MOUSE_FINGER, (float)event.button.x, (float)event.button.y);
        }
        return plain(ACTION_NONE);
    case SDL_MOUSEMOTION:
        return finger_moved(MOUSE_FINGER, (float)event.motion.x, (float)event.motion.y);
    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT) {
            return finger_up(MOUSE_FINGER);
        }
        return plain(ACTION_NONE);
    case SDL_MOUSEWHEEL:
        return input_for_wheel(event.wheel.y);

    default:
        return plain(ACTION_NONE);
    }
}
