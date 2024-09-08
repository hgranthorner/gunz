#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include <raylib.h>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr uint32_t TOP_OF_FLOOR = HEIGHT - 20;
constexpr uint32_t PLAYER_MOVE_SPEED = 5;
constexpr uint32_t PLAYER_DASH_SPEED = 50;
constexpr int32_t JUMP_DELTA = -20;

struct Player : Rectangle {
  int delta_x;
  int delta_y;

  void jump() noexcept { this->delta_y = JUMP_DELTA; }
  void dash(const uint32_t key) noexcept {
    switch (key) {
    case KEY_A: {
      this->delta_x -= PLAYER_DASH_SPEED;
    } break;
    case KEY_D: {
      this->delta_x += PLAYER_DASH_SPEED;
    } break;
    case KEY_S:
    case KEY_W:
    default:
      break;
    }
  }
};

void apply_gravity(Player &player, const Rectangle floor,
                   const std::vector<Rectangle> &walls) {
  player.delta_y += 1;
  if (player.delta_y >= 10) {
    player.delta_y = 10;
  }

  auto next_y = player.y + player.delta_y;
  auto next_x = player.x + player.delta_x;

  Rectangle next_rect = {};
  next_rect.x = std::min(next_x, player.x);
  next_rect.y = std::min(next_y, player.y);
  next_rect.width = abs(player.x - next_x) + player.width;
  next_rect.height = abs(player.y - next_y) + player.height;

  const auto one_below = (Rectangle){
      .x = next_x,
      .y = next_y + player.height - 1,
      .width = player.width,
      .height = 2,
  };

  if (CheckCollisionRecs(one_below, floor)) {
    next_y = floor.y - player.height;
    if (player.delta_y > 0)
      player.delta_y = 0;
  }

  for (const auto wall : walls) {
    const auto collision = GetCollisionRec(next_rect, wall);

    if (collision.width != 0 || collision.height != 0) {
      {
        bool started_above = player.y + player.height <= wall.y;
        bool started_below = wall.y + wall.height <= player.y;

        bool ended_above = next_rect.y + next_rect.height <= wall.y;
        bool ended_below = wall.y + wall.height <= next_rect.y;

        if (started_above && !ended_above) {
          next_y = wall.y - player.height;
        }
        if (started_below && !ended_below) {
          next_y = wall.y + wall.height;
        }
      }

      // 4 cases:
      //  1. collided with left; didn't go through
      //  2. collided with left; did go through
      //  3. collided with right; didn't go through
      //  4. collided with right; did go through

      {
        bool started_left = player.x + player.width <= wall.x;
        bool started_right = wall.x + wall.width <= player.x;

        bool ended_left = next_rect.x + next_rect.width <= wall.x;
        bool ended_right = wall.x + wall.width <= next_rect.x;

        if (started_left && !ended_left) {
          next_x = wall.x - player.width;
        }
        if (started_right && !ended_right) {
          next_x = wall.x + wall.width;
        }
      }
    }
  }

  player.x = next_x;
  player.delta_x = 0;
  player.y = next_y;
}

struct KeyDoublePress {
  std::chrono::time_point<std::chrono::system_clock> last_pressed;
  uint32_t key{};
  uint32_t is_double_press(const uint32_t keys[4]) noexcept {
    uint32_t double_pressed = 0;
    for (int i = 0; i < 4; i++) {
      if (const auto current_key = keys[i]; IsKeyPressed(current_key)) {
        auto now = std::chrono::system_clock::now();
        if (this->key == current_key &&
            now < this->last_pressed + std::chrono::milliseconds{300}) {
          double_pressed = current_key;
        }
        this->key = current_key;
        this->last_pressed = now;
      }
    }

    return double_pressed;
  }
};

int main() {
  InitWindow(WIDTH, HEIGHT, "Gunz");

  Player player = {0};
  player.x = 100;
  player.y = 100;
  player.width = 20;
  player.height = 20;
  player.delta_x = 0;
  player.delta_y = 0;
  auto camera = Camera2D{0};
  constexpr Rectangle floor = {
      .x = -100,
      .y = static_cast<float>(TOP_OF_FLOOR),
      .width = 1000,
      .height = 20,
  };

  std::vector<Rectangle> walls;
  walls.push_back((Rectangle){100, TOP_OF_FLOOR - 100, 20, 100});
  walls.push_back((Rectangle){600, 200, 100, 20});

  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  SetTargetFPS(60);
  KeyDoublePress double_press = {
      .last_pressed = std::chrono::system_clock::now(),
      .key = 0,
  };
  constexpr uint32_t keys[4] = {KEY_W, KEY_A, KEY_S, KEY_D};

  while (!WindowShouldClose()) {
    const uint32_t pressed = double_press.is_double_press(keys);
    player.dash(pressed);

    if (IsKeyDown(KEY_A))
      player.delta_x -= PLAYER_MOVE_SPEED;
    if (IsKeyDown(KEY_D))
      player.delta_x += PLAYER_MOVE_SPEED;
    if (IsKeyPressed(KEY_SPACE))
      player.jump();

    apply_gravity(player, floor, walls);

    BeginDrawing();
    {
      ClearBackground(BLACK);

      BeginMode2D(camera);
      {
        DrawRectangleRec(player, GREEN);
        DrawRectangleRec(floor, BLUE);
        for (const auto wall : walls) {
          DrawRectangleRec(wall, ORANGE);
        }
      }
      EndMode2D();

      DrawFPS(10, 10);
    }

    EndDrawing();
  }
}
