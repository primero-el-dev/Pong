#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL_ttf.h>

#define EXIT_GAME_CODE -1
#define RETURN_TO_MENU_CODE 0
#define FISRT_PLAYER_INDEX 1
#define SECOND_PLAYER_INDEX 2

#define MAIN_MENU_SCREEN 1
#define GAME_PLAY_SCREEN 2
#define GAME_RESULT_SCREEN 3

struct Player
{
    SDL_Rect paddle;
    int velocity;
    bool key_up_pressed;
    bool key_down_pressed;
    int key_up;
    int key_down;
    int index;
    bool human;
};

struct Ball
{
    SDL_Rect rect;
    int velocity_x;
    int velocity_y;
};

// Here is stored all shared data. It's the alternative to global variables. See 'main' 
struct GameConfig
{
    int paddle_width;
    int paddle_height;
    int x_offset;
    int max_velocity;
    int ball_diameter;
    int max_points;
    int screen_width;
    int screen_height;
    int content_red;
    int content_green;
    int content_blue;
    int selected_content_red;
    int selected_content_green;
    int selected_content_blue;
    int menu_options_gap;
    int fps;
    char font[100];
};

struct Points
{
    int first_player;
    int second_player;
};

void move_player(struct Player* player, struct GameConfig* game_config);

struct Player init_player(struct GameConfig* game_config, int index, bool human);

struct Ball init_ball(struct GameConfig* game_config);

void handle_player_key_press(struct Player* player, SDL_Event ev);

void handle_human_player_acceleration(struct Player* player, struct GameConfig* game_config);

void handle_computer_player_acceleration(struct Player* player, struct Ball* ball, struct GameConfig* game_config);

void move_ball(struct Ball* ball, struct Player* first_player, struct Player* second_player, struct GameConfig* game_config);

/**
 * Returns:
 * -1 - exit game
 * 0 - return to menu
 * 1 - first player wins
 * 2 - wsecond player wins
 */
int init_game(struct GameConfig* game_config, SDL_Renderer* renderer, bool single_player);

bool show_menu_option(
    struct GameConfig* game_config, 
    SDL_Renderer* renderer, 
    TTF_Font* font, 
    const char* string,
    int index,
    int max_options,
    bool selected,
    SDL_Surface* surface, 
    SDL_Texture* texture
);

int main(int argc, char* args[])
{
    SDL_Window* window = NULL;
    SDL_Surface* window_surface = NULL;
    SDL_Renderer* renderer = NULL;

    struct GameConfig game_config;
    game_config.paddle_width = 20;
    game_config.paddle_width = 20;
    game_config.paddle_height = 80;
    game_config.x_offset = 10;
    game_config.max_velocity = 10;
    game_config.ball_diameter = 20;
    game_config.max_points = 1;
    game_config.screen_width = 960;
    game_config.screen_height = 720;
    game_config.content_red = 255;
    game_config.content_green = 255;
    game_config.content_blue = 255;
    game_config.selected_content_red = 0;
    game_config.selected_content_green = 255;
    game_config.selected_content_blue = 0;
    game_config.menu_options_gap = 40;
    game_config.fps = 60;
    strcpy(game_config.font, "fonts/Roboto-Regular.ttf");

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("Video initialization error: %s", SDL_GetError());

        return 0;
    }
    if (TTF_Init() < 0) {
        printf("Font initialization error: %s", TTF_GetError());

        return 0;
    }

    window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, game_config.screen_width, game_config.screen_height, SDL_WINDOW_SHOWN);
    window_surface = SDL_GetWindowSurface(window);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED & SDL_RENDERER_PRESENTVSYNC);

    TTF_Font* font = TTF_OpenFont(game_config.font, 24);
    SDL_Surface* surface;
    SDL_Texture* texture;

    bool running = true;
    int current_screen = MAIN_MENU_SCREEN;
    int result = 0;
    int selected_option = 1;
    SDL_Event ev;

    int first_player_scores;
    int second_player_scores;
    bool single_player;
    bool ok = true;

    while (running) {
        while (running && current_screen == MAIN_MENU_SCREEN) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            
            ok = ok && show_menu_option(&game_config, renderer, font, "PONG", 0, 4, false, surface, texture);
            ok = ok && show_menu_option(&game_config, renderer, font, "SINGLE PLAYER", 1, 4, selected_option == 0, surface, texture);
            ok = ok && show_menu_option(&game_config, renderer, font, "MULTI PLAYER", 2, 4, selected_option == 1, surface, texture);
            ok = ok && show_menu_option(&game_config, renderer, font, "EXIT", 3, 4, selected_option == 2, surface, texture);
            if (!ok) {
                running = false;
            }
            SDL_RenderPresent(renderer);
            
            while (SDL_PollEvent(&ev) != 0) {
                if (ev.type == SDL_QUIT) {
                    running = false;
                }
                else if (ev.type == SDL_KEYDOWN) {
                    switch (ev.key.keysym.sym) {
                    case SDLK_UP:
                        selected_option--;
                        break;
                    case SDLK_DOWN:
                        selected_option++;
                        break;
                    case SDLK_SPACE:
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                        switch (selected_option) {
                        case 0:
                            single_player = true;
                            current_screen = GAME_PLAY_SCREEN;
                            break;
                        case 1:
                            single_player = false;
                            current_screen = GAME_PLAY_SCREEN;
                            break;
                        case 2:
                            running = false;
                            break;
                        }
                    }
                    selected_option = selected_option % 3;
                    if (selected_option < 0) {
                        selected_option = 2;
                    }
                }
            }
        }

        first_player_scores = 0;
        second_player_scores = 0;

        while (running && current_screen == GAME_PLAY_SCREEN) {
            result = init_game(&game_config, renderer, single_player);
            switch (result) {
            case EXIT_GAME_CODE:
                running = false;
                break;
            case RETURN_TO_MENU_CODE:
                current_screen = MAIN_MENU_SCREEN;
                break;
            case FISRT_PLAYER_INDEX:
                first_player_scores++;
                break;
            case SECOND_PLAYER_INDEX:
                second_player_scores++;
                break;
            }
            if (first_player_scores >= game_config.max_points || second_player_scores >= game_config.max_points) {
                current_screen = GAME_RESULT_SCREEN;
            }
        }
        
        selected_option = 0;

        while (running && current_screen == GAME_RESULT_SCREEN) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            while (SDL_PollEvent(&ev) != 0) {
                if (ev.type == SDL_QUIT) {
                    running = false;
                }
                else if (ev.type == SDL_KEYDOWN) {
                    switch (ev.key.keysym.sym) {
                    case SDLK_UP:
                        selected_option--;
                        break;
                    case SDLK_DOWN:
                        selected_option++;
                        break;
                    case SDLK_ESCAPE:
                        current_screen = MAIN_MENU_SCREEN;
                        break;
                    case SDLK_SPACE:
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                        switch (selected_option) {
                        case 0:
                            current_screen = GAME_PLAY_SCREEN;
                            break;
                        case 1:
                            current_screen = MAIN_MENU_SCREEN;
                            break;
                        }
                    }
                    selected_option = selected_option % 2;
                    if (selected_option < 0) {
                        selected_option = 1;
                    }
                }
                
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
            }
            
            char winner_message[20];
            memset(winner_message, '\0', sizeof(winner_message));
            if (first_player_scores > second_player_scores) {
                strcpy(winner_message, "FIRST PLAYER WINS");
            }
            else {
                strcpy(winner_message, "SECOND PLAYER WINS");
            }
            
            ok = ok && show_menu_option(&game_config, renderer, font, winner_message, 0, 3, false, surface, texture);
            ok = ok && show_menu_option(&game_config, renderer, font, "PLAY AGAIN", 1, 3, selected_option == 0, surface, texture);
            ok = ok && show_menu_option(&game_config, renderer, font, "MAIN MENU", 2, 3, selected_option == 1, surface, texture);
            if (!ok) {
                running = false;
            }
            SDL_RenderPresent(renderer);
        }
    }

    TTF_CloseFont(font);
    TTF_Quit();

    SDL_DestroyWindow(window);
    SDL_FreeSurface(window_surface);
    free(window_surface);

    SDL_DestroyTexture(texture);
    free(texture);
    SDL_FreeSurface(surface);
    free(surface);
    SDL_DestroyRenderer(renderer);
    free(renderer);
    SDL_Quit();

    return 0;
}

void move_player(struct Player* player, struct GameConfig* game_config)
{
    player->paddle.y += player->velocity;
    if ((player->paddle.y + player->paddle.h) >= game_config->screen_height) {
        player->velocity = 0;
        player->paddle.y = game_config->screen_height - player->paddle.h;
    }
    else if (player->paddle.y <= 0) {
        player->velocity = 0;
        player->paddle.y = 0;
    }
}

struct Player init_player(struct GameConfig* game_config, int index, bool human)
{
    struct Player player;
    player.paddle.y = (game_config->screen_height - game_config->paddle_height) / 2;
    player.paddle.w = game_config->paddle_width;
    player.paddle.h = game_config->paddle_height;
    player.velocity = 0;
    player.key_up_pressed = false;
    player.key_down_pressed = false;
    player.index = index;
    player.human = human;

    if (index == FISRT_PLAYER_INDEX) {
        player.paddle.x = game_config->x_offset;
        player.key_up = SDLK_w;
        player.key_down = SDLK_s;
    }
    else if (index == SECOND_PLAYER_INDEX) {
        player.paddle.x = game_config->screen_width - game_config->x_offset - game_config->paddle_width;
        player.key_up = SDLK_UP;
        player.key_down = SDLK_DOWN;
    }
    
    return player;
}

struct Ball init_ball(struct GameConfig* game_config)
{
    struct Ball ball;
    ball.rect.w = game_config->ball_diameter;
    ball.rect.h = game_config->ball_diameter;
    ball.rect.x = (game_config->screen_width - game_config->ball_diameter) / 2;
    ball.rect.y = (game_config->screen_height - game_config->ball_diameter) / 2;
    ball.velocity_x = (((rand() % (game_config->max_velocity / 2)) + 1) + (game_config->max_velocity / 2)) * ((rand() % 2 == 1) ? 1 : -1);
    ball.velocity_y = (((rand() % (game_config->max_velocity / 2)) + 1) + (game_config->max_velocity / 2)) * ((rand() % 2 == 1) ? 1 : -1);

    return ball;
}

void handle_player_key_press(struct Player* player, SDL_Event ev)
{
    if (ev.type == SDL_KEYDOWN) {
        if (ev.key.keysym.sym == player->key_up) {
            player->key_up_pressed = true;
        }
        else if (ev.key.keysym.sym == player->key_down) {
            player->key_down_pressed = true;
        }
    }
    else if (ev.type == SDL_KEYUP) {
        if (ev.key.keysym.sym == player->key_up) {
            player->key_up_pressed = false;
        }
        else if (ev.key.keysym.sym == player->key_down) {
            player->key_down_pressed = false;
        }
    }
}

void handle_human_player_acceleration(struct Player* player, struct GameConfig* game_config)
{
    if (player->key_up_pressed) {
        if (player->velocity > -game_config->max_velocity) {
            player->velocity--;
        }
    }
    else if (player->key_down_pressed) {
        if (player->velocity < game_config->max_velocity) {
            player->velocity++;
        }
    }
}

void handle_computer_player_acceleration(struct Player* player, struct Ball* ball, struct GameConfig* game_config)
{
    if (ball->rect.y + ball->rect.h <= player->paddle.y && player->velocity > -game_config->max_velocity) {
        player->velocity--;
    }
    else if (ball->rect.y >= player->paddle.y + player->paddle.h && player->velocity < game_config->max_velocity) {
        player->velocity++;
    }
}

void move_ball(struct Ball* ball, struct Player* first_player, struct Player* second_player, struct GameConfig* game_config)
{
    ball->rect.x += ball->velocity_x;
    ball->rect.y += ball->velocity_y;

    if ((ball->rect.x <= first_player->paddle.x + first_player->paddle.w) 
        && (ball->rect.x >= first_player->paddle.x) 
        && (ball->rect.y <= first_player->paddle.y + first_player->paddle.h)
        && (ball->rect.y + ball->rect.h >= first_player->paddle.y))
    {
        ball->velocity_x = abs(ball->velocity_x);
    }
    else if ((ball->rect.x <= second_player->paddle.x + second_player->paddle.w) 
        && (ball->rect.x >= second_player->paddle.x) 
        && (ball->rect.y <= second_player->paddle.y + second_player->paddle.h)
        && (ball->rect.y + ball->rect.h >= second_player->paddle.y))
    {
        ball->velocity_x = -abs(ball->velocity_x);
    }

    if ((ball->rect.y + ball->rect.h + ball->velocity_y >= game_config->screen_height) || (ball->rect.y + ball->velocity_y <= 0)) {
        ball->velocity_y = -ball->velocity_y;
    }
}

/**
 * Returns:
 * -1 - exit game
 * 0 - return to menu
 * 1 - first player wins
 * 2 - wsecond player wins
 */
int init_game(struct GameConfig* game_config, SDL_Renderer* renderer, bool single_player)
{
    srand(time(NULL));

    struct Ball ball = init_ball(game_config);
    struct Player first_player = init_player(game_config, FISRT_PLAYER_INDEX, true);
    struct Player second_player = init_player(game_config, SECOND_PLAYER_INDEX, !single_player);

    SDL_Event ev;
    float frame_time = 0;
    int previous_time = 0;
    int current_time = 0;
    float delta_time = 0;
    float max_frame_time = 1.0 / game_config->fps;

    while (true) {
        previous_time = current_time;
        current_time = SDL_GetTicks();
        delta_time = (current_time - previous_time) / 1000.0f;
        frame_time += delta_time;

        while (SDL_PollEvent(&ev) != 0) {
            if (ev.type == SDL_QUIT) {
                return EXIT_GAME_CODE;
            }
            else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
                return 0;
            }
            handle_player_key_press(&first_player, ev);
            if (second_player.human) {
                handle_player_key_press(&second_player, ev);
            }
        }

        if (frame_time >= max_frame_time) {
            frame_time = 0;

            handle_human_player_acceleration(&first_player, game_config);
            if (second_player.human) {
                handle_human_player_acceleration(&second_player, game_config);
            }
            else {
                handle_computer_player_acceleration(&second_player, &ball, game_config);
            }

            move_player(&first_player, game_config);
            move_player(&second_player, game_config);

            move_ball(&ball, &first_player, &second_player, game_config);
        }

        if (ball.rect.x + ball.rect.w >= game_config->screen_width) {
            return 1;
        }
        if (ball.rect.x <= 0) {
            return 2;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &first_player.paddle);
        SDL_RenderFillRect(renderer, &second_player.paddle);
        SDL_RenderFillRect(renderer, &ball.rect);
        SDL_RenderPresent(renderer);
    }
}

bool show_menu_option(
    struct GameConfig* game_config, 
    SDL_Renderer* renderer, 
    TTF_Font* font, 
    const char* string,
    int index,
    int max_options,
    bool selected,
    SDL_Surface* surface, 
    SDL_Texture* texture
)
{
    SDL_Color color;
    if (selected) {
        color.r = game_config->selected_content_red;
        color.g = game_config->selected_content_green;
        color.b = game_config->selected_content_blue;
    }
    else {
        color.r = game_config->content_red;
        color.g = game_config->content_green;
        color.b = game_config->content_blue;
    }
    color.a = 255;

    surface = TTF_RenderText_Solid(font, string, color);
    if (surface == NULL) {
        printf("Error: %s\n", TTF_GetError());

        return false;
    }
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    int tex_w = 0;
    int tex_h = 0;
    SDL_QueryTexture(texture, NULL, NULL, &tex_w, &tex_h);
    SDL_Rect rect;
    rect.x = (game_config->screen_width  / 2) - tex_w;
    rect.y = (game_config->screen_height / 2) - tex_h - 
        ((((tex_h + game_config->menu_options_gap) * max_options) - game_config->menu_options_gap) / 2) + 
        ((tex_h + game_config->menu_options_gap) * index);
    rect.w = tex_w * 2;
    rect.h = tex_h * 2;

    SDL_RenderCopy(renderer, texture, NULL, &rect);

    return true;
}