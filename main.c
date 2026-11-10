#include "raylib.h"
#include <math.h>
#include <stdbool.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700
#define MAX_BALLS 120

/*
Learning resources used:
- W3Schools C tutorial
- Learn-C.org
- FreeCodeCamp C videos
- POP lecture materials
*/

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float radius;
    float mass;
    Color color;
    bool active;
} Ball;

static Ball balls[MAX_BALLS];
static int ballCount = 0;

static bool paused = false;
static bool showDebug = true;
static bool gravityWellActive = false;
static Vector2 gravityWellPosition = {0};

static float worldGravity = 900.0f;
static float bounceDamping = 0.82f;
static float airDrag = 0.999f;

static float ClampFloat(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static float RandomFloat(float min, float max) {
    return min + ((float)GetRandomValue(0, 10000) / 10000.0f) * (max - min);
}

static Color RandomBrightColor(void) {
    Color palette[] = {
        {255, 99, 132, 255},
        {54, 162, 235, 255},
        {255, 206, 86, 255},
        {75, 192, 192, 255},
        {153, 102, 255, 255},
        {255, 159, 64, 255},
        {120, 255, 180, 255},
        {255, 120, 220, 255}
    };

    int index = GetRandomValue(0, 7);
    return palette[index];
}

static void ClearBalls(void) {
    for (int i = 0; i < MAX_BALLS; i++) {
        balls[i].active = false;
    }
    ballCount = 0;
}

static void AddBall(Vector2 position, Vector2 velocity) {
    if (ballCount >= MAX_BALLS) return;

    Ball *b = &balls[ballCount];

    b->position = position;
    b->velocity = velocity;
    b->radius = RandomFloat(8.0f, 18.0f);
    b->mass = b->radius * 0.8f;
    b->color = RandomBrightColor();
    b->active = true;

    ballCount++;
}

static void SpawnBurst(Vector2 center, int amount) {
    for (int i = 0; i < amount; i++) {
        Vector2 velocity = {
            RandomFloat(-220.0f, 220.0f),
            RandomFloat(-320.0f, -60.0f)
        };
        AddBall(center, velocity);
    }
}

static void ResetSimulation(void) {
    ClearBalls();
    paused = false;
    showDebug = true;
    gravityWellActive = false;
    gravityWellPosition = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};

    SpawnBurst((Vector2){300.0f, 100.0f}, 12);
    SpawnBurst((Vector2){700.0f, 100.0f}, 12);
}

static void ApplyGravityWell(Ball *b, Vector2 wellPosition, float dt) {
    Vector2 direction = {
        wellPosition.x - b->position.x,
        wellPosition.y - b->position.y
    };

    float distanceSq = direction.x * direction.x + direction.y * direction.y;
    distanceSq = ClampFloat(distanceSq, 120.0f, 50000.0f);

    float distance = sqrtf(distanceSq);

    if (distance > 0.001f) {
        direction.x /= distance;
        direction.y /= distance;

        float strength = 22000.0f / distanceSq;

        b->velocity.x += direction.x * strength * dt * 1000.0f;
        b->velocity.y += direction.y * strength * dt * 1000.0f;
    }
}

static void HandleWallCollisions(Ball *b) {
    if (b->position.x - b->radius < 0.0f) {
        b->position.x = b->radius;
        b->velocity.x *= -bounceDamping;
    }

    if (b->position.x + b->radius > SCREEN_WIDTH) {
        b->position.x = SCREEN_WIDTH - b->radius;
        b->velocity.x *= -bounceDamping;
    }

    if (b->position.y - b->radius < 0.0f) {
        b->position.y = b->radius;
        b->velocity.y *= -bounceDamping;
    }

    if (b->position.y + b->radius > SCREEN_HEIGHT) {
        b->position.y = SCREEN_HEIGHT - b->radius;
        b->velocity.y *= -bounceDamping;

        if (fabsf(b->velocity.y) < 25.0f) {
            b->velocity.y = 0.0f;
        }

        b->velocity.x *= 0.98f;
    }
}

static void ResolveBallCollision(Ball *a, Ball *b) {
    if (!a->active || !b->active) return;

    Vector2 delta = {
        b->position.x - a->position.x,
        b->position.y - a->position.y
    };

    float distanceSq = delta.x * delta.x + delta.y * delta.y;
    float minDistance = a->radius + b->radius;

    if (distanceSq <= 0.0001f) {
        delta.x = 1.0f;
        delta.y = 0.0f;
        distanceSq = 1.0f;
    }

    if (distanceSq < minDistance * minDistance) {
        float distance = sqrtf(distanceSq);
        Vector2 normal = { delta.x / distance, delta.y / distance };

        float overlap = minDistance - distance;
        float totalMass = a->mass + b->mass;

        a->position.x -= normal.x * overlap * (b->mass / totalMass);
        a->position.y -= normal.y * overlap * (b->mass / totalMass);
        b->position.x += normal.x * overlap * (a->mass / totalMass);
        b->position.y += normal.y * overlap * (a->mass / totalMass);

        Vector2 relativeVelocity = {
            b->velocity.x - a->velocity.x,
            b->velocity.y - a->velocity.y
        };

        float velocityAlongNormal =
            relativeVelocity.x * normal.x + relativeVelocity.y * normal.y;

        if (velocityAlongNormal > 0.0f) return;

        float restitution = 0.82f;

        float impulseScalar =
            -(1.0f + restitution) * velocityAlongNormal /
            ((1.0f / a->mass) + (1.0f / b->mass));

        Vector2 impulse = {
            impulseScalar * normal.x,
            impulseScalar * normal.y
        };

        a->velocity.x -= impulse.x / a->mass;
        a->velocity.y -= impulse.y / a->mass;
        b->velocity.x += impulse.x / b->mass;
        b->velocity.y += impulse.y / b->mass;

        a->color = RandomBrightColor();
        b->color = RandomBrightColor();
    }
}

static void HandleInput(void) {
    if (IsKeyPressed(KEY_SPACE)) {
        paused = !paused;
    }

    if (IsKeyPressed(KEY_R)) {
        ResetSimulation();
    }

    if (IsKeyPressed(KEY_C)) {
        ClearBalls();
    }

    if (IsKeyPressed(KEY_D)) {
        showDebug = !showDebug;
    }

    if (IsKeyPressed(KEY_UP)) {
        worldGravity += 50.0f;
    }

    if (IsKeyPressed(KEY_DOWN)) {
        worldGravity -= 50.0f;
        if (worldGravity < 0.0f) {
            worldGravity = 0.0f;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        SpawnBurst(GetMousePosition(), 8);
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        gravityWellActive = true;
        gravityWellPosition = GetMousePosition();
    } else {
        gravityWellActive = false;
    }
}

static void UpdateSimulation(float dt) {
    HandleInput();

    if (paused) return;

    for (int i = 0; i < ballCount; i++) {
        Ball *b = &balls[i];

        if (!b->active) continue;

        b->velocity.y += worldGravity * dt;

        if (gravityWellActive) {
            ApplyGravityWell(b, gravityWellPosition, dt);
        }

        b->velocity.x *= airDrag;
        b->velocity.y *= airDrag;

        b->position.x += b->velocity.x * dt;
        b->position.y += b->velocity.y * dt;

        HandleWallCollisions(b);
    }

    for (int i = 0; i < ballCount; i++) {
        for (int j = i + 1; j < ballCount; j++) {
            ResolveBallCollision(&balls[i], &balls[j]);
        }
    }
}

static void DrawSimulation(void) {
    ClearBackground((Color){18, 18, 28, 255});

    DrawRectangle(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50, (Color){28, 35, 55, 255});
    DrawLine(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, SCREEN_HEIGHT - 50, (Color){90, 140, 200, 255});

    for (int i = 0; i < ballCount; i++) {
        if (!balls[i].active) continue;

        DrawCircleV(balls[i].position, balls[i].radius, balls[i].color);
        DrawCircleLines((int)balls[i].position.x, (int)balls[i].position.y, balls[i].radius, RAYWHITE);
    }

    if (gravityWellActive) {
        DrawCircleV(gravityWellPosition, 18.0f, Fade(PURPLE, 0.35f));
        DrawCircleLines((int)gravityWellPosition.x, (int)gravityWellPosition.y, 18.0f, PURPLE);
        DrawCircleLines((int)gravityWellPosition.x, (int)gravityWellPosition.y, 40.0f, Fade(PURPLE, 0.6f));
    }

    DrawText("Particle Physics Sandbox", 20, 18, 32, RAYWHITE);
    DrawText("Left click: spawn burst", 20, 62, 20, LIGHTGRAY);
    DrawText("Right click hold: gravity well", 20, 88, 20, LIGHTGRAY);
    DrawText("SPACE: pause  R: reset  C: clear  D: debug", 20, 114, 20, LIGHTGRAY);
    DrawText("UP/DOWN: change gravity", 20, 140, 20, LIGHTGRAY);

    if (paused) {
        DrawText("PAUSED", SCREEN_WIDTH - 170, 20, 34, YELLOW);
    }

    if (showDebug) {
        DrawRectangle(20, SCREEN_HEIGHT - 90, 330, 60, Fade(BLACK, 0.5f));
        DrawText(TextFormat("Balls: %d / %d", ballCount, MAX_BALLS), 30, SCREEN_HEIGHT - 78, 20, GREEN);
        DrawText(TextFormat("Gravity: %.0f", worldGravity), 30, SCREEN_HEIGHT - 52, 20, GREEN);
        DrawText(TextFormat("FPS: %d", GetFPS()), 200, SCREEN_HEIGHT - 52, 20, GREEN);
    }
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "POP Project - Particle Physics Sandbox");
    SetTargetFPS(60);

    ResetSimulation();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        UpdateSimulation(dt);

        BeginDrawing();
        DrawSimulation();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}