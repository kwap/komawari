/* Turns key presses, button presses and touches into the few things a user can ask for.

   Touches come from the Vita's front screen. In a desktop window the mouse
   stands in for them: holding the left button is one finger, and the wheel
   spreads or pinches two fingers under the pointer. */
#ifndef KOMAWARI_INPUT_H
#define KOMAWARI_INPUT_H

typedef enum {
    ACTION_NONE,    /* nothing the reader cares about */
    ACTION_RIGHT,   /* Right arrow        | D-pad right or R */
    ACTION_LEFT,    /* Left arrow         | D-pad left or L */
    ACTION_UP,      /* Up arrow           | D-pad up or Triangle */
    ACTION_CONFIRM, /* Space or Enter     | Cross or START */
    ACTION_BACK,    /* Backspace          | Circle */
    ACTION_QUIT,    /* Escape, or the window was closed */
    ACTION_TAP,     /* a short touch that stayed in one place */
    ACTION_DRAG,    /* one finger moved across the screen */
    ACTION_PINCH,   /* two fingers moved */
    ACTION_RELEASE  /* the last finger was lifted after a drag or a pinch */
} Action;

/* One thing the user did. Everything after action is for touches only.
   Places and distances are in screen pixels. */
typedef struct {
    Action action;
    float x; /* where the finger is; for a pinch, the point halfway between the two */
    float y;
    float dx; /* how far that point moved since the last drag or pinch */
    float dy;
    float spread; /* pinch: the distance between the fingers, divided by what it was before */
} Input;

/* Starts listening to the Vita's buttons. Call once, after SDL_Init. */
void input_open(void);

/* Returns the next thing the user did.
   While something on screen is moving it returns at once, with ACTION_NONE if
   nothing happened, so the caller can draw the next frame. When nothing is
   moving it sleeps until something happens, which saves battery. */
Input input_next(int something_is_moving);

#endif
