#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

SDL_Window* Window(const char* title, int width, int height);
int globalR = 0;
int globalG = 0;
int globalB = 0;
int globalA = 255;

int ends_with(const char *str, const char *suffix) {
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix > lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

int is_regular_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int load_lua_file(lua_State *L, const char *filename) {
    if (luaL_dofile(L, filename)) {
        fprintf(stderr, "Error loading %s: %s\n", filename, lua_tostring(L, -1));
        lua_pop(L, 1);
        return 0;
    }
    return 1;
}

void create_sdl_window_metatable(lua_State *L) {
    luaL_newmetatable(L, "SDL_Window");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

void create_sdl_texture_metatable(lua_State *L) {
    luaL_newmetatable(L, "SDL_Texture");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

void create_sdl_font_metatable(lua_State *L) {
    luaL_newmetatable(L, "TTF_Font");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

static int lua_Window(lua_State *L) {
    const char* title = luaL_checkstring(L, 1);   
    int width = luaL_checkinteger(L, 2);          
    int height = luaL_checkinteger(L, 3);         

    SDL_Window* window = Window(title, width, height);  
    if (window) {
        SDL_Window** ud = (SDL_Window**)lua_newuserdata(L, sizeof(SDL_Window*));
        *ud = window;
        luaL_getmetatable(L, "SDL_Window");
        lua_setmetatable(L, -2);
        return 1;
    } else {
        lua_pushnil(L);
        return 1;
    }
}

static int lua_DrawRect(lua_State *L) {
    SDL_Window* window = *(SDL_Window**)luaL_checkudata(L, 1, "SDL_Window");
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int w = luaL_checkinteger(L, 4);
    int h = luaL_checkinteger(L, 5);
    int r = luaL_checkinteger(L, 6);
    int g = luaL_checkinteger(L, 7);
    int b = luaL_checkinteger(L, 8);
    int a = luaL_optinteger(L, 9, 255);

    SDL_Renderer* renderer = SDL_GetRenderer(window);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);

    return 0;
}

int lua_keyHeld(lua_State *L) {
    const char* key = luaL_checkstring(L, 1);

    const Uint8* state = SDL_GetKeyboardState(NULL);
    SDL_Scancode scancode = SDL_GetScancodeFromName(key);
    if (scancode != SDL_SCANCODE_UNKNOWN && state[scancode]) {
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

SDL_Window* Window(const char* title, int width, int height) {
    SDL_Window* window;
    SDL_Renderer* renderer;

    if (SDL_Init(SDL_INIT_VIDEO) != 0 || SDL_Init(SDL_INIT_AUDIO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return NULL;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        SDL_Log("SDL_mixer could not initialize! Mix_Error: %s\n", Mix_GetError());
        return NULL;
    }

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return NULL;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }

    return window;
}

SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* file) {
    SDL_Texture* texture = NULL;
    SDL_Surface* loadedSurface = IMG_Load(file);
    if (!loadedSurface) {
        fprintf(stderr, "Unable to load image %s! SDL_image Error: %s\n", file, IMG_GetError());
        return NULL;
    }

    texture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    SDL_FreeSurface(loadedSurface);
    if (!texture) {
        fprintf(stderr, "Unable to create texture from %s! SDL Error: %s\n", file, SDL_GetError());
        return NULL;
    }

    return texture;
}

static int lua_LoadTexture(lua_State* L) {
    const char* filename = luaL_checkstring(L, 1);
    SDL_Window* window = *(SDL_Window**)luaL_checkudata(L, 2, "SDL_Window");
    SDL_Renderer* renderer = SDL_GetRenderer(window);

    SDL_Texture* texture = loadTexture(renderer, filename);
    if (texture) {
        SDL_Texture** ud = (SDL_Texture**)lua_newuserdata(L, sizeof(SDL_Texture*));
        *ud = texture;

        luaL_getmetatable(L, "SDL_Texture");
        lua_setmetatable(L, -2);

        return 1;
    } else {
        lua_pushnil(L);
        return 1;
    }
}

static int lua_DrawTexture(lua_State* L) {
    SDL_Texture* texture = *(SDL_Texture**)luaL_checkudata(L, 1, "SDL_Texture");
    SDL_Window* window = *(SDL_Window**)luaL_checkudata(L, 2, "SDL_Window");
    SDL_Renderer* renderer = SDL_GetRenderer(window);
    int x = luaL_checkinteger(L, 3);
    int y = luaL_checkinteger(L, 4);
    int w, h;
    if (lua_gettop(L) > 4) {
        w = luaL_checkinteger(L, 5);
        h = luaL_checkinteger(L, 6);
    } else {
        SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    }
    SDL_Rect dstRect = {x, y, w, h};
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    return 0;
}

static int lua_RotateTexture(lua_State* L) {
    SDL_Texture* texture = *(SDL_Texture**)luaL_checkudata(L, 1, "SDL_Texture");
    SDL_Window* window = *(SDL_Window**)luaL_checkudata(L, 2, "SDL_Window");
    double angle = luaL_checknumber(L, 3);
    int width, height;
    if (lua_gettop(L) >= 5) {
        width = luaL_checkinteger(L, 4);
        height = luaL_checkinteger(L, 5);
    } else {
        width = 0;
        height = 0;
    }
    SDL_Renderer* renderer = SDL_GetRenderer(window);
    if (width == 0 || height == 0) {
        int texture_width, texture_height;
        SDL_QueryTexture(texture, NULL, NULL, &texture_width, &texture_height);
        SDL_Rect dstrect = { 0, 0, texture_width, texture_height };
        SDL_Point rotationPoint = { texture_width / 2, texture_height / 2 };
        SDL_RenderCopyEx(renderer, texture, NULL, &dstrect, angle, &rotationPoint, SDL_FLIP_NONE);
    } else {
        SDL_Rect dstrect = { 0, 0, width, height };
        SDL_Point rotationPoint = { width / 2, height / 2 };
        SDL_RenderCopyEx(renderer, texture, NULL, &dstrect, angle, &rotationPoint, SDL_FLIP_NONE);
    }
    return 0;
}

TTF_Font* LoadFont(const char* file, int size) {
    TTF_Font* font = TTF_OpenFont(file, size);
    if (!font) {
        printf("Error: Unable to load font! %s\n", TTF_GetError());
    }
    return font; 
}

static int lua_LoadFont(lua_State* L) {
    const char* file = luaL_checkstring(L, 1);
    int size = luaL_checkinteger(L, 2);
    TTF_Font* font = LoadFont(file, size);
    if (font) {
        TTF_Font** ud = (TTF_Font**)lua_newuserdata(L, sizeof(TTF_Font*));
        *ud = font;
        luaL_getmetatable(L, "TTF_Font");
        lua_setmetatable(L, -2);
        return 1;
    } else {
        lua_pushnil(L);
        return 1;
    }
}

static int lua_DrawFont(lua_State* L) {
    TTF_Font** fontPtr = (TTF_Font**)luaL_checkudata(L, 1, "TTF_Font");
    SDL_Window** windowPtr = (SDL_Window**)luaL_checkudata(L, 2, "SDL_Window");
    const char* text = luaL_checkstring(L, 3);
    int x = luaL_checkinteger(L, 4);
    int y = luaL_checkinteger(L, 5);
    int r = 255, g = 255, b = 255;
    if (!fontPtr || !*fontPtr) {
        return luaL_error(L, "Invalid TTF_Font");
    }
    if (!windowPtr || !*windowPtr) {
        return luaL_error(L, "Invalid SDL_Window");
    }
    TTF_Font* font = *fontPtr;
    SDL_Window* window = *windowPtr;

    if (lua_gettop(L) >= 6) {
        r = luaL_checkinteger(L, 6);
    }
    if (lua_gettop(L) >= 7) {
        g = luaL_checkinteger(L, 7);
    }
    if (lua_gettop(L) >= 8) {
        b = luaL_checkinteger(L, 8);
    }
    r = (r < 0) ? 0 : (r > 255) ? 255 : r;
    g = (g < 0) ? 0 : (g > 255) ? 255 : g;
    b = (b < 0) ? 0 : (b > 255) ? 255 : b;
    SDL_Color color = {(Uint8)r, (Uint8)g, (Uint8)b, 255};
    SDL_Renderer* renderer = SDL_GetRenderer(window);
    if (!renderer) {
        return luaL_error(L, "Failed to get renderer");
    }
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text, color);
    if (!textSurface) {
        return luaL_error(L, "Failed to create text surface");
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!textTexture) {
        return luaL_error(L, "Failed to create texture from surface");
    }
    int textWidth, textHeight;
    if (SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight) != 0) {
        SDL_DestroyTexture(textTexture);
        return luaL_error(L, "Failed to query texture");
    }
    SDL_Rect dstRect = {x, y, textWidth, textHeight};
    if (SDL_RenderCopy(renderer, textTexture, NULL, &dstRect) != 0) {
        SDL_DestroyTexture(textTexture);
        return luaL_error(L, "Failed to render texture");
    }
    SDL_DestroyTexture(textTexture);
    return 0;
}

static int lua_DrawCircle(lua_State* L) {
    SDL_Window* window = *(SDL_Window**)luaL_checkudata(L, 1, "SDL_Window");
    int radius = luaL_checkinteger(L, 2);
    int x = luaL_checkinteger(L, 3);
    int y = luaL_checkinteger(L, 4);
    int r = 255, g = 255, b = 255, a = 255;
    if (lua_gettop(L) > 4) {
        r = luaL_checkinteger(L, 5);
        g = luaL_checkinteger(L, 6);
        b = luaL_checkinteger(L, 7);
        a = luaL_checkinteger(L, 8);
    }
    SDL_Renderer* renderer = SDL_GetRenderer(window);
    if (!renderer) {
        return luaL_error(L, "Failed to get renderer");
    }

    SDL_SetRenderDrawColor(renderer, r, g, b, a);

    for (int dy = -radius; dy <= radius; dy++) {
        int dx = (int)sqrt(radius * radius - dy * dy);

        SDL_RenderDrawLine(renderer, x - dx, y + dy, x + dx, y + dy);
    }
    return 0;
}

static int lua_PlaySound(lua_State* L) {
    Mix_Music* bgm = Mix_LoadMUS(luaL_checkstring(L, 1));
    int playIndefinetely = lua_toboolean(L, 2);
    if (playIndefinetely == 1) {
        playIndefinetely = -1;
    }
    if (bgm == NULL) {
        printf("Failed to load beat music! Mix_Error: %s\n", Mix_GetError());
        return 1;
    }
    if (Mix_PlayMusic(bgm, playIndefinetely) == -1) {
        printf("SDL_mixer could not play music! Mix_Error: %s\n", Mix_GetError());
        Mix_FreeMusic(bgm); 
        return 1;
    }
    return 0;
}

static int lua_changeBackgroundColor(lua_State* L) {
    globalR = luaL_checkinteger(L, 1);
    globalG = luaL_checkinteger(L, 2);
    globalB = luaL_checkinteger(L, 3);
    globalA = luaL_checkinteger(L, 4);
    return 0;
}

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        fprintf(stderr, "IMG_Init Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    lua_State *L = luaL_newstate();
    create_sdl_window_metatable(L);
    create_sdl_texture_metatable(L);
    create_sdl_font_metatable(L);
    luaL_openlibs(L);
    lua_register(L, "Window", lua_Window);
    lua_register(L, "DrawRect", lua_DrawRect);
    lua_register(L, "DrawCircle", lua_DrawCircle);
    lua_register(L, "KeyHeld", lua_keyHeld);
    lua_register(L, "LoadTexture", lua_LoadTexture);
    lua_register(L, "DrawTexture", lua_DrawTexture);    
    lua_register(L, "RotateTexture", lua_RotateTexture);
    lua_register(L, "LoadFont", lua_LoadFont);
    lua_register(L, "DrawFont", lua_DrawFont);
    lua_register(L, "PlaySound", lua_PlaySound);
    lua_register(L, "ChangeBackgroundColor", lua_changeBackgroundColor);

    const char *directory = "src";
    DIR *dir;
    struct dirent *ent;
    int main_lua_loaded = 0;

    if ((dir = opendir(directory)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            char filepath[1024];
            snprintf(filepath, sizeof(filepath), "%s/%s", directory, ent->d_name);
            if (ends_with(ent->d_name, ".lua") && is_regular_file(filepath)) {
                printf("Loading Lua file: %s\n", filepath);
                if (!load_lua_file(L, filepath)) {
                    closedir(dir);
                    lua_close(L);
                    TTF_Quit();
                    IMG_Quit();
                    Mix_Quit();
                    SDL_Quit();
                    return 1;
                }
                if (strcmp(ent->d_name, "main.lua") == 0) {
                    main_lua_loaded = 1;
                }
            }
        }
        closedir(dir);
    } else {
        perror("opendir");
        lua_close(L);
        TTF_Quit();
        IMG_Quit();
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    if (!main_lua_loaded) {
        fprintf(stderr, "main.lua not loaded\n");
        lua_close(L);
        TTF_Quit();
        IMG_Quit();
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = NULL;
    lua_getglobal(L, "window");
    if (!lua_isuserdata(L, -1)) {
        fprintf(stderr, "Error: global 'window' is not a userdata\n");
        lua_close(L);
        TTF_Quit();
        IMG_Quit();
        Mix_Quit();
        SDL_Quit();
        return 1;
    } else {
        window = *(SDL_Window**)luaL_checkudata(L, -1, "SDL_Window");
    }
    lua_pop(L, 1);

    if (!window) {
        fprintf(stderr, "Error: Failed to get valid window from Lua\n");
        lua_close(L);
        TTF_Quit();
        IMG_Quit();
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_GetRenderer(window);
    if (!renderer) {
        fprintf(stderr, "Error: Failed to get renderer\n");
        lua_close(L);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    SDL_Event event;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        SDL_SetRenderDrawColor(renderer, globalR, globalB, globalB, globalA);
        SDL_RenderClear(renderer);
        lua_getglobal(L, "update");

        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0) != 0) {
                fprintf(stderr, "Error calling update: %s\n", lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        } else {
            fprintf(stderr, "Error: update function not found in main.lua\n");
            running = 0;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    lua_close(L);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    Mix_Quit();
    SDL_Quit();
    return 0;
}