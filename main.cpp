// main.cpp
// Simple FPS target practice using raylib
// Save as main.cpp. Requires raylib (https://www.raylib.com/)

#include "raylib.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>

struct Target {
    Vector3 pos;
    float radius;
    Color color;
    bool visible;
    double respawnTime; // in seconds (GetTime())
};

static const int SCREEN_W = 1280;
static const int SCREEN_H = 720;
static const int TARGET_COUNT = 8;
static const float SPAWN_RADIUS_XZ = 40.0f;
static const float SPAWN_MIN_Y = 1.0f;
static const float SPAWN_MAX_Y = 6.0f;
static const float MIN_DIST_FROM_PLAYER = 6.0f;
static const double RESPAWN_DELAY_MIN = 0.4; // seconds
static const double RESPAWN_DELAY_MAX = 1.6; // seconds

static float randf(float a, float b) { return a + ((float)rand()/RAND_MAX) * (b - a); }

Vector3 randomSpawnPos(const Vector3 &playerPos) {
    for (int tries = 0; tries < 16; ++tries) {
        float x = randf(-SPAWN_RADIUS_XZ, SPAWN_RADIUS_XZ);
        float z = randf(-SPAWN_RADIUS_XZ, SPAWN_RADIUS_XZ);
        float y = randf(SPAWN_MIN_Y, SPAWN_MAX_Y);
        Vector3 p = { x, y, z };
        float dx = p.x - playerPos.x;
        float dz = p.z - playerPos.z;
        if (sqrtf(dx*dx + dz*dz) >= MIN_DIST_FROM_PLAYER) return p;
    }
    // Fallback: place somewhere far
    return { SPAWN_RADIUS_XZ, SPAWN_MIN_Y + 1.0f, SPAWN_RADIUS_XZ };
}

int main() {
    srand((unsigned)time(NULL));

    InitWindow(SCREEN_W, SCREEN_H, "FPS Target Practice (C++ / raylib)");
    InitAudioDevice();

    // Simple beep sounds
    Wave wave = GenWaveTone(440, 0.08f); // used for hit; GenWaveTone helper used below
    Sound hitSound = LoadSoundFromWave(wave);
    UnloadWave(wave); // sound now contains data

    SetTargetFPS(60);

    // Camera (first-person)
    Camera camera = { 0 };
    camera.position = { 0.0f, 2.0f, 10.0f }; // camera position
    camera.target = { 0.0f, 2.0f, 0.0f };    // camera looking at point
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Hide and capture the cursor so mouse centers for FPS look
    SetMouseMode(MOUSE_DISABLED);

    // Create targets
    std::vector<Target> targets;
    Color colors[] = { RED, LIME, BLUE, GOLD, MAGENTA, SKYBLUE };
    for (int i = 0; i < TARGET_COUNT; ++i) {
        Target t;
        t.radius = 0.9f;
        t.color = colors[i % (sizeof(colors)/sizeof(colors[0]))];
        t.visible = true;
        t.respawnTime = 0.0;
        t.pos = randomSpawnPos(camera.position);
        targets.push_back(t);
    }

    int score = 0;
    int hits = 0;

    // Main loop
    while (!WindowShouldClose()) {
        // Update camera in a first-person mode
        UpdateCamera(&camera, CAMERA_FIRST_PERSON);

        // Shooting: use center of screen ray
        Vector2 centerMouse = { (float)SCREEN_W / 2.0f, (float)SCREEN_H / 2.0f };
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Ray ray = GetMouseRay(centerMouse, camera);
            bool didHit = false;
            for (auto &t : targets) {
                if (!t.visible) continue;
                if (CheckCollisionRaySphere(ray, t.pos, t.radius)) {
                    // Register hit
                    t.visible = false;
                    double delay = RESPAWN_DELAY_MIN + ((double)rand()/RAND_MAX) * (RESPAWN_DELAY_MAX - RESPAWN_DELAY_MIN);
                    t.respawnTime = GetTime() + delay;
                    score += 10;
                    hits += 1;
                    PlaySound(hitSound);
                    didHit = true;
                    break; // only one target per shot
                }
            }
            if (!didHit) {
                // optional miss sound (low beep)
                Wave w = GenWaveTone(220, 0.06f);
                Sound s = LoadSoundFromWave(w);
                UnloadWave(w);
                PlaySound(s);
                // schedule immediate unload (safe because we don't block)
                UnloadSound(s);
            }
        }

        // Respawn logic
        double now = GetTime();
        for (auto &t : targets) {
            if (!t.visible && t.respawnTime > 0.0 && now >= t.respawnTime) {
                Vector3 old = t.pos;
                Vector3 newPos;
                int tries = 0;
                do {
                    newPos = randomSpawnPos(camera.position);
                    tries++;
                } while (Vector3Distance(newPos, old) < MIN_DIST_FROM_PLAYER && tries < 8);
                t.pos = newPos;
                t.visible = true;
                t.respawnTime = 0.0;
            }
        }

        // Draw
        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);
                // Floor and grid
                DrawCube({0.0f, -1.0f, 0.0f}, 200.0f, 1.0f, 200.0f, GRAY);
                DrawGrid(20, 5.0f);

                // Draw targets
                for (auto &t : targets) {
                    if (!t.visible) continue;
                    // simple rotation visual: sphere + ring
                    DrawSphere(t.pos, t.radius, t.color);
                    DrawRing(t.pos, t.radius * 1.05f, 0.1f, 32, t.color); // small ring using helper below
                }

            EndMode3D();

            // HUD
            DrawText(TextFormat("Score: %d  Hits: %d  Targets: %d", score, hits, TARGET_COUNT), 10, 10, 20, DARKGRAY);

            // Crosshair
            DrawCircle(SCREEN_W/2, SCREEN_H/2, 4, BLACK);
            DrawCircleLines(SCREEN_W/2, SCREEN_H/2, 4, WHITE);

            DrawFPS(SCREEN_W - 90, 10);
        EndDrawing();
    }

    // Cleanup
    UnloadSound(hitSound);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}

// --- Helper: generate a simple sine wave tone (mono) for quick beeps
// This implements a minimal GenWaveTone used above for dynamic beep generation
Wave GenWaveTone(float freq, float duration) {
    const int sampleRate = 22050;
    int sampleCount = (int)(duration * sampleRate);
    Wave wave = { 0 };
    wave.sampleCount = sampleCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;

    wave.data = (short*)MemAlloc(sampleCount * wave.channels * sizeof(short));
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / (float)sampleRate;
        float s = sinf(2*PI*freq*t);
        // simple envelope (quick)
        float env = 1.0f - (t / duration);
        short val = (short)(s * env * 30000);
        wave.data[i] = val;
    }
    return wave;
}

// --- Helper: Draw a simple ring around a point (approximated by drawing lines in 3D)
void DrawRing(Vector3 center, float radius, float thickness, int slices, Color color) {
    // draw as many small cylinders (approximated with lines)
    for (int i = 0; i < slices; ++i) {
        float a0 = (2*PI*(float)i) / slices;
        float a1 = (2*PI*(float)(i+1)) / slices;
        Vector3 p0 = { center.x + cosf(a0)*radius, center.y, center.z + sinf(a0)*radius };
        Vector3 p1 = { center.x + cosf(a1)*radius, center.y, center.z + sinf(a1)*radius };
        DrawLine3D(p0, p1, color);
    }
}
