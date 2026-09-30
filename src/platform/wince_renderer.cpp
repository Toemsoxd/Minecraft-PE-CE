#include "platform/wince_renderer.h"
#include "platform/framebuffer_wince.h"
#include "world/level/world.h"
#include "world/level/tile/tile.h"
#include <math.h>

#define RENDER_W 160
#define RENDER_H 120
#define RENDER_SCALE 2
#define MAX_RAY_STEPS 96
#define PLAYER_HEIGHT 1.80f
#define PLAYER_EYE 1.62f
#define PLAYER_RADIUS 0.28f
#define MOVE_SPEED 4.2f
#define FLY_SPEED 5.0f
#define GRAVITY 12.0f
#define JUMP_SPEED 5.0f
#define MOUSELESS_TURN 85.0f

struct RayHit {
    bool hit;
    int x, y, z;
    int face;
    float distance;
    unsigned char id;
};

static bool solidAt(const World* w, int x, int y, int z)
{
    unsigned char id = worldBlock(w, x, y, z);
    if (id == BLOCK_AIR) return false;
    if (!Tile::tiles[id]) return id != BLOCK_WATER && id != BLOCK_CALM_WATER &&
                                      id != BLOCK_LAVA && id != BLOCK_CALM_LAVA;
    return Tile::tiles[id]->solidPhys;
}

static bool cameraBlocked(const World* w, float x, float eyeY, float z)
{
    const float feet = eyeY - PLAYER_EYE;
    const float head = feet + PLAYER_HEIGHT - 0.08f;
    const float r = PLAYER_RADIUS;
    int xs[3];
    int zs[3];
    int i, j;

    xs[0] = (int)floorf(x - r);
    xs[1] = (int)floorf(x);
    xs[2] = (int)floorf(x + r);
    zs[0] = (int)floorf(z - r);
    zs[1] = (int)floorf(z);
    zs[2] = (int)floorf(z + r);

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++) {
            if (solidAt(w, xs[i], (int)floorf(feet + 0.05f), zs[j])) return true;
            if (solidAt(w, xs[i], (int)floorf(head), zs[j])) return true;
        }
    return false;
}

static bool hasGround(const World* w, float x, float eyeY, float z)
{
    float feet = eyeY - PLAYER_EYE;
    int bx = (int)floorf(x);
    int bz = (int)floorf(z);
    int by = (int)floorf(feet - 0.08f);
    return solidAt(w, bx, by, bz);
}

static unsigned int blockBaseColor(unsigned char id, unsigned char data)
{
    switch (id) {
    case BLOCK_GRASS:      return 0xFF69A84Fu;
    case BLOCK_DIRT:       return 0xFF8B5A32u;
    case BLOCK_STONE:      return 0xFF888888u;
    case BLOCK_COBBLESTONE:return 0xFF666666u;
    case BLOCK_SAND:       return 0xFFE0C47Au;
    case BLOCK_GRAVEL:     return 0xFF8C877Fu;
    case BLOCK_SANDSTONE:  return 0xFFD4B878u;
    case BLOCK_LOG:        return 0xFF76502Eu;
    case BLOCK_LEAVES:     return 0xFF3F8B42u;
    case BLOCK_PLANKS:     return 0xFFB58450u;
    case BLOCK_BEDROCK:    return 0xFF333333u;
    case BLOCK_COAL_BLOCK: return 0xFF292929u;
    case BLOCK_IRON_BLOCK: return 0xFFB9B9B9u;
    case BLOCK_GOLD_BLOCK: return 0xFFE3BE38u;
    case BLOCK_DIAMOND_BLOCK:return 0xFF46D6D0u;
    case BLOCK_OBSIDIAN:   return 0xFF33224Au;
    case BLOCK_BRICKS:     return 0xFF9C4C3Du;
    case BLOCK_GLASS:     return 0xFFB9DDE8u;
    case BLOCK_GLOWSTONE:  return 0xFFE7C86Au;
    case BLOCK_NETHERRACK: return 0xFF713F3Bu;
    case BLOCK_NETHER_BRICK:return 0xFF33262Au;
    case BLOCK_SNOW_BLOCK:return 0xFFE9E9E9u;
    case BLOCK_ICE:       return 0xFF9BD4E8u;
    case BLOCK_CLAY:      return 0xFF9AA7A9u;
    case BLOCK_CACTUS:    return 0xFF4E963Du;
    case BLOCK_MELON:     return 0xFF719C42u;
    case BLOCK_PUMPKIN:   return 0xFFD27D2Eu;
    case BLOCK_TNT:       return 0xFFC83C35u;
    case BLOCK_WOOL:
        switch (data & 15) {
        case 0: return 0xFFD7D7D7u;
        case 1: return 0xFFD84A42u;
        case 2: return 0xFF3C9E3Cu;
        case 3: return 0xFF3D7AC6u;
        case 4: return 0xFFE0B52Fu;
        case 5: return 0xFFB65A31u;
        case 6: return 0xFF6A4EA2u;
        case 7: return 0xFF777777u;
        default:return 0xFFB5B5B5u;
        }
    default:
        return 0xFF9A9A9Au;
    }
}

static unsigned int shadeColor(unsigned int c, float shade)
{
    int r = (int)(((c >> 16) & 255) * shade);
    int g = (int)(((c >> 8) & 255) * shade);
    int b = (int)((c & 255) * shade);
    if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
    if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
    return 0xFF000000u | ((unsigned int)r << 16) |
           ((unsigned int)g << 8) | (unsigned int)b;
}

static RayHit traceRay(const World* w, float ox, float oy, float oz,
                       float dx, float dy, float dz)
{
    RayHit h;
    h.hit = false; h.x = h.y = h.z = 0; h.face = 0; h.distance = 0.0f; h.id = 0;

    int ix = (int)floorf(ox);
    int iy = (int)floorf(oy);
    int iz = (int)floorf(oz);

    int sx = dx < 0.0f ? -1 : 1;
    int sy = dy < 0.0f ? -1 : 1;
    int sz = dz < 0.0f ? -1 : 1;

    float adx = dx < 0.0f ? -dx : dx;
    float ady = dy < 0.0f ? -dy : dy;
    float adz = dz < 0.0f ? -dz : dz;

    float txDelta = adx > 0.00001f ? 1.0f / adx : 1.0e30f;
    float tyDelta = ady > 0.00001f ? 1.0f / ady : 1.0e30f;
    float tzDelta = adz > 0.00001f ? 1.0f / adz : 1.0e30f;

    float txMax = adx > 0.00001f
        ? (dx < 0.0f ? (ox - (float)ix) : ((float)ix + 1.0f - ox)) * txDelta
        : 1.0e30f;
    float tyMax = ady > 0.00001f
        ? (dy < 0.0f ? (oy - (float)iy) : ((float)iy + 1.0f - oy)) * tyDelta
        : 1.0e30f;
    float tzMax = adz > 0.00001f
        ? (dz < 0.0f ? (oz - (float)iz) : ((float)iz + 1.0f - oz)) * tzDelta
        : 1.0e30f;

    int face = 0;
    int step;
    for (step = 0; step < MAX_RAY_STEPS; step++) {
        unsigned char id = worldBlock(w, ix, iy, iz);
        if (id != BLOCK_AIR &&
            id != BLOCK_WATER && id != BLOCK_CALM_WATER &&
            id != BLOCK_LAVA && id != BLOCK_CALM_LAVA &&
            id != BLOCK_FIRE) {
            h.hit = true;
            h.x = ix; h.y = iy; h.z = iz;
            h.face = face;
            h.distance = txMax < tyMax ? (txMax < tzMax ? txMax : tzMax)
                                       : (tyMax < tzMax ? tyMax : tzMax);
            h.id = id;
            return h;
        }

        if (txMax < tyMax) {
            if (txMax < tzMax) { ix += sx; txMax += txDelta; face = sx > 0 ? 4 : 5; }
            else                { iz += sz; tzMax += tzDelta; face = sz > 0 ? 2 : 3; }
        } else {
            if (tyMax < tzMax) { iy += sy; tyMax += tyDelta; face = sy > 0 ? 0 : 1; }
            else                { iz += sz; tzMax += tzDelta; face = sz > 0 ? 2 : 3; }
        }
    }
    return h;
}

static void putLowPixel(FramebufferWince* fb, int x, int y, unsigned int c)
{
    fb->fillRect(x * RENDER_SCALE, y * RENDER_SCALE, RENDER_SCALE, RENDER_SCALE, c);
}

WinceRenderer::WinceRenderer()
    : x(0.0f), y(0.0f), z(0.0f), yaw(0.0f), pitch(0.0f),
      grounded(false), m_world(0), m_verticalVelocity(0.0f)
{
}

void WinceRenderer::reset(World* world, int spawnX, int spawnY, int spawnZ)
{
    m_world = world;
    x = (float)spawnX + 0.5f;
    y = (float)spawnY + PLAYER_EYE + 0.02f;
    z = (float)spawnZ + 0.5f;
    yaw = 0.0f;
    pitch = 0.0f;
    m_verticalVelocity = 0.0f;
    grounded = hasGround(m_world, x, y, z);
}

void WinceRenderer::update(const bool* keys, float dt)
{
    if (!m_world || !keys) return;
    if (dt > 0.05f) dt = 0.05f;

    float turn = MOUSELESS_TURN * dt;
    if (keys[VK_LEFT]) yaw -= turn;
    if (keys[VK_RIGHT]) yaw += turn;
    if (keys[VK_UP]) pitch -= turn;
    if (keys[VK_DOWN]) pitch += turn;
    if (keys['A']) yaw -= turn * 0.55f;
    if (keys['D']) yaw += turn * 0.55f;

    if (pitch > 85.0f) pitch = 85.0f;
    if (pitch < -85.0f) pitch = -85.0f;

    float yawRad = yaw * 0.01745329252f;
    float forwardX = (float)sin(yawRad);
    float forwardZ = (float)cos(yawRad);
    float rightX = forwardZ;
    float rightZ = -forwardX;

    float mx = 0.0f, mz = 0.0f;
    if (keys['W']) { mx += forwardX; mz += forwardZ; }
    if (keys['S']) { mx -= forwardX; mz -= forwardZ; }
    if (keys['A']) { mx -= rightX;   mz -= rightZ; }
    if (keys['D']) { mx += rightX;   mz += rightZ; }

    float ml = sqrtf(mx * mx + mz * mz);
    if (ml > 0.001f) {
        mx /= ml; mz /= ml;
        float speed = keys[VK_SHIFT] ? FLY_SPEED : MOVE_SPEED;
        float nx = x + mx * speed * dt;
        float nz = z + mz * speed * dt;
        if (!cameraBlocked(m_world, nx, y, z)) x = nx;
        if (!cameraBlocked(m_world, x, y, nz)) z = nz;
    }

    grounded = hasGround(m_world, x, y, z);
    if (keys[VK_SPACE] && grounded) {
        m_verticalVelocity = JUMP_SPEED;
        grounded = false;
    }

    m_verticalVelocity -= GRAVITY * dt;
    if (grounded && m_verticalVelocity < 0.0f) m_verticalVelocity = 0.0f;

    float ny = y + m_verticalVelocity * dt;
    if (!cameraBlocked(m_world, x, ny, z)) {
        y = ny;
        grounded = false;
    } else {
        if (m_verticalVelocity < 0.0f) {
            int by = (int)floorf(y - PLAYER_EYE - 0.05f);
            y = (float)by + PLAYER_EYE + 1.0f + 0.01f;
            grounded = true;
        }
        m_verticalVelocity = 0.0f;
    }

    if (y < 2.0f) {
        y = 2.0f;
        m_verticalVelocity = 0.0f;
        grounded = true;
    }
}

void WinceRenderer::render(FramebufferWince* fb)
{
    if (!fb || !m_world) return;

    int py, px;
    float pitchRad = pitch * 0.01745329252f;
    float yawRad = yaw * 0.01745329252f;
    float cy = (float)cos(yawRad), sy = (float)sin(yawRad);
    float cp = (float)cos(pitchRad), sp = (float)sin(pitchRad);

    for (py = 0; py < RENDER_H; py++) {
        for (px = 0; px < RENDER_W; px++) {
            float sx = ((float)px + 0.5f) / (float)RENDER_W * 2.0f - 1.0f;
            float syScreen = 1.0f - ((float)py + 0.5f) / (float)RENDER_H * 2.0f;
            float cameraX = sx * 0.72f;
            float cameraY = syScreen * 0.54f;

            float vx = cameraX;
            float vy = cameraY;
            float vz = 1.0f;

            float pyawX = vx * cy + vz * sy;
            float pyawZ = -vx * sy + vz * cy;
            float dirX = pyawX;
            float dirZ = pyawZ;
            float dirY = vy * cp - pyawZ * sp;
            dirZ = vy * sp + pyawZ * cp;

            float dl = sqrtf(dirX * dirX + dirY * dirY + dirZ * dirZ);
            if (dl > 0.0001f) {
                dirX /= dl; dirY /= dl; dirZ /= dl;
            }

            RayHit hit = traceRay(m_world, x, y, z, dirX, dirY, dirZ);
            unsigned int color;

            if (!hit.hit) {
                float sky = 0.5f + 0.5f * syScreen;
                int r = (int)(80.0f + 55.0f * sky);
                int g = (int)(145.0f + 65.0f * sky);
                int b = (int)(185.0f + 55.0f * sky);
                color = 0xFF000000u | ((unsigned int)r << 16) |
                        ((unsigned int)g << 8) | (unsigned int)b;
            } else {
                unsigned char data = worldData(m_world, hit.x, hit.y, hit.z);
                color = blockBaseColor(hit.id, data);

                float faceShade = 1.0f;
                if (hit.face == 1) faceShade = 1.00f;
                else if (hit.face == 0) faceShade = 0.72f;
                else if (hit.face == 2 || hit.face == 3) faceShade = 0.84f;
                else faceShade = 0.92f;

                int light = lightRawAt(m_world, hit.x, hit.y, hit.z);
                float lightShade = 0.35f + 0.65f * ((float)light / 15.0f);
                color = shadeColor(color, faceShade * lightShade);
            }

            putLowPixel(fb, px, py, color);
        }
    }

    /* Crosshair and minimal HUD are intentionally drawn at full framebuffer size. */
    fb->drawLine(156, 120, 164, 120, 0xFFFFFFFFu);
    fb->drawLine(160, 116, 160, 124, 0xFFFFFFFFu);

    {
        TCHAR buf[64];
        wsprintf(buf, TEXT("%d %d %d"), (int)floorf(x), (int)floorf(y - PLAYER_EYE),
                 (int)floorf(z));
        fb->drawText(4, 4, buf, 0xFFFFFFFFu, false);
    }

    fb->drawText(160, 226, TEXT("WASD MOVE  ARROWS LOOK  SPACE JUMP"),
                 0xFFFFFFFFu, true);
}
