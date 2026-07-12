/*******************************************************************************************
*
*   raylib gamejam template
*
*   Code licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2022-2026 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "stringlib.h"
#include <math.h>
#include <time.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>      // Emscripten library
#endif


#include <stdio.h>                          // Required for: printf()
#include <stdlib.h>                         // Required for: 
#include <string.h>                         // Required for:

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------

#define TICK_TIME 0.2f
#define COMPANY_COUNT_MAX 20
#define SPELL_SLOT_COUNT_MAX 10
#define SPELL_BUCKET_COUNT_MAX 50
#define SPELL_EFFECT_COUNT_MAX 20

#define HISTORY_SIZE 100
#define HISTORY_GRAPH_PX 200

#define SPELL_BUTTON_SIZE 60

#define FADE_TIME 0.5f


typedef enum {
    BUILDING_A_THREE,
    BUILDING_B_THREE,
    BUILDING_C_TWO,
    BUILDING_D_TWO,
    BUILDING_E_TWO_WIDE,
    BUILDING_F_FOUR,
    BUILDING_G_FOUR,
    BUILDING_H_THREE,

    BUILDING_MODEL_COUNT,
} BuildingModelKind;


#define SHOW_LETTER_BOUNDRY 0
#define LETTER_BOUNDRY_SIZE     0.25f
#define LETTER_BOUNDRY_COLOR    VIOLET

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif


// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
    #define DEBUG_LOG(log_level, ...) TraceLog(log_level, __VA_ARGS__)
#else
    #define DEBUG_LOG(...)
#endif

#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define CLAMP(a, min, max) ((a) < (min) ? (min) : ((a) > (max) ? (max) : (a)))

#define ARR_COUNT(arr) sizeof(arr) / sizeof(*arr)

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum { 
    SCREEN_LOGO = 0, 
    SCREEN_TITLE, 
    SCREEN_GAMEPLAY, 
    SCREEN_ENDING
} GameScreen;

typedef enum { 
    GAME_PLAYING = 0,
    GAME_END,
} GameState;


typedef enum {
    BUTTON_NORMAL,
    BUTTON_HOVERED,
    BUTTON_PRESSED,
    BUTTON_DISABLED,
} ButtonState;

typedef enum {
    SPELL_NONE = 0,
    SPELL_OVERLOAD_ELECTRICAL,
    SPELL_LUCK_BOOST,
    SPELL_NEGATIVE_THOUGHTS,
    SPELL_TAX_EVASION,
    SPELL_DESTROY,
    SPELL_COUNT,
} SpellKind;

typedef enum {
    SAVE_INITIAL = 0,
    SAVE_LAST_PLUS_ONE,
} SaveVersion;

typedef struct {
    Rectangle rect;
    ButtonState state;
    bool has_focus;
} ButtonInfo;

typedef struct {
    int max_ticks;
    int ticks;
    SpellKind kind;
    float bias;
    float volatility;
    bool is_backfire;
} SpellEffect;

typedef struct {
    SpellKind kind;
    ButtonInfo button;
} SpellSlot;

typedef struct {
    bool enabled;
    bool owned;
    int random_seed;

    int tier;
    
    int name_index;
    int model_index;

    float value;
    float volatility;
    float bias;

    float history[HISTORY_SIZE]; // Circular buffer
    int history_index;

    SpellEffect spells[SPELL_EFFECT_COUNT_MAX];
} Company;

typedef struct {
    float min;
    float max;
} CompanyTier;




//----------------------------------------------------------------------------------
// Global Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 720;
static const int screenHeight = 720;

static RenderTexture2D target = { 0 };  // Render texture to render our game

static const char* BuildingAssetPaths[BUILDING_MODEL_COUNT] = {
    "resources/models/buildings/building-a.glb",
    "resources/models/buildings/building-b.glb",
    "resources/models/buildings/building-c.glb",
    "resources/models/buildings/building-d.glb",
    "resources/models/buildings/building-e.glb",
    "resources/models/buildings/building-f.glb",
    "resources/models/buildings/building-g.glb",
    "resources/models/buildings/building-h.glb",
};


static const char* SpellAssetPaths[] = {
    "resources/runes/runeBlue_tile_000.png",
    "resources/runes/runeBlue_tile_001.png",
    "resources/runes/runeBlue_tile_002.png",
    "resources/runes/runeBlue_tile_003.png",
    "resources/runes/runeBlue_tile_004.png",
    "resources/runes/runeBlue_tile_005.png",
    "resources/runes/runeBlue_tile_006.png",
    "resources/runes/runeBlue_tile_007.png",
    "resources/runes/runeBlue_tile_008.png",
    "resources/runes/runeBlue_tile_009.png",
    "resources/runes/runeBlue_tile_010.png",
    "resources/runes/runeBlue_tile_011.png",
    "resources/runes/runeBlue_tile_012.png",
    "resources/runes/runeBlue_tile_013.png",
    "resources/runes/runeBlue_tile_014.png",
    "resources/runes/runeBlue_tile_015.png",
    "resources/runes/runeBlue_tile_016.png",
    "resources/runes/runeBlue_tile_017.png",
    "resources/runes/runeBlue_tile_018.png",
    "resources/runes/runeBlue_tile_019.png",
};

static const char* SpellBackfireAssetPaths[] = {
    "resources/runes/runeBlack_tile_000.png",
    "resources/runes/runeBlack_tile_001.png",
    "resources/runes/runeBlack_tile_002.png",
    "resources/runes/runeBlack_tile_003.png",
    "resources/runes/runeBlack_tile_004.png",
    "resources/runes/runeBlack_tile_005.png",
    "resources/runes/runeBlack_tile_006.png",
    "resources/runes/runeBlack_tile_007.png",
    "resources/runes/runeBlack_tile_008.png",
    "resources/runes/runeBlack_tile_009.png",
    "resources/runes/runeBlack_tile_010.png",
    "resources/runes/runeBlack_tile_011.png",
    "resources/runes/runeBlack_tile_012.png",
    "resources/runes/runeBlack_tile_013.png",
    "resources/runes/runeBlack_tile_014.png",
    "resources/runes/runeBlack_tile_015.png",
    "resources/runes/runeBlack_tile_016.png",
    "resources/runes/runeBlack_tile_017.png",
    "resources/runes/runeBlack_tile_018.png",
    "resources/runes/runeBlack_tile_019.png",
};

static const int SpellAssetCount = sizeof(SpellAssetPaths) / sizeof(*SpellAssetPaths);


static const char* CompanyNames[] = {
    "Orange LLC.",
    "Not Evil Inc.",
    "Baby Food 4 All",
    "Green Earth",
    "Burn Down Forests Co.",
    "Lizards and Sons",
};

static const int CompanyNameCount = sizeof(CompanyNames) / sizeof(*CompanyNames);


static const CompanyTier CompanyTiers[] = {
    (CompanyTier){0, 100},
    (CompanyTier){100, 250},
    (CompanyTier){250, 500},
    (CompanyTier){500, 1000},
    (CompanyTier){1000, 2000},
    (CompanyTier){2000, 5000},
};

static const int CompanyTierCount = sizeof(CompanyTiers) / sizeof(*CompanyTiers);

static const char* SpellNames[SPELL_COUNT] = {
    "ERROR: Empty Slot", // SPELL_NONE
    "Hex The Grid", // SPELL_OVERLOAD_ELECTRICAL
    "Luck Augmentation", // SPELL_LUCK_BOOST
    "Anti-Love Spell", // SPELL_NEGATIVE_THOUGHTS
    "Cook Their Books", // SPELL_TAX_EVASION
    "Meteor Strike", // SPELL_DESTROY
};

static const char* SpellDescriptions[SPELL_COUNT] = {
    "ERROR: Empty Slot.", // SPELL_NONE
    "Cause a magical power outage.", // SPELL_OVERLOAD_ELECTRICAL
    "People in the company will feel luckier. May backfire.", // SPELL_LUCK_BOOST
    "Make the people of the world dislike them, causing a social media outrage. May backfire.", // SPELL_NEGATIVE_THOUGHTS
    "Use your magic to commit tax fraud for them. May backfire.", // SPELL_TAX_EVASION
    "Destroy the company causing a new one to take it's place.", // SPELL_DESTROY
};

static const Color SkyColors[] = {
    BLUE,
    SKYBLUE,
    SKYBLUE,
    BLUE,
    DARKBLUE,
    CLITERAL(Color){ 0, 41, 86, 255 },
    CLITERAL(Color){ 0, 41, 86, 255 },
    DARKBLUE,
};

static const int SkyColorCount = sizeof(SkyColors) / sizeof(*SkyColors);

static struct {
    GameScreen screen;

    bool has_save;
    bool is_paused;
    bool unlimited;
    GameState state;
    Camera3D camera;
    int rand_seed;

    float tick_timer;
    unsigned int ticks;
    unsigned int frame;

    bool is_dragging;
    Vector2 drag_amount;
    Vector2 camera_offset;

    float fade;
    float screen_start_time;

    float money;
    float goal;

    int focused_company;
    Company companies[COMPANY_COUNT_MAX];
    ButtonInfo company_buttons[COMPANY_COUNT_MAX];
    int company_count;

    SpellKind spell_bucket[SPELL_BUCKET_COUNT_MAX];
    SpellSlot spells[SPELL_SLOT_COUNT_MAX];

    float last_backfire_time;

    ButtonInfo end_button_unlimited;
    ButtonInfo end_button_restart;


    Font font;
    Font bold;

    Texture2D button_green;
    Texture2D button_green_pressed;
    Texture2D button_red;
    Texture2D button_red_pressed;
    Texture2D button_disabled;

    Texture2D panel;

    Texture2D coin_texture;
    Texture2D star_texture;

    Texture2D spell_textures[ARR_COUNT(SpellAssetPaths)];
    Texture2D spell_backfire_textures[ARR_COUNT(SpellAssetPaths)];

    Model floor_model;
    Texture2D floor_texture;


    Model building_models[BUILDING_MODEL_COUNT];

    Model player_model;
    ModelAnimation* player_animations;
    int player_animation_count;
    float player_animation_timer;
} GAME;


//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);      // Update and Draw one frame

static void Init(void);

static void Update(void);
static void Tick(void);
static void Draw(void);

static void Save(void);
static void Load(void);

static void RestartGame();

static bool UpdateButton(ButtonInfo* button, bool disabled);
static void DrawButton(const char* text, float font_size, Rectangle rect, int state, Texture2D normal, Texture2D pressed);
static void DrawBuyButton(const char* text, float font_size, Rectangle rect, int state);
static void DrawSellButton(const char* text, float font_size, Rectangle rect, int state);

static void DrawTextCodepoint3D(Font font, int codepoint, Vector3 position, float fontSize, bool backface, Color tint);
static void DrawText3D(Font font, const char *text, Vector3 position, float fontSize, float fontSpacing, float lineSpacing, bool backface, Color tint);
static Vector3 MeasureText3D(Font font, const char* text, float fontSize, float fontSpacing, float lineSpacing);

static float Hash2D(float x, float y);
static float SmoothRandom(float x, float seed); // Generates numbers in range [-1,+1] interpolating smoothly between integer x values

static Company CompanyMakeRandom();
static void CompanyDoTick(Company* company, int game_ticks);
static void DrawCompanyHistory(Company* company, Vector2 position, Vector2 size);

static int FindPlayerTier();

static void SpellApply(SpellKind spell, Company* company);
static void SpellBucketFill();
static SpellKind SpellBucketPick();

static String GetLineFittingWidth(String text, Font font, int font_size, float max_width);
static Color ColorHueLerp(Color color1, Color color2, float factor);
static Mesh GenFloor(float width, float length, float u_scale, float v_scale);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_ERROR);         // Disable raylib trace log messages
#else
    SetTraceLogLevel(LOG_TRACE);         // Disable raylib trace log messages
#endif

    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "raylib gamejam template");

    #ifndef _DEBUG
    SetExitKey(KEY_NULL);
    #endif 

    Init();
    
    // Render texture to draw, enables screen scaling
    // NOTE: If screen is scaled, mouse input should be scaled proportionally
    target = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);     // Set our game frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button
    {
        UpdateDrawFrame();
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadRenderTexture(target);
    
    // TODO: Unload all loaded resources at this point

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//--------------------------------------------------------------------------------------------
// Module Functions Definition
//--------------------------------------------------------------------------------------------
// Update and draw frame
void UpdateDrawFrame(void)
{
    Update();
    Draw();
    GAME.frame++;
}

void Init()
{
    // Setup Game
    srand(time(NULL));
    GAME.rand_seed = rand();

    GAME.camera = (Camera3D){.position = (Vector3){0, 1.f, 2.f}, .target = (Vector3){0, 0.5f, 0}, .up = (Vector3){0, 1, 0}, .fovy = 80, CAMERA_PERSPECTIVE};

    GAME.floor_texture = LoadTexture("resources/textures/floor_stone_pattern.png");
    SetTextureFilter(GAME.floor_texture, TEXTURE_FILTER_BILINEAR);

    Mesh floor_mesh = GenFloor(50.f, 5.f, 100.f, 10.f);
    Model floor_model = LoadModelFromMesh(floor_mesh);
    floor_model.materials[0].maps->texture = GAME.floor_texture;

    GAME.floor_model = floor_model;

    for (int i = 0; i < BUILDING_MODEL_COUNT; i++)
    {
        GAME.building_models[i] = LoadModel(BuildingAssetPaths[i]);
    }

    GAME.font = LoadFontEx("resources/fonts/NotoSans-Regular.ttf", 48, NULL, 0);
    SetTextureFilter(GAME.font.texture, TEXTURE_FILTER_BILINEAR);
    GAME.bold = LoadFontEx("resources/fonts/NotoSans-Black.ttf", 48, NULL, 0);
    SetTextureFilter(GAME.bold.texture, TEXTURE_FILTER_BILINEAR);

    GAME.button_green = LoadTexture("resources/ui/button_green_normal.png");
    SetTextureFilter(GAME.button_green, TEXTURE_FILTER_BILINEAR);
    GAME.button_green_pressed = LoadTexture("resources/ui/button_green_pressed.png");
    SetTextureFilter(GAME.button_green_pressed, TEXTURE_FILTER_BILINEAR);
    GAME.button_red = LoadTexture("resources/ui/button_red_normal.png");
    SetTextureFilter(GAME.button_red, TEXTURE_FILTER_BILINEAR);
    GAME.button_red_pressed = LoadTexture("resources/ui/button_red_pressed.png");
    SetTextureFilter(GAME.button_red_pressed, TEXTURE_FILTER_BILINEAR);
    GAME.button_disabled = LoadTexture("resources/ui/button_disabled.png");
    SetTextureFilter(GAME.button_disabled, TEXTURE_FILTER_BILINEAR);

    GAME.panel = LoadTexture("resources/ui/panel.png");
    SetTextureFilter(GAME.panel, TEXTURE_FILTER_BILINEAR);

    GAME.coin_texture = LoadTexture("resources/ui/coin.png");
    SetTextureFilter(GAME.coin_texture, TEXTURE_FILTER_BILINEAR);

    GAME.star_texture = LoadTexture("resources/ui/star.png");
    SetTextureFilter(GAME.star_texture, TEXTURE_FILTER_BILINEAR);

    for (int i = 0; i < SpellAssetCount; i++)
    {
        GAME.spell_textures[i] = LoadTexture(SpellAssetPaths[i]);
        SetTextureFilter(GAME.spell_textures[i], TEXTURE_FILTER_BILINEAR);
        GAME.spell_backfire_textures[i] = LoadTexture(SpellBackfireAssetPaths[i]);
        SetTextureFilter(GAME.spell_backfire_textures[i], TEXTURE_FILTER_BILINEAR);
    }


    // TODO: Create rounds with multiple goals
    GAME.goal = 10000;


    // Generate game state
    Load();

    if (!GAME.has_save)
    {
        RestartGame();
        

        GAME.has_save = true;
        Save();
    }

}


void Update()
{
    // if (IsKeyPressed(KEY_F12))
    // {
    //     time_t now = time(NULL);         // Get current time
    //     struct tm *t = localtime(&now);  // Convert to local time structure

    //     TakeScreenshot(TextFormat("screenshot-%d%d%d-%d%d%d.png", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec));
    // }

    if (GAME.screen == SCREEN_LOGO)
    {
        if (GAME.frame == 0)
        {
            GAME.screen_start_time = GetTime();
        }

        float screen_time = GetTime() - GAME.screen_start_time;

        float fade_t = Clamp((screen_time - 1.f) / FADE_TIME, 0 , 1);
        GAME.fade = fade_t * fade_t;

        if (screen_time > 2.f)
        {
            GAME.frame = 0;
            GAME.fade = 1.f;
            GAME.screen = SCREEN_GAMEPLAY;
        }
    }
    if (GAME.screen == SCREEN_GAMEPLAY)
    {
        if (GAME.frame == 0)
        {
            GAME.screen_start_time = GetTime();
        }
        
        float screen_time = GetTime() - GAME.screen_start_time;
        float fade_t = Clamp(screen_time / FADE_TIME, 0 , 1);
        GAME.fade = 1.f - (fade_t * fade_t * fade_t);

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            GAME.is_dragging = true;
            GAME.drag_amount = Vector2Add(GAME.drag_amount, GetMouseDelta());
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            int new_focused_company = GAME.focused_company + roundf(-GAME.drag_amount.x / 100.f / 2.f);
            new_focused_company = CLAMP(new_focused_company, 0, GAME.company_count - 1);
            if (GAME.focused_company != new_focused_company)
            {
                // TODO: Animation timers
                GAME.focused_company = new_focused_company;
            }

            GAME.camera_offset.x = GAME.focused_company * 100.f * 2.f;
            GAME.camera_offset.y = 0;


            GAME.is_dragging = false;
            GAME.drag_amount = (Vector2){0};
        }

        GAME.camera.position.x = (GAME.camera_offset.x - GAME.drag_amount.x) / 100.f;
        GAME.camera.target.x = (GAME.camera_offset.x - GAME.drag_amount.x) / 100.f;


        if (!GAME.is_paused)
        {
            GAME.tick_timer += GetFrameTime();
            while(GAME.tick_timer > TICK_TIME)
            {
                Tick();
                GAME.tick_timer -= TICK_TIME;
            }
        }

        if (GAME.state == GAME_PLAYING)
        {
            for (int i = 0; i < COMPANY_COUNT_MAX; i++)
            {
                Vector2 building_reference_pos = GetWorldToScreen((Vector3){i * 2, 1.f, 0.5}, GAME.camera);
                Company* company = &GAME.companies[i];
                if (!company->enabled)
                {
                    continue;
                }

                const char* button_text = NULL;
                bool disabled = false;
                if (!company->owned)
                {
                    button_text = TextFormat("Acquire $%.0fK", company->value);
                    disabled = company->value > GAME.money;
                }
                else
                {
                    button_text = TextFormat("Sell $%.0fK", company->value);
                    disabled = company->value < 0;
                }
                
                Vector2 button_text_size = MeasureTextEx(GAME.bold, button_text, 32, 0);
                float text_width = MAX(button_text_size.x, 100);
                
                Rectangle button_rect = (Rectangle){building_reference_pos.x - (text_width + 40) / 2.f, screenHeight - 60 - 60 / 2.f, (text_width + 40), 60};
                GAME.company_buttons[i].rect = button_rect;
                
                if (UpdateButton(&GAME.company_buttons[i], disabled))
                {
                    Company* company = &GAME.companies[i];
                    if (!company->owned)
                    {
                        if (GAME.money >= company->value)
                        {
                            GAME.money -= company->value;
                            company->owned = true;
                        }
                    }
                    else
                    {
                        GAME.money += company->value;
                        *company = CompanyMakeRandom();
                    }
                }
            }


            int spell_count = 0;
            for (int i = 0; i < SPELL_SLOT_COUNT_MAX; i++)
            {
                SpellKind spell = GAME.spells[i].kind;
                if (spell != SPELL_NONE)
                {
                    spell_count++;
                }
            }

            if (spell_count == 0)
            {
                for (int i = 0; i < 3; i++)
                {
                    GAME.spells[i].kind = SpellBucketPick();
                }
                spell_count = 3;
            }

            float container_begin_y = screenHeight / 2.f - (spell_count * SPELL_BUTTON_SIZE + (spell_count - 1) * 10) / 2.f;
            float fence_length = SPELL_BUTTON_SIZE + 10;
            int spell_row = 0;
            for (int i = 0; i < SPELL_SLOT_COUNT_MAX; i++)
            {
                SpellKind spell = GAME.spells[i].kind;
                if (spell != SPELL_NONE)
                {

                    Vector2 button_size = {GAME.spell_textures[spell].width, GAME.spell_textures[spell].height};
                    Rectangle button_rect = {screenWidth - 10 - button_size.x, container_begin_y + fence_length * spell_row, button_size.x , button_size.y};

                    GAME.spells[i].button.rect = button_rect;

                    if (UpdateButton(&GAME.spells[i].button, false))
                    {
                        SpellApply(spell, &GAME.companies[GAME.focused_company]);
                        GAME.spells[i].kind = SPELL_NONE;
                    }

                    spell_row++;
                }
            }


            if (GAME.money > GAME.goal && !GAME.unlimited)
            {
                GAME.is_paused = true;
                GAME.state = GAME_END;
            }
        }
        else if (GAME.state == GAME_END)
        {
            Vector2 panel_size = { screenWidth / 2.f, screenHeight / 2.f };
            Vector2 panel_position = { (screenWidth - panel_size.x) / 2.f, (screenHeight - panel_size.y) / 2.f };
            float bottom_y = panel_position.y + panel_size.y - 10;
            float button_width = (panel_size.x - 20 * 3) / 2.f;
            GAME.end_button_unlimited.rect = (Rectangle){
                panel_position.x + 20,
                panel_position.y + panel_size.y - 20 - 60,
                button_width,
                60
            };

            GAME.end_button_restart.rect = (Rectangle){
                panel_position.x + 20 + button_width + 20,
                panel_position.y + panel_size.y - 20 - 60,
                button_width,
                60
            };

            if (UpdateButton(&GAME.end_button_unlimited, false))
            {
                GAME.state = GAME_PLAYING;
                GAME.is_paused = false;
                GAME.unlimited = true;
            }

            if (UpdateButton(&GAME.end_button_restart, false))
            {
                RestartGame();
                GAME.state = GAME_PLAYING;
                GAME.is_paused = false;
            }
        }
    }
}

void Tick()
{
    for (int i = 0; i < COMPANY_COUNT_MAX; i++)
    {
        Company* company = &GAME.companies[i];
        if (company->enabled)
        {
            CompanyDoTick(company, GAME.ticks);
        }
    }

    if (GAME.ticks % 25 == 0)
    {
        Save();
    }


    GAME.ticks++;
}


void Draw()
{
    BeginDrawing();

        if (GAME.screen == SCREEN_LOGO)
        {
            ClearBackground(BLACK);

            Vector2 text_size = {0};
            Vector2 text_position = {0};
            const char* text = NULL;

            text = "Mergers and Hexquisitions";
            text_size = MeasureTextEx(GAME.bold, text, 48, 0);
            text_position = (Vector2){(screenWidth - text_size.x) / 2.f, (screenHeight - text_size.y) / 2.f - 200};
            DrawTextEx(GAME.bold, text, text_position, 48, 0, WHITE);

            text = "Tecelli Akintug";
            text_size = MeasureTextEx(GAME.bold, text, 32, 0);
            text_position = (Vector2){(screenWidth - text_size.x) / 2.f, (screenHeight - text_size.y) / 2.f};
            DrawTextEx(GAME.bold, text, text_position, 32, 0, WHITE);

            DrawTextEx(GAME.font, "A game by", (Vector2){text_position.x - 60, text_position.y - 32}, 32, 0, WHITE);

            text = "This game autosaves...";
            text_size = MeasureTextEx(GAME.bold, text, 32, 0);
            text_position = (Vector2){(screenWidth - text_size.x) / 2.f, screenHeight - text_size.y / 2.f - 100};
            DrawTextEx(GAME.font, text, text_position, 32, 0, WHITE);

            text = "Assets by Kenney.nl";
            text_size = MeasureTextEx(GAME.bold, text, 32, 0);
            text_position = (Vector2){(screenWidth - text_size.x) / 2.f, screenHeight - text_size.y / 2.f - 50};
            DrawTextEx(GAME.font, text, text_position, 32, 0, WHITE);
        }

        else if (GAME.screen == SCREEN_GAMEPLAY)
        {
            // TODO: Fade in
            const float DAY_TIME_SECONDS = 120.f;
            float day_time = fmodf(GetTime() / DAY_TIME_SECONDS, 1.0f);
            Color prev_color = SkyColors[(int)floorf(day_time * SkyColorCount)];
            Color next_color = SkyColors[(int)ceilf(day_time  * SkyColorCount) % SkyColorCount];
            ClearBackground(ColorHueLerp(prev_color, next_color, fmodf(day_time * SkyColorCount, 1.0f)));
            
            float moon_angle = 2 * PI * day_time + PI / SkyColorCount;
            float sun_angle = moon_angle + PI;
            DrawCircleV((Vector2){screenWidth / 2.f + cosf(sun_angle) * screenHeight * 0.4f, screenHeight / 2.f + sinf(sun_angle) * screenHeight * 0.4f}, 40, YELLOW);
            DrawCircleV((Vector2){screenWidth / 2.f + cosf(moon_angle) * screenHeight * 0.45f, screenHeight / 2.f + sinf(moon_angle) * screenHeight * 0.45f}, 20, RAYWHITE);


            BeginMode3D(GAME.camera);

            // DrawGrid(10, 1.f);

            DrawModel(GAME.floor_model, (Vector3){0, -0.1f, 0}, 1.0f, WHITE);

            for (int i = 0; i < COMPANY_COUNT_MAX; i++)
            {
                Company* company = &GAME.companies[i];
                if (company->enabled)
                {
                    DrawModelEx(GAME.building_models[company->model_index], (Vector3){i * 2.f, 0, 0}, (Vector3){0, 1, 0}, 180, (Vector3){1, 1, 1}, WHITE);
                }
            }


            EndMode3D();
            

            for (int i = 0; i < COMPANY_COUNT_MAX; i++)
            {
                Vector2 building_reference_pos = GetWorldToScreen((Vector3){i * 2, 1.f, 0.5}, GAME.camera);
                Vector2 building_text_pos = (Vector2){building_reference_pos.x, building_reference_pos.y - 100};
                Company* company = &GAME.companies[i];
                if (!company->enabled)
                {
                    continue;
                }

                const char* value_text = NULL;
                Vector2 text_size = {0}; 
                Vector2 text_position = {0};
                
                text_size = MeasureTextEx(GAME.bold, CompanyNames[company->name_index], 32, 0);
                text_position = (Vector2){building_text_pos.x - text_size.x / 2.f, building_text_pos.y - text_size.y / 2};
                DrawRectangleRounded((Rectangle){text_position.x - 5, text_position.y, text_size.x + 10, text_size.y}, 0.5f, 16, ColorAlpha(RAYWHITE, 0.5f));
                DrawTextEx(GAME.bold, CompanyNames[company->name_index], text_position, 32, 0, BLACK);

                Vector2 graph_size = (Vector2){HISTORY_GRAPH_PX * 1.5f, HISTORY_GRAPH_PX};
                DrawCompanyHistory(company, (Vector2){building_reference_pos.x - graph_size.x / 2, (screenHeight - graph_size.y) / 2 + 120}, graph_size);
                

                int spell_count = 0;
                int spell_icon_gap = 20;
                float spells_width = 0;
                for (int spell_idx = 0; spell_idx < SPELL_EFFECT_COUNT_MAX; spell_idx++)
                {
                    SpellEffect effect = company->spells[spell_idx];
                    if (effect.ticks > 0)
                    {
                        spell_count++;
                        spells_width += GAME.spell_textures[effect.kind].width;
                    }
                }
                spells_width += (spell_count - 1) * spell_icon_gap;

                float spells_x = -spells_width / 2.f;
                for (int spell_idx = 0; spell_idx < SPELL_EFFECT_COUNT_MAX; spell_idx++)
                {
                    SpellEffect effect = company->spells[spell_idx];
                    if (effect.ticks > 0)
                    {
                        Texture2D spell_texture = effect.is_backfire ? GAME.spell_backfire_textures[effect.kind] : GAME.spell_textures[effect.kind];
                        Vector2 reference_pos = (Vector2){building_reference_pos.x + spells_x, screenHeight / 2.f - spell_texture.height};

                        DrawTexture(spell_texture, reference_pos.x, reference_pos.y, WHITE);
                        float t = (effect.ticks - GAME.tick_timer / TICK_TIME) / (float)effect.max_ticks;
                        DrawRectangleRounded((Rectangle){reference_pos.x, reference_pos.y + spell_texture.height + 2, t * spell_texture.width, 10}, 0.5f, 4, RED);
                        spells_x += GAME.spell_textures[effect.kind].width + spell_icon_gap;
                        
                        Rectangle spell_rect = (Rectangle){reference_pos.x, reference_pos.y, spell_texture.width, spell_texture.height + 2 + 10};
                        if (CheckCollisionPointRec(GetMousePosition(), spell_rect))
                        {
                            Vector2 name_size = MeasureTextEx(GAME.bold, SpellNames[effect.kind], 32, 0);
                            float rect_width = name_size.x + 20;
                            float rect_height = name_size.y + 10;
                            Rectangle text_box_rect = (Rectangle){
                                reference_pos.x + (spell_texture.width - rect_width) / 2.f,
                                reference_pos.y - rect_height - 2,
                                rect_width,
                                rect_height
                            };
                            NPatchInfo panel_patch = {
                                (Rectangle){0, 0, GAME.panel.width, GAME.panel.height},
                                6, 6, 6, 6,
                                NPATCH_NINE_PATCH
                            };
                            DrawTextureNPatch(GAME.panel, panel_patch, text_box_rect, (Vector2){0, 0}, 0, WHITE);
                            DrawTextEx(GAME.bold, SpellNames[effect.kind], (Vector2){text_box_rect.x + 10, text_box_rect.y + 5}, 32, 0, BLACK);
                        }
                    }
                }


                if (!company->owned)
                {
                    const char* button_text = TextFormat("Acquire $%.0fK", company->value);
                    DrawBuyButton(button_text, 32, GAME.company_buttons[i].rect, GAME.company_buttons[i].state);
                }
                else
                {
                    const char* button_text = TextFormat("Sell $%.0fK", company->value);
                    DrawSellButton(button_text, 32, GAME.company_buttons[i].rect, GAME.company_buttons[i].state);
                }
            }


            // Draw Spells

            int spell_count = 0;
            for (int i = 0; i < SPELL_SLOT_COUNT_MAX; i++)
            {
                SpellKind spell = GAME.spells[i].kind;
                if (spell != SPELL_NONE)
                {
                    spell_count++;
                }
            }

            float fence_length = SPELL_BUTTON_SIZE + 10;
            for (int i = 0; i < SPELL_SLOT_COUNT_MAX; i++)
            {
                SpellKind spell = GAME.spells[i].kind;
                if (spell == SPELL_NONE)
                {
                    continue;
                }
                
                ButtonInfo button = GAME.spells[i].button;
                Rectangle button_rect = button.rect;

                bool is_hovered = button.state == BUTTON_HOVERED || button.state == BUTTON_PRESSED;
                DrawTexture(GAME.spell_textures[spell], button_rect.x, button_rect.y, is_hovered ? ColorBrightness(WHITE, -0.1f) : WHITE);

                if (is_hovered)
                {
                    Vector2 name_size = MeasureTextEx(GAME.bold, SpellNames[spell], 32, 0);
                    Vector2 description_size = MeasureTextEx(GAME.font, SpellDescriptions[spell], 32, 0);

                    String lines[8];
                    int row_count = 0;

                    String description = Str(SpellDescriptions[spell]);
                    while (description.length > 0)
                    {
                        String line = GetLineFittingWidth(description, GAME.font, 32, screenWidth / 2.f - 30);
                        lines[row_count] = line;
                        description = StrMakeWindow(description, line.length + 1, description.length - line.length - 1);
                        row_count++;

                        if (row_count >= 8)
                        {
                            break;
                        }
                    }

                    float text_width = MAX(description_size.x, name_size.x);
                    float rect_width = MIN(screenWidth / 2.f, text_width + 30);
                    float rect_height = 30 + row_count * 30 + 20;

                    Rectangle tooltip_rect = (Rectangle){
                        Clamp(screenWidth - rect_width - 10 - button_rect.width - 10, 0, screenWidth - rect_width),
                        Clamp(button_rect.y - (rect_height - button_rect.height) / 2.f, 0, screenHeight - rect_height),
                        rect_width,
                        rect_height, 
                    };


                    NPatchInfo panel_patch = {
                        (Rectangle){0, 0, GAME.panel.width, GAME.panel.height},
                        6, 6, 6, 6,
                        NPATCH_NINE_PATCH
                    };
                    DrawTextureNPatch(GAME.panel, panel_patch, tooltip_rect, (Vector2){0, 0}, 0, WHITE);

                    DrawTextEx(GAME.bold, SpellNames[spell], (Vector2){tooltip_rect.x + 15, tooltip_rect.y + 6 }, 32, 0, BLACK);

                    for (int row = 0; row < row_count; row++)
                    {
                        String line = lines[row];
                        DrawTextEx(GAME.font, StrTempCstr(line), (Vector2){tooltip_rect.x + 15, tooltip_rect.y + 30 + 6 + 30 * row }, 32, 0, BLACK);
                    }
                }
            }

            float backfire_time = GetTime() - GAME.last_backfire_time;
            if (GAME.last_backfire_time > 0 && backfire_time < 1.5f)
            {
                const char* text = "BACKFIRE!";
                float t = backfire_time / 1.5f;
                float font_size = Lerp(100, 0, t * t * t * t);
                Vector2 size = MeasureTextEx(GAME.bold, text, font_size, 0);
                DrawTextEx(GAME.bold, text, (Vector2){(screenWidth -size.x) / 2, (screenHeight -size.y) / 2}, font_size, 0, RED);
            }


            // HUD:

            {
                Vector2 text_size = {0};
                Vector2 text_position = {0};
                Rectangle coin_rect = { 20, 20, 48, 48 };
                DrawTexturePro(GAME.coin_texture, (Rectangle){0, 0, GAME.coin_texture.width, GAME.coin_texture.height}, coin_rect, (Vector2){0, 0}, 0, WHITE);
                const char* money_str = TextFormat("%.0fK", GAME.money);
                text_size = MeasureTextEx(GAME.bold, money_str, 48, 0);
                text_position = (Vector2){ coin_rect.x + coin_rect.width + 10, 20};
                DrawRectangleRounded((Rectangle){text_position.x - 5, text_position.y, text_size.x + 10, text_size.y}, 0.5f, 16, ColorAlpha(RAYWHITE, 0.5f));
                DrawTextEx(GAME.bold, money_str, text_position, 48, 0, BLACK);


                Rectangle star_rect = { screenWidth - 20 - 48, 20, 48, 48 };
                DrawTexturePro(GAME.star_texture, (Rectangle){0, 0, GAME.star_texture.width, GAME.star_texture.height}, star_rect, (Vector2){0, 0}, 0, WHITE);
                const char* goal_str = TextFormat("%.0fK", GAME.goal);
                text_size = MeasureTextEx(GAME.bold, goal_str, 48, 0);
                text_position = (Vector2){ star_rect.x - text_size.x - 10, 20};
                DrawRectangleRounded((Rectangle){text_position.x - 5, text_position.y, text_size.x + 10, text_size.y}, 0.5f, 16, ColorAlpha(RAYWHITE, 0.5f));
                DrawTextEx(GAME.bold, goal_str, text_position, 48, 0, BLACK);
            }

            if (GAME.state == GAME_END)
            {
                Vector2 panel_size = { screenWidth / 2.f, screenHeight / 2.f };
                Vector2 panel_position = { (screenWidth - panel_size.x) / 2.f, (screenHeight - panel_size.y) / 2.f };
                NPatchInfo panel_patch = {
                    (Rectangle){0, 0, GAME.panel.width, GAME.panel.height},
                    6, 6, 6, 6,
                    NPATCH_NINE_PATCH
                };
                DrawTextureNPatch(GAME.panel, panel_patch, (Rectangle){ panel_position.x, panel_position.y, panel_size.x, panel_size.y }, (Vector2){0, 0}, 0, WHITE);

                Vector2 panel_it = (Vector2){ panel_position.x + 20, panel_position.y + 20 };
                Vector2 game_over_size = MeasureTextEx(GAME.bold, "YOU WIN!", 48, 0);
                DrawTextEx(GAME.bold, "YOU WIN!", (Vector2){panel_position.x + (panel_size.x - game_over_size.x) / 2.f, panel_it.y}, 48, 0, BLACK);
                panel_it.y += 48;

                String panel_body = Str("You used the power of magic and capitalism to please your boss!");
                while (panel_body.length > 0)
                {
                    String line = GetLineFittingWidth(panel_body, GAME.font, 32, panel_size.x - 40);
                    DrawTextEx(GAME.font, StrTempCstr(line), panel_it, 32, 0, BLACK);
                    
                    panel_body = StrMakeWindow(panel_body, line.length + 1, panel_body.length - line.length - 1);
                    panel_it.y += 30;
                }
                
                panel_body = Str("Thank you for playing. I hope you enjoyed!");
                while (panel_body.length > 0)
                {
                    String line = GetLineFittingWidth(panel_body, GAME.font, 32, panel_size.x - 40);
                    DrawTextEx(GAME.font, StrTempCstr(line), panel_it, 32, 0, BLACK);
                    
                    panel_body = StrMakeWindow(panel_body, line.length + 1, panel_body.length - line.length - 1);
                    panel_it.y += 30;
                }

                DrawBuyButton("Keep Going", 32, GAME.end_button_unlimited.rect, GAME.end_button_unlimited.state);
                DrawSellButton("Restart", 32, GAME.end_button_restart.rect, GAME.end_button_restart.state);
            }

            // DEBUG TEXT:

            #ifdef _DEBUG
            int iota = 5;

            DrawText(TextFormat("Money: %.2f", GAME.money), 20, 20 + iota * 20, 20, BLACK);
            iota++;
            DrawText(TextFormat("Ticks: %d", GAME.ticks), 20, 20 + iota * 20, 20, BLACK);
            iota++;

            if (GAME.focused_company >= 0)
            {
                Company* company = &GAME.companies[GAME.focused_company];
                // Debug Text
                DrawText(TextFormat("Value: %.2f", company->value), 20, 20 + iota * 20, 20, BLACK);
                iota++;
                DrawText(TextFormat("Tier: %d (%.2f-%.2f)", company->tier, CompanyTiers[company->tier].min, CompanyTiers[company->tier].max), 20, 20 + iota * 20, 20, BLACK);
                iota++;
                DrawText(TextFormat("Volatiliy: %.2f", company->volatility), 20, 20 + iota * 20, 20, BLACK);
                iota++;
                DrawText(TextFormat("Bias: %.4f", company->bias), 20, 20 + iota * 20, 20, BLACK);

            }
            #endif 
        }



        // FADE

        float fade_t = GAME.fade;
        fade_t = Clamp(fade_t, 0, 1);
        DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 255 * fade_t});

    EndDrawing();
}

#define SAVE_WRITE(a) fwrite(&(a), sizeof(a), 1, file);

#define SAVE_IO SAVE_WRITE
void Save(void)
{
    FILE* file = fopen("save.sav", "wb");
    if (!file)
    {
        return;
    }

    int version = SAVE_LAST_PLUS_ONE - 1;
    SAVE_IO(version);

    SAVE_IO(GAME.has_save);
    SAVE_IO(GAME.unlimited);
    SAVE_IO(GAME.ticks);
    SAVE_IO(GAME.money);
    SAVE_IO(GAME.goal);
    for (int i = 0; i < COMPANY_COUNT_MAX; i++)
    {
        Company* company = &GAME.companies[i];
        SAVE_IO(company->enabled);
        SAVE_IO(company->owned);
        SAVE_IO(company->random_seed);
        SAVE_IO(company->tier);
        SAVE_IO(company->name_index);
        SAVE_IO(company->model_index);
        SAVE_IO(company->value);
        SAVE_IO(company->volatility);
        SAVE_IO(company->bias);

        for (int j = 0; j < HISTORY_SIZE; j++)
        {
            SAVE_IO(company->history[j]);
        }
        SAVE_IO(company->history_index);

        for (int j = 0; j < SPELL_EFFECT_COUNT_MAX; j++)
        {
            SpellEffect* spell = &company->spells[j];
            SAVE_IO(spell->max_ticks);
            SAVE_IO(spell->ticks);
            SAVE_IO(spell->kind);
            SAVE_IO(spell->bias);
            SAVE_IO(spell->volatility);
            SAVE_IO(spell->is_backfire);
        }
    }
    SAVE_IO(GAME.company_count);

    for (int i = 0; i < SPELL_BUCKET_COUNT_MAX; i++)
    {
        SAVE_IO(GAME.spell_bucket[i]);
    }

    for (int i = 0; i < SPELL_SLOT_COUNT_MAX; i++)
    {
        SAVE_IO(GAME.spells[i].kind);
    }

    fclose(file);
}
#undef SAVE_IO

#define SAVE_READ(a) fread(&(a), sizeof(a), 1, file);

#define SAVE_IO SAVE_READ
void Load(void)
{
    FILE* file = fopen("save.sav", "rb");
    if (!file)
    {
        return;
    }

    int version = SAVE_LAST_PLUS_ONE - 1;
    SAVE_IO(version);

    SAVE_IO(GAME.has_save);
    SAVE_IO(GAME.unlimited);
    SAVE_IO(GAME.ticks);
    SAVE_IO(GAME.money);
    SAVE_IO(GAME.goal);
    for (int i = 0; i < COMPANY_COUNT_MAX; i++)
    {
        Company* company = &GAME.companies[i];
        SAVE_IO(company->enabled);
        SAVE_IO(company->owned);
        SAVE_IO(company->random_seed);
        SAVE_IO(company->tier);
        SAVE_IO(company->name_index);
        SAVE_IO(company->model_index);
        SAVE_IO(company->value);
        SAVE_IO(company->volatility);
        SAVE_IO(company->bias);

        for (int j = 0; j < HISTORY_SIZE; j++)
        {
            SAVE_IO(company->history[j]);
        }
        SAVE_IO(company->history_index);

        for (int j = 0; j < SPELL_EFFECT_COUNT_MAX; j++)
        {
            SpellEffect* spell = &company->spells[j];
            SAVE_IO(spell->max_ticks);
            SAVE_IO(spell->ticks);
            SAVE_IO(spell->kind);
            SAVE_IO(spell->bias);
            SAVE_IO(spell->volatility);
            SAVE_IO(spell->is_backfire);
        }
    }
    SAVE_IO(GAME.company_count);

    for (int i = 0; i < SPELL_BUCKET_COUNT_MAX; i++)
    {
        SAVE_IO(GAME.spell_bucket[i]);
    }

    for (int i = 0; i < SPELL_SLOT_COUNT_MAX; i++)
    {
        SAVE_IO(GAME.spells[i].kind);
    }

    fclose(file);
}
#undef SAVE_IO

void RestartGame()
{
    GAME.money = 50;

    GAME.company_count = 3;
    // GAME.companies[0] = (Company) {
    //     .enabled = true,
    //     .value = 200,
    //     .volatility = 100,
    //     .bias = 0.05f,
    //     .model_index = BUILDING_A_THREE,
    //     .name_index = 0,
    // };

    // GAME.companies[1] = (Company) {
    //     .enabled = true,
    //     .value = 150,
    //     .volatility = 40.f,
    //     .bias = 0.1f,
    //     .model_index = BUILDING_B_THREE,
    //     .name_index = 1,
    // };

    GAME.companies[0] = CompanyMakeRandom();
    GAME.companies[1] = CompanyMakeRandom();
    GAME.companies[2] = CompanyMakeRandom();

    // NOTE: This gurantees that the history data is populated with real data, if we need ticks for something else as well, we should MAX them
    for (int i = 0; i < HISTORY_SIZE; i++)
    {
        Tick();
    }

    
    SpellBucketFill();

        
    GAME.ticks = 0;
}


bool UpdateButton(ButtonInfo* button, bool disabled)
{
    bool pressed = false;
    int state = BUTTON_NORMAL;
    if (CheckCollisionPointRec(GetMousePosition(), button->rect))
    {
        state = BUTTON_HOVERED;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            button->has_focus = true;
        }

        if (!disabled && button->has_focus)
        {
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                pressed = true;
            }

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                state = BUTTON_PRESSED;
            }

        }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    {
        button->has_focus = false;
    }

    button->state = disabled ? BUTTON_DISABLED : state;
    return pressed;
}



void DrawButton(const char* text, float font_size, Rectangle rect, int state, Texture2D normal, Texture2D pressed)
{
    NPatchInfo patch_normal = {
        (Rectangle){0, 0, GAME.button_disabled.width, GAME.button_disabled.height},       // Texture source rectangle
        5,               // Left border offset
        5,                // Top border offset
        5,              // Right border offset
        10,             // Bottom border offset
        NPATCH_NINE_PATCH,             // Layout of the n-patch: 3x3, 1x3 or 3x1
    };

    NPatchInfo patch_pressed = {
        (Rectangle){0, 0, GAME.button_disabled.width, GAME.button_disabled.height},       // Texture source rectangle
        5,               // Left border offset
        5,                // Top border offset
        5,              // Right border offset
        5,             // Bottom border offset
        NPATCH_NINE_PATCH,             // Layout of the n-patch: 3x3, 1x3 or 3x1
    };

    switch (state)
    {
        case BUTTON_NORMAL: {
            NPatchInfo patch = patch_normal;
            DrawTextureNPatch(normal, patch, rect, (Vector2){0, 0}, 0, WHITE);
            Vector2 center = (Vector2){ rect.x + rect.width / 2 + patch.left - patch.right, rect.y + rect.height / 2 + patch.top - patch.bottom };
            Vector2 size = MeasureTextEx(GAME.bold, text, font_size, 0);
            DrawTextEx(GAME.bold, text, (Vector2){center.x - size.x / 2, center.y - size.y / 2}, font_size, 0, ColorAlpha(WHITE, 0.95f));
        } break;
        case BUTTON_DISABLED: {
            NPatchInfo patch = patch_normal;
            DrawTextureNPatch(GAME.button_disabled, patch, rect, (Vector2){0, 0}, 0, LIGHTGRAY);
            Vector2 center = (Vector2){ rect.x + rect.width / 2 + patch.left - patch.right, rect.y + rect.height / 2 + patch.top - patch.bottom };
            Vector2 size = MeasureTextEx(GAME.bold, text, font_size, 0);
            DrawTextEx(GAME.bold, text, (Vector2){center.x - size.x / 2, center.y - size.y / 2}, font_size, 0, ColorAlpha(WHITE, 0.95f));
        } break;
        case BUTTON_HOVERED: {
            NPatchInfo patch = patch_normal;
            DrawTextureNPatch(normal, patch, rect, (Vector2){0, 0}, 0, ColorBrightness(WHITE, -0.1f));
            Vector2 center = (Vector2){ rect.x + rect.width / 2 + patch.left - patch.right, rect.y + rect.height / 2 + patch.top - patch.bottom };
            Vector2 size = MeasureTextEx(GAME.bold, text, font_size, 0);
            DrawTextEx(GAME.bold, text, (Vector2){center.x - size.x / 2, center.y - size.y / 2}, font_size, 0, ColorAlpha(WHITE, 0.95f));
        } break;
        case BUTTON_PRESSED: {
            NPatchInfo patch = patch_pressed;
            rect.y += 4;
            rect.height -= 4;
            DrawTextureNPatch(pressed, patch, rect, (Vector2){0, 0}, 0, ColorBrightness(WHITE, -0.1f));
            Vector2 center = (Vector2){ rect.x + rect.width / 2 + patch.left - patch.right, rect.y + rect.height / 2 + patch.top - patch.bottom };
            Vector2 size = MeasureTextEx(GAME.bold, text, font_size, 0);
            DrawTextEx(GAME.bold, text, (Vector2){center.x - size.x / 2, center.y - size.y / 2}, font_size, 0, ColorAlpha(WHITE, 0.95f));
        } break;
    }
}

void DrawBuyButton(const char* text, float font_size, Rectangle rect, int state)
{
    DrawButton(text, font_size, rect, state, GAME.button_green, GAME.button_green_pressed);
}


void DrawSellButton(const char* text, float font_size, Rectangle rect, int state)
{
    DrawButton(text, font_size, rect, state, GAME.button_red, GAME.button_red_pressed);
}









// Draw codepoint at specified position in 3D space
static void DrawTextCodepoint3D(Font font, int codepoint, Vector3 position, float fontSize, bool backface, Color tint)
{
    // Character index position in sprite font
    // NOTE: In case a codepoint is not available in the font, index returned points to '?'
    int index = GetGlyphIndex(font, codepoint);
    float scale = fontSize/(float)font.baseSize;

    // Character destination rectangle on screen
    // NOTE: We consider charsPadding on drawing
    position.x += (float)(font.glyphs[index].offsetX - font.glyphPadding)*scale;
    position.z += (float)(font.glyphs[index].offsetY - font.glyphPadding)*scale;

    // Character source rectangle from font texture atlas
    // NOTE: We consider chars padding when drawing, it could be required for outline/glow shader effects
    Rectangle srcRec = { font.recs[index].x - (float)font.glyphPadding, font.recs[index].y - (float)font.glyphPadding,
                         font.recs[index].width + 2.0f*font.glyphPadding, font.recs[index].height + 2.0f*font.glyphPadding };

    float width = (float)(font.recs[index].width + 2.0f*font.glyphPadding)*scale;
    float height = (float)(font.recs[index].height + 2.0f*font.glyphPadding)*scale;

    if (font.texture.id > 0)
    {
        const float x = 0.0f;
        const float y = 0.0f;
        const float z = 0.0f;

        // normalized texture coordinates of the glyph inside the font texture (0.0f -> 1.0f)
        const float tx = srcRec.x/font.texture.width;
        const float ty = srcRec.y/font.texture.height;
        const float tw = (srcRec.x+srcRec.width)/font.texture.width;
        const float th = (srcRec.y+srcRec.height)/font.texture.height;

        #if SHOW_LETTER_BOUNDRY
        DrawCubeWiresV((Vector3){ position.x + width/2, position.y + height/2, position.z}, (Vector3){ width, height, LETTER_BOUNDRY_SIZE }, LETTER_BOUNDRY_COLOR);
        #endif 

        rlCheckRenderBatchLimit(4 + 4*backface);
        rlSetTexture(font.texture.id);

        rlPushMatrix();
            rlTranslatef(position.x, position.y, position.z);

            rlBegin(RL_QUADS);
                rlColor4ub(tint.r, tint.g, tint.b, tint.a);

                // Front Face
                rlNormal3f(0.0f, 1.0f, 0.0f);                                   // Normal Pointing Up
                rlTexCoord2f(tx, ty); rlVertex3f(x,         y + height, z);     // Top Left Of The Texture and Quad
                rlTexCoord2f(tx, th); rlVertex3f(x,         y, z);              // Bottom Left Of The Texture and Quad
                rlTexCoord2f(tw, th); rlVertex3f(x + width, y, z);              // Bottom Right Of The Texture and Quad
                rlTexCoord2f(tw, ty); rlVertex3f(x + width, y + height, z);     // Top Right Of The Texture and Quad

                if (backface)
                {
                    // Back Face
                    rlNormal3f(0.0f, -1.0f, 0.0f);                              // Normal Pointing Down
                    rlTexCoord2f(tw, ty); rlVertex3f(x + width, y + height, z); // Top Left Of The Texture and Quad
                    rlTexCoord2f(tw, th); rlVertex3f(x + width, y, z);          // Bottom Left Of The Texture and Quad
                    rlTexCoord2f(tx, th); rlVertex3f(x,         y, z);          // Bottom Right Of The Texture and Quad
                    rlTexCoord2f(tx, ty); rlVertex3f(x,         y + height, z); // Top Right Of The Texture and Quad
                }
            rlEnd();
        rlPopMatrix();

        rlSetTexture(0);
    }
}

// Draw a 2D text in 3D space
static void DrawText3D(Font font, const char *text, Vector3 position, float fontSize, float fontSpacing, float lineSpacing, bool backface, Color tint)
{
    int length = TextLength(text);          // Total length in bytes of the text, scanned by codepoints in loop

    float textOffsetY = 0.0f;               // Offset between lines (on line break '\n')
    float textOffsetX = 0.0f;               // Offset X to next character to draw

    float scale = fontSize/(float)font.baseSize;

    for (int i = 0; i < length;)
    {
        // Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetCodepoint(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol moving one byte
        if (codepoint == 0x3f) codepointByteCount = 1;

        if (codepoint == '\n')
        {
            // NOTE: Fixed line spacing of 1.5 line-height
            // TODO: Support custom line spacing defined by user
            textOffsetY += fontSize + lineSpacing;
            textOffsetX = 0.0f;
        }
        else
        {
            if ((codepoint != ' ') && (codepoint != '\t'))
            {
                DrawTextCodepoint3D(font, codepoint, (Vector3){ position.x + textOffsetX, position.y, position.z + textOffsetY }, fontSize, backface, tint);
            }

            if (font.glyphs[index].advanceX == 0) textOffsetX += (float)font.recs[index].width*scale + fontSpacing;
            else textOffsetX += (float)font.glyphs[index].advanceX*scale + fontSpacing;
        }

        i += codepointByteCount;   // Move text bytes counter to next codepoint
    }
}

static Vector3 MeasureText3D(Font font, const char* text, float fontSize, float fontSpacing, float lineSpacing)
{
    int len = TextLength(text);
    int tempLen = 0;                // Used to count longer text line num chars
    int lenCounter = 0;

    float tempTextWidth = 0.0f;     // Used to count longer text line width

    float scale = fontSize/(float)font.baseSize;
    float textHeight = scale;
    float textWidth = 0.0f;

    int letter = 0;                 // Current character
    int index = 0;                  // Index position in sprite font

    for (int i = 0; i < len; i++)
    {
        int next = 0;
        letter = GetCodepoint(&text[i], &next);
        index = GetGlyphIndex(font, letter);

        // NOTE: normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol so to not skip any we set next = 1
        if (letter == 0x3f) next = 1;
        i += next - 1;

        if (letter != '\n')
        {
            lenCounter++;
            if (font.glyphs[index].advanceX != 0) textWidth += font.glyphs[index].advanceX*scale;
            else textWidth += (font.recs[index].width + font.glyphs[index].offsetX)*scale;
        }
        else
        {
            if (tempTextWidth < textWidth) tempTextWidth = textWidth;
            lenCounter = 0;
            textWidth = 0.0f;
            textHeight += fontSize + lineSpacing;
        }

        if (tempLen < lenCounter) tempLen = lenCounter;
    }

    if (tempTextWidth < textWidth) tempTextWidth = textWidth;

    Vector3 vec = { 0 };
    vec.x = tempTextWidth + (float)((tempLen - 1)*fontSpacing); // Adds chars spacing to measure
    vec.y = 0.25f;
    vec.z = textHeight;

    return vec;
}

float Hash2D(float x, float y)
{
    return fmodf(sin(x * 12.9898 + y * 78.233) * 43758.5453, 1.0f);
}

float SmoothRandom(float x, float seed)
{
    int low_x = floorf(x);
    int high_x = ceilf(x);

    float low_hash = Hash2D(low_x, seed + GAME.rand_seed);
    float high_hash = Hash2D(high_x, seed + GAME.rand_seed);

    float t = x - low_x;
    return Lerp(low_hash, high_hash, t * t);
}

Company CompanyMakeRandom()
{
    // 25% chance for reduction
    // 50% chance for equal
    // 25% chance for increase
    int tier_offset = roundf(Lerp(-1, 1, (float)rand() / RAND_MAX)); 
    int tier_idx = FindPlayerTier() + tier_offset;
    tier_idx = CLAMP(tier_idx, 0, CompanyTierCount - 1);

    CompanyTier tier = CompanyTiers[tier_idx];

    Company company = {
        .enabled = true,
        .tier = tier_idx,
        .value = Lerp(tier.min, tier.max, (float)rand() / RAND_MAX),
        .volatility = ((((float)rand() / RAND_MAX) * 0.1f + 0.1f) * (tier.max - tier.min)), // NOTE: 10%-20%
        .bias = ((float)rand() / RAND_MAX) * 0.2f - 0.1f, // TODO: Pick a better range and smooth (biases near 0 are pretty boring)
        .model_index = rand() % BUILDING_MODEL_COUNT,
        .name_index = rand() % CompanyNameCount,
        .random_seed = rand(),
    };

    for (int i = 0; i < HISTORY_SIZE; i++)
    {
        CompanyDoTick(&company, GAME.ticks - HISTORY_SIZE + i);
    }

    return company;
}

void CompanyDoTick(Company* company, int game_ticks)
{
    CompanyTier tier = CompanyTiers[company->tier];
    float t = (company->value - tier.min) / (tier.max - tier.min);
    // t = Clamp(t, 0, 1);
    // float eased_t = (1 - cosf(PI * t)) * 0.5f;
    float eased_t = 1 / (1 + powf(M_E, -2 *(t-0.5f)));


    float volatility = company->volatility;
    float bias = company->bias;
    for (int i = 0; i < SPELL_EFFECT_COUNT_MAX; i++)
    {
        SpellEffect* effect = &company->spells[i];
        if (effect->ticks > 0)
        {
            effect->ticks--;
            volatility += effect->volatility;
            bias += effect->bias;
        }
    }

    
    float income_scale = volatility * (1 + bias * 0.5f);
    float random_income = SmoothRandom(game_ticks * TICK_TIME, company->random_seed) * 0.5f + 0.5f; // Always positive
    float income = (random_income * income_scale) * TICK_TIME;
    company->value += income * (1 - eased_t);
    
    float expense_scale = volatility * (1 - bias * 0.5f);
    float random_expenses = SmoothRandom(game_ticks * TICK_TIME, (company->random_seed + 7) * 13) * 0.5f + 0.5f; // Always positive
    float expenses = (random_expenses * expense_scale) * TICK_TIME;
    company->value -= expenses * (eased_t);

    company->history[company->history_index] = company->value;
    company->history_index = (company->history_index + 1) % HISTORY_SIZE;
}

void DrawCompanyHistory(Company* company, Vector2 position, Vector2 size)
{
    float max = 250;
    float min = 0;
    for (int i = 0; i < HISTORY_SIZE - 1; i++)
    {
        float value = company->history[i];
        max = MAX(max, value);
        if (min == 0)
        {
            min = value;
        }
        else
        {
            min = MIN(min, value);
        }
    }

    // Snap to nearest multiple of 500
    max = ceilf(max / 250.f) * 250.f;
    min = floorf(min / 250.f) * 250.f;

    DrawRectangleV(position, size, ColorAlpha(RAYWHITE, 0.5f));
    DrawRectangleLinesEx((Rectangle){position.x - 2, position.y - 2, size.x + 2 + 2, size.y + 2 + 2}, 2, ColorAlpha(BLACK, 0.9f));
    static Vector2 history_points[HISTORY_SIZE];
    for (int i = 0; i < HISTORY_SIZE; i++)
    {
        int index = (i + company->history_index) % HISTORY_SIZE;

        Vector2 current = (Vector2){(i / (float)(HISTORY_SIZE - 1)) * size.x, size.y + -1.f * ((company->history[index] - min) / (max - min)) * size.y };
        history_points[i] =  Vector2Add((Vector2){-(GAME.tick_timer / TICK_TIME - 0.5f) * (size.x / HISTORY_SIZE), 0}, Vector2Add(current, position));
    }
    DrawSplineLinear(history_points, HISTORY_SIZE, 5, RED);

    DrawTextEx(GAME.font, TextFormat("%.0fK", max), Vector2Add((Vector2){10, 0}, position), 20, 0, BLACK);
    DrawTextEx(GAME.font, TextFormat("%.0fK", min), Vector2Add((Vector2){10, size.y - 20}, position), 20, 0, BLACK);
    const char* value_str = TextFormat("%.0fK", company->value);
    Vector2 value_size = MeasureTextEx(GAME.bold, value_str, 32, 0);
    DrawTextEx(GAME.bold, value_str, (Vector2){-value_size.x + position.x + size.x - 10, -value_size.y / 2 + history_points[HISTORY_SIZE - 1].y}, 32, 0, BLACK);
}

int FindPlayerTier()
{
    if (GAME.money < 0)
    {
        return 0;
    }

    for (int i = 0; i < CompanyTierCount; i++)
    {
        CompanyTier tier = CompanyTiers[i];
        if (GAME.money >= tier.min && GAME.money < tier.max)
        {
            return i;
        }
    }

    return CompanyTierCount - 1;
}

void SpellApply(SpellKind spell, Company* company)
{
    SpellEffect effect = {0};
    effect.kind = spell;
    switch (spell) {
        case SPELL_OVERLOAD_ELECTRICAL:
        {
            effect.ticks = 20;
            effect.bias = -0.2;
            effect.volatility = +company->value * 0.1f;
            company->value *= 0.90f;
        } break;

        case SPELL_LUCK_BOOST:
        {
            effect.is_backfire = ((float)rand() / RAND_MAX) < 0.05f;

            if (effect.is_backfire)
            {
                effect.ticks = 5 * 30;
                effect.bias = -0.6;
                effect.volatility = +company->value * 0.1f;
            }
            else
            {
                effect.ticks = 5 * 30;
                effect.bias = +0.4;
                effect.volatility = +company->value * 0.1f;
            }
        } break;

        case SPELL_NEGATIVE_THOUGHTS:
        {
            effect.is_backfire = ((float)rand() / RAND_MAX) < 0.1f;
            if (effect.is_backfire)
            {
                effect.ticks = 5 * 20;
                effect.bias = +0.75;
                effect.volatility = company->value * 0.33f;
            }
            else
            {
                effect.ticks = 5 * 20;
                effect.bias = -0.5;
                effect.volatility = company->value * 0.25f;
            }
        } break;

        case SPELL_TAX_EVASION: 
        {
            effect.is_backfire = ((float)rand() / RAND_MAX) < 0.1f;
            if (effect.is_backfire)
            {
                effect.ticks = 5 * 20;
                effect.bias = +0.25;

                company->value *= 1.5f;
            }
            else
            {
                effect.ticks = 5 * 60;
                effect.bias = -0.25;
                effect.volatility = company->value * 0.25f;
                
                company->value *= 0.50f;
            }
            
        } break;

        case SPELL_DESTROY:
        {
            *company = CompanyMakeRandom();

        } break;

        case SPELL_NONE:
        case SPELL_COUNT:

            DEBUG_LOG(LOG_ERROR, "Something went horribly wrong, invalid spell kind!");
            break;
    }

    effect.max_ticks = effect.ticks;
    if (effect.is_backfire)
    {
        GAME.last_backfire_time = GetTime();
    }

    if (effect.ticks > 0)
    {
        for (int i = 0; i < SPELL_EFFECT_COUNT_MAX; i++)
        {
            if (company->spells[i].ticks <= 0)
            {
                company->spells[i] = effect;
                break;
            }
        }
    }
}

static void SpellBucketFill()
{
    int iota = 0;
    GAME.spell_bucket[iota++] = SPELL_DESTROY;
    GAME.spell_bucket[iota++] = SPELL_DESTROY;

    GAME.spell_bucket[iota++] = SPELL_OVERLOAD_ELECTRICAL;
    GAME.spell_bucket[iota++] = SPELL_OVERLOAD_ELECTRICAL;
    GAME.spell_bucket[iota++] = SPELL_OVERLOAD_ELECTRICAL;

    GAME.spell_bucket[iota++] = SPELL_LUCK_BOOST;
    GAME.spell_bucket[iota++] = SPELL_LUCK_BOOST;

    GAME.spell_bucket[iota++] = SPELL_NEGATIVE_THOUGHTS;
    GAME.spell_bucket[iota++] = SPELL_NEGATIVE_THOUGHTS;
    GAME.spell_bucket[iota++] = SPELL_NEGATIVE_THOUGHTS;

    GAME.spell_bucket[iota++] = SPELL_TAX_EVASION;
    GAME.spell_bucket[iota++] = SPELL_TAX_EVASION;
}

static SpellKind SpellBucketPick()
{
    int spell_count = 0;
    for (int i = 0; i < SPELL_BUCKET_COUNT_MAX; i++)
    {
        if (GAME.spell_bucket[i] != SPELL_NONE)
        {
            spell_count++;
        }
    }

    int rand_order = ((float)rand() / (RAND_MAX - 1.f)) * spell_count;
    int picked_idx = 0;
    for (int i = 0; i < SPELL_BUCKET_COUNT_MAX; i++)
    {
        if (GAME.spell_bucket[i] != SPELL_NONE)
        {
            rand_order--;
            if (rand_order == 0)
            {
                picked_idx = i;
                break;
            }
        }
    }

    SpellKind spell = GAME.spell_bucket[picked_idx];
    GAME.spell_bucket[picked_idx] = SPELL_NONE;

    if (spell_count <= 1)
    {
        SpellBucketFill();
    }

    return spell;
}



String GetLineFittingWidth(String text, Font font, int font_size, float max_width)
{
    float total_width = 0;
    size_t it = 0;
    bool end_reached = false;

    while (total_width < max_width && !end_reached)
    {
        size_t new_end = StrFindFrom(text, Str(" "), it);
        if (new_end == StrInvalidIndex)
        {
            new_end = text.length;
            end_reached = true;
        }
        String new_line = StrMakeWindow(text, 0, new_end);
        Vector2 size = MeasureTextEx(font, StrTempCstr(new_line), font_size, 0);
        if (size.x > max_width)
        {   
            break;
        }
        total_width = size.x;
        it = new_end + 1;
    }

    if (it == 0)
    {
        return StrMakeWindow(text, 0, 0);
    }
    else
    {
        return StrMakeWindow(text, 0, it - 1);
    }
}


Color ColorHueLerp(Color color1, Color color2, float factor)
{
    Vector3 hsv1 = ColorToHSV(color1);
    Vector3 hsv2 = ColorToHSV(color2);

    if (factor < 0.0f) factor = 0.0f;
    else if (factor > 1.0f) factor = 1.0f;

    float h = ((1.0f - factor)*hsv1.x + factor*hsv2.x);
    float s = ((1.0f - factor)*hsv1.y + factor*hsv2.y);
    float v = ((1.0f - factor)*hsv1.z + factor*hsv2.z);

    return ColorFromHSV(h, s, v);
}


Mesh GenFloor(float width, float length, float u_scale, float v_scale)
{
    Mesh mesh = {0};
    int resX = 2;
    int resZ = 2;

    // Vertices definition
    int vertexCount = resX*resZ; // vertices get reused for the faces

    Vector3 *vertices = (Vector3 *)RL_MALLOC(vertexCount*sizeof(Vector3));
    for (int z = 0; z < resZ; z++)
    {
        // [-length/2, length/2]
        float zPos = ((float)z/(resZ - 1) - 0.5f)*length;
        for (int x = 0; x < resX; x++)
        {
            // [-width/2, width/2]
            float xPos = ((float)x/(resX - 1) - 0.5f)*width;
            vertices[x + z*resX] = (Vector3){ xPos, 0.0f, zPos };
        }
    }

    // Normals definition
    Vector3 *normals = (Vector3 *)RL_MALLOC(vertexCount*sizeof(Vector3));
    for (int n = 0; n < vertexCount; n++) normals[n] = (Vector3){ 0.0f, 1.0f, 0.0f };   // Vector3.up;

    // TexCoords definition
    Vector2 *texcoords = (Vector2 *)RL_MALLOC(vertexCount*sizeof(Vector2));
    for (int v = 0; v < resZ; v++)
    {
        for (int u = 0; u < resX; u++)
        {
            texcoords[u + v*resX] = (Vector2){ u_scale*(float)u/(resX - 1), v_scale*(float)v/(resZ - 1) };
        }
    }

    // Triangles definition (indices)
    int numFaces = (resX - 1)*(resZ - 1);
    int *triangles = (int *)RL_MALLOC(numFaces*6*sizeof(int));
    int t = 0;
    for (int face = 0; face < numFaces; face++)
    {
        // Retrieve lower left corner from face ind
        int i = face + face/(resX - 1);

        triangles[t++] = i + resX;
        triangles[t++] = i + 1;
        triangles[t++] = i;

        triangles[t++] = i + resX;
        triangles[t++] = i + resX + 1;
        triangles[t++] = i + 1;
    }

    mesh.vertexCount = vertexCount;
    mesh.triangleCount = numFaces*2;
    mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount*3*sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount*2*sizeof(float));
    mesh.normals = (float *)RL_MALLOC(mesh.vertexCount*3*sizeof(float));
    mesh.indices = (unsigned short *)RL_MALLOC(mesh.triangleCount*3*sizeof(unsigned short));

    // Mesh vertices position array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.vertices[3*i] = vertices[i].x;
        mesh.vertices[3*i + 1] = vertices[i].y;
        mesh.vertices[3*i + 2] = vertices[i].z;
    }

    // Mesh texcoords array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.texcoords[2*i] = texcoords[i].x;
        mesh.texcoords[2*i + 1] = texcoords[i].y;
    }

    // Mesh normals array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.normals[3*i] = normals[i].x;
        mesh.normals[3*i + 1] = normals[i].y;
        mesh.normals[3*i + 2] = normals[i].z;
    }

    // Mesh indices array initialization
    for (int i = 0; i < mesh.triangleCount*3; i++) mesh.indices[i] = triangles[i];

    RL_FREE(vertices);
    RL_FREE(normals);
    RL_FREE(texcoords);
    RL_FREE(triangles);

    // Upload vertex data to GPU (static mesh)
    UploadMesh(&mesh, false);

    return mesh;
}
