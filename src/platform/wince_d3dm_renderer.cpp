#include "platform/wince_d3dm_renderer.h"

#include <d3dm.h>
#include <d3dmx.h>
#include <math.h>
#include <string.h>

#include "world/level/world.h"
#include "world/level/chunk/chunk.h"

#define D3DM_VERTEX_CAPACITY 4096
#define PLAYER_EYE 1.62f
#define PLAYER_RADIUS 0.28f
#define PLAYER_HEIGHT 1.80f
#define MOVE_SPEED 4.2f
#define FLY_SPEED 5.0f
#define GRAVITY 12.0f
#define JUMP_SPEED 5.0f
#define TURN_SPEED 85.0f
#define D3DM_VIEW_DISTANCE 56.0f

struct D3DMVertex
{
    float x, y, z;
    DWORD color;
};

#define D3DM_VERTEX_FVF (D3DMFVF_XYZ_FLOAT | D3DMFVF_DIFFUSE)

static bool solidAt(const World* w, int x, int y, int z)
{
    unsigned char id = worldBlock(w, x, y, z);
    if (id == BLOCK_AIR) return false;
    if (!Tile::tiles[id])
        return id != BLOCK_WATER && id != BLOCK_CALM_WATER &&
               id != BLOCK_LAVA && id != BLOCK_CALM_LAVA;
    return Tile::tiles[id]->solidPhys;
}

static bool cameraBlocked(const World* w, float x, float eyeY, float z)
{
    const float feet = eyeY - PLAYER_EYE;
    const float head = feet + PLAYER_HEIGHT - 0.08f;
    const float r = PLAYER_RADIUS;
    const int xs[3] = {
        (int)floorf(x - r), (int)floorf(x), (int)floorf(x + r)
    };
    const int zs[3] = {
        (int)floorf(z - r), (int)floorf(z), (int)floorf(z + r)
    };

    int i, j;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j) {
            if (solidAt(w, xs[i], (int)floorf(feet + 0.05f), zs[j])) return true;
            if (solidAt(w, xs[i], (int)floorf(head), zs[j])) return true;
        }
    return false;
}

static bool hasGround(const World* w, float x, float eyeY, float z)
{
    const float feet = eyeY - PLAYER_EYE;
    const int bx = (int)floorf(x);
    const int bz = (int)floorf(z);
    const int by = (int)floorf(feet - 0.08f);
    return solidAt(w, bx, by, bz);
}

static float degToRad(float v)
{
    return v * 0.01745329252f;
}

WinceD3DMRenderer::WinceD3DMRenderer()
    : x(0.0f), y(0.0f), z(0.0f), yaw(0.0f), pitch(0.0f),
      grounded(false), m_world(0), m_d3dm(0), m_device(0),
      m_vertexBuffer(0), m_hwnd(0), m_vertexCapacity(D3DM_VERTEX_CAPACITY),
      m_verticalVelocity(0.0f), m_ready(false), m_hasDepth(false)
{
}

WinceD3DMRenderer::~WinceD3DMRenderer()
{
    shutdown();
}

bool WinceD3DMRenderer::createPresentParameters(HWND hwnd, bool depth)
{
    D3DMPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));

    pp.BackBufferWidth = 320;
    pp.BackBufferHeight = 240;
    pp.BackBufferCount = 1;
    pp.BackBufferFormat = D3DMFMT_R5G6B5;
    pp.MultiSampleType = D3DMMULTISAMPLE_NONE;
    pp.SwapEffect = D3DMSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = hwnd;
    pp.Windowed = TRUE;
    pp.EnableAutoDepthStencil = depth ? TRUE : FALSE;
    pp.AutoDepthStencilFormat = depth ? D3DMFMT_D16 : D3DMFMT_UNKNOWN;

    HRESULT hr = m_d3dm->CreateDevice(
        D3DMADAPTER_DEFAULT,
        D3DMDEVTYPE_DEFAULT,
        hwnd,
        0,
        &pp,
        &m_device);

    return SUCCEEDED(hr);
}

bool WinceD3DMRenderer::createDevice(HWND hwnd)
{
    m_d3dm = Direct3DMobileCreate(D3DM_SDK_VERSION);
    if (!m_d3dm)
        return false;

    /*
     * Prefer a real depth buffer. Some old drivers reject the requested
     * format, so retry without depth rather than making D3DM unavailable.
     */
    if (!createPresentParameters(hwnd, true)) {
        if (m_device) {
            m_device->Release();
            m_device = 0;
        }
        if (!createPresentParameters(hwnd, false))
            return false;
        m_hasDepth = false;
    } else {
        m_hasDepth = true;
    }

    D3DMCAPS caps;
    ZeroMemory(&caps, sizeof(caps));
    if (FAILED(m_device->GetDeviceCaps(&caps)))
        return false;

    m_vertexCapacity = D3DM_VERTEX_CAPACITY;
    if (caps.MaxPrimitiveCount > 0) {
        unsigned int maxVerts = (unsigned int)caps.MaxPrimitiveCount * 3u;
        if (maxVerts < (unsigned int)m_vertexCapacity)
            m_vertexCapacity = (int)maxVerts;
    }
    if (m_vertexCapacity < 3)
        return false;

    if (FAILED(m_device->SetRenderState(D3DMRS_LIGHTING, FALSE)))
        return false;
    m_device->SetRenderState(D3DMRS_CULLMODE, D3DMCULL_NONE);
    m_device->SetRenderState(D3DMRS_ZENABLE, m_hasDepth ? TRUE : FALSE);
    m_device->SetRenderState(D3DMRS_ZWRITEENABLE, m_hasDepth ? TRUE : FALSE);
    m_device->SetRenderState(D3DMRS_ALPHABLENDENABLE, FALSE);

    return true;
}

bool WinceD3DMRenderer::createVertexBuffer()
{
    if (!m_device)
        return false;

    D3DMCAPS caps;
    ZeroMemory(&caps, sizeof(caps));
    if (FAILED(m_device->GetDeviceCaps(&caps)))
        return false;

    D3DMPOOL pool = D3DMPOOL_SYSTEMMEM;
    if (caps.SurfaceCaps & D3DMSURFCAPS_VIDVERTEXBUFFER)
        pool = D3DMPOOL_VIDEOMEM;

    HRESULT hr = m_device->CreateVertexBuffer(
        (UINT)(sizeof(D3DMVertex) * m_vertexCapacity),
        0,
        D3DM_VERTEX_FVF,
        pool,
        &m_vertexBuffer);

    return SUCCEEDED(hr) && m_vertexBuffer != 0;
}

void WinceD3DMRenderer::releaseResources()
{
    if (m_vertexBuffer) {
        m_vertexBuffer->Release();
        m_vertexBuffer = 0;
    }
    if (m_device) {
        m_device->Release();
        m_device = 0;
    }
    if (m_d3dm) {
        m_d3dm->Release();
        m_d3dm = 0;
    }
}

bool WinceD3DMRenderer::init(HWND hwnd)
{
    shutdown();
    m_hwnd = hwnd;

    if (!createDevice(hwnd)) {
        releaseResources();
        return false;
    }

    if (!createVertexBuffer()) {
        releaseResources();
        return false;
    }

    m_ready = true;
    return true;
}

void WinceD3DMRenderer::shutdown()
{
    m_ready = false;
    m_hasDepth = false;
    releaseResources();
    m_world = 0;
    m_hwnd = 0;
}

bool WinceD3DMRenderer::isReady() const
{
    return m_ready && m_device != 0 && m_vertexBuffer != 0;
}

void WinceD3DMRenderer::reset(World* world, int spawnX, int spawnY, int spawnZ)
{
    m_world = world;
    x = (float)spawnX + 0.5f;
    y = (float)spawnY + PLAYER_EYE + 0.02f;
    z = (float)spawnZ + 0.5f;
    yaw = 0.0f;
    pitch = 0.0f;
    m_verticalVelocity = 0.0f;
    grounded = m_world ? hasGround(m_world, x, y, z) : false;
}

void WinceD3DMRenderer::update(const bool* keys, float dt)
{
    if (!m_world || !keys)
        return;

    if (dt > 0.05f)
        dt = 0.05f;

    const float turn = TURN_SPEED * dt;

    if (keys[VK_LEFT])  yaw -= turn;
    if (keys[VK_RIGHT]) yaw += turn;
    if (keys[VK_UP])    pitch -= turn;
    if (keys[VK_DOWN])  pitch += turn;
    if (keys['A'])      yaw -= turn * 0.55f;
    if (keys['D'])      yaw += turn * 0.55f;

    if (pitch > 85.0f) pitch = 85.0f;
    if (pitch < -85.0f) pitch = -85.0f;

    const float yawRad = degToRad(yaw);
    const float forwardX = sinf(yawRad);
    const float forwardZ = cosf(yawRad);
    const float rightX = forwardZ;
    const float rightZ = -forwardX;

    float mx = 0.0f, mz = 0.0f;
    if (keys['W']) { mx += forwardX; mz += forwardZ; }
    if (keys['S']) { mx -= forwardX; mz -= forwardZ; }
    if (keys['A']) { mx -= rightX;   mz -= rightZ; }
    if (keys['D']) { mx += rightX;   mz += rightZ; }

    const float ml = sqrtf(mx * mx + mz * mz);
    if (ml > 0.001f) {
        mx /= ml;
        mz /= ml;
        const float speed = keys[VK_SHIFT] ? FLY_SPEED : MOVE_SPEED;
        const float nx = x + mx * speed * dt;
        const float nz = z + mz * speed * dt;
        if (!cameraBlocked(m_world, nx, y, z)) x = nx;
        if (!cameraBlocked(m_world, x, y, nz)) z = nz;
    }

    grounded = hasGround(m_world, x, y, z);

    if (keys[VK_SPACE] && grounded) {
        m_verticalVelocity = JUMP_SPEED;
        grounded = false;
    }

    m_verticalVelocity -= GRAVITY * dt;
    if (grounded && m_verticalVelocity < 0.0f)
        m_verticalVelocity = 0.0f;

    const float ny = y + m_verticalVelocity * dt;
    if (!cameraBlocked(m_world, x, ny, z)) {
        y = ny;
        grounded = false;
    } else {
        if (m_verticalVelocity < 0.0f) {
            const int by = (int)floorf(y - PLAYER_EYE - 0.05f);
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

void WinceD3DMRenderer::setupMatrices()
{
    const float yawRad = degToRad(yaw);
    const float pitchRad = degToRad(pitch);

    const float cp = cosf(pitchRad);
    const float sp = sinf(pitchRad);
    const float cy = cosf(yawRad);
    const float sy = sinf(yawRad);

    D3DMXVECTOR3 eye(x, y, z);
    D3DMXVECTOR3 at(
        x + sy * cp,
        y + sp,
        z + cy * cp);
    D3DMXVECTOR3 up(0.0f, 1.0f, 0.0f);

    D3DMXMATRIX view;
    D3DMXMATRIX proj;
    D3DMXMATRIX world;

    D3DMXMatrixLookAtLH(&view, &eye, &at, &up);
    D3DMXMatrixPerspectiveFovLH(
        &proj,
        degToRad(70.0f),
        320.0f / 240.0f,
        0.1f,
        D3DM_VIEW_DISTANCE);

    D3DMXMatrixIdentity(&world);

    m_device->SetTransform(
        D3DMTS_VIEW, (D3DMMATRIX*)&view, D3DMFMT_D3DMVALUE_FLOAT);
    m_device->SetTransform(
        D3DMTS_PROJECTION, (D3DMMATRIX*)&proj, D3DMFMT_D3DMVALUE_FLOAT);
    m_device->SetTransform(
        D3DMTS_WORLD, (D3DMMATRIX*)&world, D3DMFMT_D3DMVALUE_FLOAT);
}

void WinceD3DMRenderer::drawSection(const void* sectionPtr, int count)
{
    const ChunkSection* section = (const ChunkSection*)sectionPtr;
    if (!section || !section->mesh || count <= 0)
        return;

    const DrawVertex* src = section->mesh;
    int remaining = count;
    int offset = 0;

    while (remaining >= 3) {
        int n = remaining;
        if (n > m_vertexCapacity)
            n = m_vertexCapacity;
        n -= n % 3;
        if (n < 3)
            break;

        void* dst = 0;
        const UINT bytes = (UINT)(n * (int)sizeof(D3DMVertex));

        if (FAILED(m_vertexBuffer->Lock(0, bytes, &dst, 0)) || !dst)
            return;

        D3DMVertex* out = (D3DMVertex*)dst;
        int i;
        for (i = 0; i < n; ++i) {
            out[i].x = (float)src[offset + i].x / 256.0f;
            out[i].y = (float)src[offset + i].y / 256.0f;
            out[i].z = (float)src[offset + i].z / 256.0f;
            out[i].color = src[offset + i].color;
        }
        m_vertexBuffer->Unlock();

        m_device->SetStreamSource(0, m_vertexBuffer, sizeof(D3DMVertex));
        m_device->DrawPrimitive(
            D3DMPT_TRIANGLELIST, 0, (UINT)(n / 3));

        offset += n;
        remaining -= n;
    }
}

void WinceD3DMRenderer::drawMeshSections()
{
    if (!m_world)
        return;

    const int camCX = (int)floorf(x) >> 4;
    const int camCZ = (int)floorf(z) >> 4;
    const int radius = 4;

    int cz;
    for (cz = camCZ - radius; cz <= camCZ + radius; ++cz) {
        int cx;
        if (cz < 0 || cz >= WORLD_CHUNKS_Z)
            continue;

        for (cx = camCX - radius; cx <= camCX + radius; ++cx) {
            if (cx < 0 || cx >= WORLD_CHUNKS_X)
                continue;
            if (!worldChunkReady(m_world, cx, cz))
                continue;

            const ChunkMesh* mesh = worldMesh(m_world, cx, cz);
            const float centerX = mesh->cx;
            const float centerZ = mesh->cz;
            const float dx = centerX - x;
            const float dz = centerZ - z;
            if (dx * dx + dz * dz > D3DM_VIEW_DISTANCE * D3DM_VIEW_DISTANCE)
                continue;

            int si;
            for (si = 0; si < N_SECTIONS; ++si) {
                const ChunkSection* s = &mesh->sec[si];
                if (!s->mesh || s->vertexCount < 3)
                    continue;

                const float centerY = (float)s->oy + 8.0f;
                const float dy = centerY - y;
                if (dx * dx + dy * dy + dz * dz >
                    (D3DM_VIEW_DISTANCE + 16.0f) *
                    (D3DM_VIEW_DISTANCE + 16.0f))
                    continue;

                D3DMXMATRIX model;
                D3DMXMatrixTranslation(
                    &model, (float)s->ox, (float)s->oy, (float)s->oz);
                m_device->SetTransform(
                    D3DMTS_WORLD, (D3DMMATRIX*)&model,
                    D3DMFMT_D3DMVALUE_FLOAT);

                drawSection(s, s->vertexCount);
            }
        }
    }
}

void WinceD3DMRenderer::render()
{
    if (!isReady() || !m_world)
        return;

    m_device->Clear(
        0, NULL, m_hasDepth ? (D3DMCLEAR_TARGET | D3DMCLEAR_ZBUFFER) : D3DMCLEAR_TARGET,
        D3DMCOLOR_XRGB(82, 146, 190), 1.0f, 0);

    if (FAILED(m_device->BeginScene()))
        return;

    setupMatrices();
    drawMeshSections();

    m_device->EndScene();
    m_device->Present(NULL, NULL, NULL, NULL);
}
