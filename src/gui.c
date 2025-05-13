#include "gui.h"


SDL_Renderer *renderer;
static SDL_Window *window;

static const Uint8 *keyboard_state = NULL; // Current keyboard state
static Uint8 previous_keyboard_state[SDL_NUM_SCANCODES]; // Previous keyboard state


int mouse_location[2] = {0, 0};
int mouse_clicked = 0;
int mouse_down = 0;
int should_continue = 1;

void initialize_keyboard_state() {
    keyboard_state = SDL_GetKeyboardState(NULL);
    memcpy(previous_keyboard_state, keyboard_state, sizeof(previous_keyboard_state));
}

void update_keyboard_state() {
    memcpy(previous_keyboard_state, keyboard_state, sizeof(previous_keyboard_state));
    keyboard_state = SDL_GetKeyboardState(NULL);
}

int is_key_pressed(SDL_Scancode key) {
    return keyboard_state && keyboard_state[key];
}

int is_key_down(SDL_Scancode key) {
    return keyboard_state && keyboard_state[key] && !previous_keyboard_state[key];
}

int is_key_up(SDL_Scancode key) {
    return keyboard_state && !keyboard_state[key] && previous_keyboard_state[key];
}

void handle_events() {
    SDL_Event event;

    // Update keyboard state before processing events
    update_keyboard_state();

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                should_continue = 0;
                break;
            case SDL_MOUSEBUTTONDOWN:
                mouse_location[0] = event.button.x;
                mouse_location[1] = event.button.y;
                if (mouse_clicked == 0) {
                    mouse_clicked = 1;
                } else if (mouse_clicked == 1) {
                    mouse_clicked = 2;
                }

                mouse_down = 1;

                break;
            case SDL_MOUSEBUTTONUP:
                mouse_down = 0;
                mouse_clicked = 0;
                break;
            default:
                break;
        }
    }
}

void clear_window() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}



void initialize_window(const char *title, int window_width, int window_height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Could not initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow(title, 0, 0, window_width, window_height, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Couldn't set screen mode to required dimensions: %s\n", SDL_GetError());
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
}

void cleanup() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}


void draw_rectangle(int x, int y, int width, int height) {
    SDL_Rect rectangle = { x, y, width, height};
    SDL_RenderFillRect(renderer, &rectangle);
}

void present_window(){
    SDL_RenderPresent(renderer);
}

void set_color(int r, int g, int b, int a){
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); // Enable alpha blending
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

SDL_Texture *load_sprite(const char *path) {
    SDL_Texture *texture = IMG_LoadTexture(renderer, path);
    if (texture == NULL) {
        printf("Could not load sprite: %s\n", SDL_GetError());
        exit(1);
    }
    return texture;
}

void draw_sprite(SDL_Texture *texture, int x, int y) {
    SDL_Rect dest = {x, y, 0, 0};
    SDL_QueryTexture(texture, NULL, NULL, &dest.w, &dest.h);
    SDL_RenderCopy(renderer, texture, NULL, &dest);
}

void draw_sprite_scaled(SDL_Texture *texture, int x, int y, int width, int height) {
    SDL_Rect dest = {x, y, width, height};
    SDL_RenderCopy(renderer, texture, NULL, &dest);
}

void cleanup_sprite(SDL_Texture *texture) {
    SDL_DestroyTexture(texture);
}

