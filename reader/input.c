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

void input_open(void)
{
    /* The Vita's buttons arrive through SDL's game controller interface. */
    if (SDL_NumJoysticks() > 0) {
        SDL_GameControllerOpen(0);
    }
    /* SDL also reports every touch as a mouse click. One report is enough. */
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
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
    SDL_Event event;
    int got_event = something_is_moving ? SDL_PollEvent(&event) : SDL_WaitEvent(&event);
    if (!got_event) {
        return plain(ACTION_NONE);
    }
    switch (event.type) {
    case SDL_QUIT:
        return plain(ACTION_QUIT);
    case SDL_KEYDOWN:
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
