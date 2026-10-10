/* Turns key presses and button presses into the few things a user can ask for. */
#ifndef KOMAWARI_INPUT_H
#define KOMAWARI_INPUT_H

typedef enum {
    ACTION_NONE,    /* nothing the reader cares about */
    ACTION_RIGHT,   /* Right arrow        | D-pad right or R */
    ACTION_LEFT,    /* Left arrow         | D-pad left or L */
    ACTION_UP,      /* Up arrow           | D-pad up or Triangle */
    ACTION_CONFIRM, /* Space or Enter     | Cross or START */
    ACTION_BACK,    /* Backspace          | Circle */
    ACTION_QUIT     /* Escape, or the window was closed */
} Action;

/* Starts listening to the Vita's buttons. Call once, after SDL_Init. */
void input_open(void);

/* Returns the next thing the user asked for.
   While something on screen is moving it returns at once, with ACTION_NONE if
   nothing was pressed, so the caller can draw the next frame. When nothing is
   moving it sleeps until a key or button is pressed, which saves battery. */
Action input_next(int something_is_moving);

#endif
