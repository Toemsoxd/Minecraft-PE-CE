#ifndef MCPECE_WINCE_D3DM_RENDERER_H
#define MCPECE_WINCE_D3DM_RENDERER_H

#include <windows.h>
#include <d3dm.h>

class FramebufferWince;
struct World;

class WinceD3DMRenderer
{
public:
    WinceD3DMRenderer();
    ~WinceD3DMRenderer();

    bool init(HWND hwnd);
    void shutdown();
    bool isReady() const;

    void reset(World* world, int spawnX, int spawnY, int spawnZ);
    void update(const bool* keys, float dt);
    void render();

    float x;
    float y;
    float z;
    float yaw;
    float pitch;
    bool grounded;

private:
    bool createDevice(HWND hwnd);
    bool createVertexBuffer();
    bool createPresentParameters(HWND hwnd, bool depth);
    void releaseResources();

    void setupMatrices();
    void drawSection(const void* section, int count);
    void drawMeshSections();

    World* m_world;
    IDirect3DMobile* m_d3dm;
    IDirect3DMobileDevice* m_device;
    IDirect3DMobileVertexBuffer* m_vertexBuffer;
    HWND m_hwnd;
    int m_vertexCapacity;
    float m_verticalVelocity;
    bool m_ready;
    bool m_hasDepth;
};

#endif
