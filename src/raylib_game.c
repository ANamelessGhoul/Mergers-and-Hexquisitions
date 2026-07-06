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
#include <math.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>      // Emscripten library
#endif


#include <stdio.h>                          // Required for: printf()
#include <stdlib.h>                         // Required for: 
#include <string.h>                         // Required for:

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------

#define TICK_TIME 0.1f

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

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum { 
    SCREEN_LOGO = 0, 
    SCREEN_TITLE, 
    SCREEN_GAMEPLAY, 
    SCREEN_ENDING
} GameScreen;

//----------------------------------------------------------------------------------
// Global Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 720;
static const int screenHeight = 720;

static RenderTexture2D target = { 0 };  // Render texture to render our game

static struct {
    Camera3D camera;

    float tick_timer;
    unsigned int ticks;

    Model floor_model;

    Model player_model;
    ModelAnimation* player_animations;
    int player_animation_count;
    float player_animation_timer;
} GAME;


//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);      // Update and Draw one frame
static void Update(void);
static void Draw(void);
static void Tick(void);

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
    
    GAME.camera = (Camera3D){.position = (Vector3){0, 1.7f, 10}, .up = (Vector3){0, 1, 0}, .fovy = 80, CAMERA_PERSPECTIVE};

    Mesh floor_mesh = GenMeshCube(10, 0.1, 10);
    Model floor_model = LoadModelFromMesh(floor_mesh);

    GAME.floor_model = floor_model;

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
}

void Update()
{
    // UpdateCamera(&GAME.camera, CAMERA_FIRST_PERSON);

    GAME.tick_timer += GetFrameTime();
    while(GAME.tick_timer > TICK_TIME)
    {
        Tick();
        GAME.ticks++;
        GAME.tick_timer -= TICK_TIME;
    }

}

void Tick()
{

}

void Draw()
{
    BeginTextureMode(target);
        ClearBackground(BLACK);
        
        BeginMode3D(GAME.camera);

        DrawGrid(10, 1.f);

        DrawModel(GAME.floor_model, (Vector3){0, -0.1f, 0}, 1.0f, WHITE);

        EndMode3D();
    EndTextureMode();
    



    // Render to screen (main framebuffer)
    BeginDrawing();
        ClearBackground(RAYWHITE);
        
        DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height }, 
            (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);


    EndDrawing();
}