#include "input.h"

#include <SDL.h>

void input_open(void)
{
    /* The Vita's buttons arrive through SDL's game controller interface. */
    if (SDL_NumJoysticks() > 0) {
        SDL_GameControllerOpen(0);
    }
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

Action input_next(int something_is_moving)
{
    SDL_Event event;
    int got_event = something_is_moving ? SDL_PollEvent(&event) : SDL_WaitEvent(&event);
    if (!got_event) {
        return ACTION_NONE;
    }
    switch (event.type) {
    case SDL_QUIT:
        return ACTION_QUIT;
    case SDL_KEYDOWN:
        return action_for_key(event.key.keysym.sym);
    case SDL_CONTROLLERBUTTONDOWN:
        return action_for_button(event.cbutton.button);
    default:
        return ACTION_NONE;
    }
}
