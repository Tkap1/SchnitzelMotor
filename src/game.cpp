#include "sound.h"
#include "game.h"
#include "assets.h"
#include "input.h"
#include "render_interface.h"

// #############################################################################
//                           Game Globals
// #############################################################################
static GameState* gameState;

// #############################################################################
//                           Game Functions
// #############################################################################
bool is_down(GameInputType type);
bool just_pressed(GameInputType type);
void update_game_input();
void move_x();
IRect get_player_rect();

// #############################################################################
//                           Update Game (Exported from DLL)
// #############################################################################
EXPORT_FN void update_game(GameState* gameStateIn, Input* inputIn, RenderData* renderDataIn, BumpAllocator* allocator, PlaySoundFunc play_sound)
{
  if(gameState != gameStateIn)
  {
    input = inputIn;
    renderData = renderDataIn;
    gameState = gameStateIn;

  }

  if(!gameState->initialized)
  {
    gameState->initialized = true;
    gameState->jumpSound = load_wav("assets/sounds/jump.wav", allocator);
    gameState->wallJumpSound = load_wav("assets/sounds/wall.wav", allocator);
    gameState->playerPos = {50, -100};
  }



  update_game_input();

  // This is inside of the data section of the memory, int the exe
  float dt = 1.0f / 60.0f;
  float maxRunSpeed = 10.0f;
  float wallJumpSpeed = 15.0f;
  float runAcceleration = 50.0f;
  float fallSideAcceleration = 35.0f;
  float maxJumpSpeed = -14.0f;
  float fallSpeed = 18.0f;
  float gravity = 70.0f;
  float runReduce = 80.0f;
  float wallClimbSpeed = -4.0f;
  float wallSlideDownSpeed = 8.0f;
  static Vec2 speed;
  static float highestHeight = input->screenSize.y;
  static float varJumpTimer = 0.0f;
  static float xRemainder = 0.0f;
  static float yRemainder = 0.0f;
  static float wallJumpTimer = 0.0f;
  static bool playerGrounded = true;
  static bool grabbingWall = false;

  static IRect solids [] =
  {
    // {48 * 2, 48, 48 * 8, 48},
    // {624, 96, 480, 48},
    {48 * 13, 48 * 2, 48, 48 * 8},
    {48 *  8, 48 * 2, 48, 48 * 8},
    {0, 624, 1048, 48},
    // {1048 + 240, 624, 1048, 48},
    // {1040 - 240, 96 * 4, 480, 48},
    // {48 * 3, 48 * 10, 48 * 3, 48},
    // {48 * 5, 48 * 7, 48 * 2, 48},
    // {48 * 9, 48 * 5, 48 * 3, 48},
  };

  for(int solidIdx = 0; solidIdx < ArraySize(solids); solidIdx++)
  {
    draw_quad(solids[solidIdx].pos, solids[solidIdx].size);
  }

  if(key_pressed_this_frame(KEY_R))
  {
    gameState->playerPos = {0, 200};
  }

  float directionChangeMult = 1.6f;
  if(is_down(INPUT_MOVE_LEFT))
  {
    float mult = 1.0f;
    if(speed.x > 0.0f)
    {
      mult = directionChangeMult;
    }

    if(playerGrounded)
    {
      speed.x = approach(speed.x, -maxRunSpeed, runAcceleration * mult * dt);
    }
    else
    {
      speed.x = approach(speed.x, -maxRunSpeed, fallSideAcceleration * mult * dt);
    }

  }

  if(is_down(INPUT_MOVE_RIGHT))
  {
    float mult = 1.0f;
    if(speed.x < 0.0f)
    {
      mult = directionChangeMult;
    }

    if(playerGrounded)
    {
      speed.x = approach(speed.x, maxRunSpeed, runAcceleration * mult * dt);
    }
    else
    {
      speed.x = approach(speed.x, maxRunSpeed, fallSideAcceleration * mult * dt);
    }
  }

  // firction
  if(!is_down(INPUT_MOVE_LEFT) &&
     !is_down(INPUT_MOVE_RIGHT))
  {
    speed.x = approach(speed.x, 0, runReduce * dt);
  }

  // Jumping
  {
    if(just_pressed(INPUT_JUMP) && playerGrounded)
    {
      play_sound(gameState->jumpSound);
      varJumpTimer = 0.0f;
      speed.y = maxJumpSpeed;
      playerGrounded = false;
      gameState->gameInput[INPUT_JUMP].justPressed = false;
    }

    if(is_down(INPUT_JUMP) &&
      varJumpTimer < 0.1f)
    {
      speed.y = min(speed.y, maxJumpSpeed);
    }

    if(just_pressed(INPUT_JUMP))
    {
      for(int solidIdx = 0; solidIdx < ArraySize(solids); solidIdx++)
      {
        IRect playerRect = get_player_rect();
        playerRect.pos.x -= 2 * 6;
        playerRect.size.x += 4 * 6;
        IRect solidRect = solids[solidIdx];

        if(rect_collision(solidRect, playerRect))
        {
          int playerRectLeft = playerRect.pos.x;
          int playerRectRight = playerRect.pos.x + playerRect.size.x;
          int solidRectLeft = solidRect.pos.x;
          int solidRectRight = solidRect.pos.x + solidRect.size.x;

          // Colliding on the Right
          if(solidRectRight - playerRectLeft <
            playerRectRight - solidRectLeft)
          {
            wallJumpTimer = 0.1f;
            varJumpTimer = 0.0f;
            speed.x = wallJumpSpeed;
            speed.y = maxJumpSpeed;
            play_sound(gameState->wallJumpSound);
          }

          // Colliding on the Left
          if(solidRectRight - playerRectLeft >
            playerRectRight - solidRectLeft)
          {
            wallJumpTimer = 0.1f;
            varJumpTimer = 0.0f;
            speed.x = -wallJumpSpeed;
            speed.y = maxJumpSpeed;
            play_sound(gameState->wallJumpSound);
          }
        }
      }
    }

    varJumpTimer += dt;
  }

  // Gravity
  if(!grabbingWall)
  {
    speed.y = approach(speed.y, fallSpeed, gravity * dt);
  }

  // Wall Grabbing
  {
    grabbingWall = false;
    if(is_down(INPUT_WALL_GRAB) &&
       wallJumpTimer == 0.0f)
    {
      for(int solidIdx = 0; solidIdx < ArraySize(solids); solidIdx++)
      {
        IRect playerRect = get_player_rect();
        playerRect.pos.x -= 2 * 6;
        playerRect.size.x += 4 * 6;
        IRect solidRect = solids[solidIdx];

        if(rect_collision(solidRect, playerRect))
        {
          int playerRectLeft = playerRect.pos.x;
          int playerRectRight = playerRect.pos.x + playerRect.size.x;
          int solidRectLeft = solidRect.pos.x;
          int solidRectRight = solidRect.pos.x + solidRect.size.x;

          // Colliding on the Right
          if(solidRectRight - playerRectLeft <
             playerRectRight - solidRectLeft)
          {
            speed.x = 0;
            grabbingWall = true;
          }

          // Colliding on the Left
          if(solidRectRight - playerRectLeft >
             playerRectRight - solidRectLeft)
          {
            speed.x = 0;
            grabbingWall = true;
          }
        }
      }
    }

    wallJumpTimer = max(0.0f, wallJumpTimer - dt);

    if(grabbingWall &&
      is_down(INPUT_MOVE_UP))
    {
      float mult = 1.0f;
      if(speed.y > 0.0f)
      {
        mult = directionChangeMult;
      }
      speed.y = approach(speed.y, wallClimbSpeed, runAcceleration * mult * dt);
    }

    if(grabbingWall &&
      is_down(INPUT_MOVE_DOWN))
    {
      float mult = 1.0f;
      if(speed.y < 0.0f)
      {
        mult = directionChangeMult;
      }
      speed.y = approach(speed.y, wallSlideDownSpeed, runAcceleration * mult * dt);
    }

    // firction
    if(grabbingWall &&
      !is_down(INPUT_MOVE_UP) &&
      !is_down(INPUT_MOVE_DOWN))
    {
      speed.y = approach(speed.y, 0, runReduce * dt);
    }
  }

  // Move X from https://maddythorson.medium.com/celeste-and-towerfall-physics-d24bd2ae0fc5
  {
    float amount = speed.x;
    xRemainder += amount;
    int move = round(xRemainder);
    if (move != 0)
    {
      xRemainder -= move;
      int moveSign = sign(move);
      while (move != 0)
      {
        bool collisionHappened = false;
        for(int solidIdx = 0; solidIdx < ArraySize(solids); solidIdx++)
        {
          IRect playerRect = get_player_rect();
          IRect newPlayerRect = playerRect;
          newPlayerRect.pos.x += moveSign;
          IRect solidRect = solids[solidIdx];

          if(rect_collision(solidRect, newPlayerRect))
          {
            collisionHappened = true;
            break;
          }
        }

        if(!collisionHappened)
        {
          //There is no Solid immediately beside us, move
          gameState->playerPos.x += moveSign;
          move -= moveSign;
        }
        else
        {
          //Hit a solid! Don't move!
          break;
        }
      }
    }
  }

  // Move Y
  {
    float amount = speed.y;
    yRemainder += amount;
    int move = round(yRemainder);
    if (move != 0)
    {
      yRemainder -= move;
      int moveSign = sign(move);
      while (move != 0)
      {
        bool collisionHappened = false;
        for(int solidIdx = 0; solidIdx < ArraySize(solids); solidIdx++)
        {
          IRect playerRect = get_player_rect();
          IRect newPlayerRect = playerRect;
          newPlayerRect.pos.y += moveSign;
          IRect solidRect = solids[solidIdx];

          if(rect_collision(solidRect, newPlayerRect))
          {
            collisionHappened = true;
            break;
          }
        }

        if(!collisionHappened)
        {
          // There is no Solid immediately beside us, move
          gameState->playerPos.y += moveSign;
          move -= moveSign;
        }
        else
        {
          // Hit a solid! Don't move!
          if(moveSign < 0)
          {
            speed.y = 0.0f;
          }
          if(moveSign > 0)
          {
            playerGrounded = true;
          }
          break;
        }
      }
    }
  }

  IRect playerRect = get_player_rect();
  draw_quad(playerRect.pos, playerRect.size);
  draw_sprite(SPRITE_CELESTE_01, vec_2(gameState->playerPos));
  // draw_quad(gameState.playerPos, {50.0f, 50.0f});
}

// #############################################################################
//                           Implementations
// #############################################################################
bool is_down(GameInputType type)
{
  return gameState->gameInput[type].isDown;
}

bool just_pressed(GameInputType type)
{
  return gameState->gameInput[type].justPressed;
}

void update_game_input()
{
  // Moving
  gameState->gameInput[INPUT_MOVE_LEFT].isDown = input->keys[KEY_A].isDown;
  gameState->gameInput[INPUT_MOVE_RIGHT].isDown = input->keys[KEY_D].isDown;
  gameState->gameInput[INPUT_MOVE_UP].isDown = input->keys[KEY_W].isDown;
  gameState->gameInput[INPUT_MOVE_DOWN].isDown = input->keys[KEY_S].isDown;
  gameState->gameInput[INPUT_MOVE_LEFT].isDown |= input->keys[KEY_LEFT].isDown;
  gameState->gameInput[INPUT_MOVE_RIGHT].isDown |= input->keys[KEY_RIGHT].isDown;
  gameState->gameInput[INPUT_MOVE_UP].isDown |= input->keys[KEY_UP].isDown;
  gameState->gameInput[INPUT_MOVE_DOWN].isDown |= input->keys[KEY_DOWN].isDown;

  // Jumping
  gameState->gameInput[INPUT_JUMP].justPressed =
    input->keys[KEY_SPACE].justPressed;
  gameState->gameInput[INPUT_JUMP].isDown =
    input->keys[KEY_SPACE].isDown;

  // Wall Grabbing
  gameState->gameInput[INPUT_WALL_GRAB].isDown =
    input->keys[KEY_E].isDown;
  gameState->gameInput[INPUT_WALL_GRAB].isDown |=
    input->keys[KEY_Q].isDown;
}

void move_x(float amount)
{
}

IRect get_player_rect()
{
  return
  {
    gameState->playerPos.x + 30,
    gameState->playerPos.y + 12,
    9 * 6,
    16 * 6
  };
}