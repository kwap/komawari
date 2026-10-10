#include "draw.h"

#include "screen.h"

#define PROGRESS_BAR_HEIGHT 4.0f
#define SQUARE_SIZE 8.0f
#define SQUARE_GAP 8.0f

void draw_fill(SDL_Renderer *renderer, SDL_FRect area, Uint8 grey, Uint8 alpha)
{
    if (area.w <= 0 || area.h <= 0 || alpha == 0) {
        return;
    }
    SDL_SetRenderDrawColor(renderer, grey, grey, grey, alpha);
    SDL_RenderFillRectF(renderer, &area);
}

void draw_dim_outside(SDL_Renderer *renderer, SDL_FRect lit, Uint8 alpha)
{
    float right = lit.x + lit.w;
    float bottom = lit.y + lit.h;
    /* Four bands around the lit rectangle: above, below, left and right. */
    draw_fill(renderer, (SDL_FRect){0, 0, SCREEN_WIDTH, lit.y}, 0, alpha);
    draw_fill(renderer, (SDL_FRect){0, bottom, SCREEN_WIDTH, SCREEN_HEIGHT - bottom}, 0, alpha);
    draw_fill(renderer, (SDL_FRect){0, lit.y, lit.x, lit.h}, 0, alpha);
    draw_fill(renderer, (SDL_FRect){right, lit.y, SCREEN_WIDTH - right, lit.h}, 0, alpha);
}

void draw_squares(SDL_Renderer *renderer, int count, int current, Uint8 alpha)
{
    if (count < 2) {
        return;
    }
    float row_width = (float)count * SQUARE_SIZE + (float)(count - 1) * SQUARE_GAP;
    float x = (SCREEN_WIDTH - row_width) / 2;
    float y = SCREEN_HEIGHT - PROGRESS_BAR_HEIGHT - 12 - SQUARE_SIZE;

    /* A dark plate behind the squares keeps them visible on white paper. */
    SDL_FRect plate = {x - 8, y - 6, row_width + 16, SQUARE_SIZE + 12};
    draw_fill(renderer, plate, 0, (Uint8)(alpha / 2));

    for (int i = 0; i < count; i++) {
        SDL_FRect square = {x + (float)i * (SQUARE_SIZE + SQUARE_GAP), y, SQUARE_SIZE, SQUARE_SIZE};
        draw_fill(renderer, square, 255, i == current ? alpha : (Uint8)(alpha / 3));
    }
}

void draw_cross_button(SDL_Renderer *renderer, float centre_x, float centre_y, float radius,
                       Uint8 alpha)
{
    /* The ring is drawn as a many-sided shape, the X as two strokes. Each is
       drawn twice, one pixel apart, to make the lines two pixels thick. */
    enum { SIDES = 32 };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
    for (int thickness = 0; thickness < 2; thickness++) {
        float r = radius - (float)thickness;
        SDL_FPoint ring[SIDES + 1];
        for (int i = 0; i <= SIDES; i++) {
            float angle = (float)i * 2.0f * (float)M_PI / SIDES;
            ring[i] = (SDL_FPoint){centre_x + r * SDL_cosf(angle), centre_y + r * SDL_sinf(angle)};
        }
        SDL_RenderDrawLinesF(renderer, ring, SIDES + 1);

        float arm = radius * 0.42f;
        float shift = (float)thickness;
        SDL_RenderDrawLineF(renderer, centre_x - arm + shift, centre_y - arm,
                            centre_x + arm + shift, centre_y + arm);
        SDL_RenderDrawLineF(renderer, centre_x - arm + shift, centre_y + arm,
                            centre_x + arm + shift, centre_y - arm);
    }
}

void draw_progress_bar(SDL_Renderer *renderer, float done, Uint8 alpha)
{
    float y = SCREEN_HEIGHT - PROGRESS_BAR_HEIGHT;
    draw_fill(renderer, (SDL_FRect){0, y, SCREEN_WIDTH, PROGRESS_BAR_HEIGHT}, 0,
              (Uint8)(alpha / 2));
    draw_fill(renderer, (SDL_FRect){0, y, SCREEN_WIDTH * done, PROGRESS_BAR_HEIGHT}, 255, alpha);
}

void draw_failure(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 160, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    SDL_Delay(4000);
}
