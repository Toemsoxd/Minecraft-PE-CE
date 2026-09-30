#ifndef MCPECE_WINCE_RENDERER_H
#define MCPECE_WINCE_RENDERER_H

#include <windows.h>

class FramebufferWince;
struct World;

class WinceRenderer
{
public:
    WinceRenderer();

    void reset(World* world, int spawnX, int spawnY, int spawnZ);
    void update(const bool* keys, float dt);
    void render(FramebufferWince* fb);

    float x;
    float y;
    float z;
    float yaw;
    float pitch;
    bool grounded;

private:
    World* m_world;
    float m_verticalVelocity;
};

#endif
