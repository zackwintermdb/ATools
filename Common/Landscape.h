///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#ifndef LANDSCAPE_H
#define LANDSCAPE_H

#include "World.h"

class CLandscape;
class CObject;

struct WaterHeight
{
	byte height;
	byte texture;
};

struct WaterVertexBuffer
{
	uint vertexCount;
	LPDIRECT3DVERTEXBUFFER9 VB;
};

struct LandLayer
{
	int textureID;
	CTexture* texture;
	bool patchEnable[NUM_PATCHES_PER_SIDE * NUM_PATCHES_PER_SIDE];
	LPDIRECT3DTEXTURE9 lightMap;
	byte alphaMap[MAP_SIZE * MAP_SIZE];
};

class CLandPatch
{
public:
	CLandPatch();
	
	void Init(LPDIRECT3DDEVICE9 device, int posX, int posY, int X, int Y, float* heightMap);
	void CalculateBounds();
	void Cull();
	void Render() const;

	float GetMinHeight() const {
		return m_bounds[0].y;
	}

private:
	LPDIRECT3DDEVICE9 m_device;
	bool m_dirty;
	bool m_visible;
	float* m_heightMap;
	int m_posX;
	int m_posY;
	int m_offset;
	D3DXVECTOR3 m_center;
	D3DXVECTOR3 m_bounds[8];
	int m_level;
	int m_topLevel;
	int m_bottomLevel;
	int m_leftLevel;
	int m_rightLevel;

	void _calculateLevel();

	friend class CLandscape;
	friend class CWorld;
};

class CLandscape
{
public:
	CLandscape(LPDIRECT3DDEVICE9 device, CWorld* world, int posX, int posY);
	~CLandscape();

	bool Load(const string& filename);
	bool Save(const string& filename);

	LandLayer* GetLayer(int textureID);
	float GetHeight(int offset) const;
	float GetHeight(float x, float z) const;
	float GetHeightAttribute(int offset) const;
	float GetMaxHeight() const;
	int GetTextureID(const D3DXVECTOR3& pos);
	
	void Render();
	void RenderWater();
	void RenderAttributes();
	void MakeWaterVertexBuffer();
	void MakeAttributesVertexBuffer();
	void UpdateTextureLayers();
	void UpdateCull();

	WaterHeight* GetWaterHeight(int x, int z) {
		return &m_waterHeight[x + z * NUM_PATCHES_PER_SIDE];
	}
	D3DXVECTOR3 GetPosition() const {
		return D3DXVECTOR3(m_posX * MPU, 0, m_posY * MPU);
	}

#ifdef WORLD_EDITOR
	void SetHeight(int x, int z, float height);
	bool SetLayerAlpha(int x, int z, int textureID, byte alpha);
#endif // WORLD_EDITOR

private:
	LPDIRECT3DDEVICE9 m_device;
	int m_posX;
	int m_posY;
	float m_heightMap[(MAP_SIZE + 1) * (MAP_SIZE + 1)];
	WaterHeight m_waterHeight[NUM_PATCHES_PER_SIDE * NUM_PATCHES_PER_SIDE];
	CPtrArray<LandLayer> m_layers;
	LPDIRECT3DVERTEXBUFFER9 m_VB;
	LPDIRECT3DVERTEXBUFFER9 m_attrVB;
	bool m_dirty;
	bool m_visible;
	CLandPatch m_patches[NUM_PATCHES_PER_SIDE][NUM_PATCHES_PER_SIDE];
	D3DXVECTOR3 m_bounds[8];
	WaterVertexBuffer m_cloudVB;
	WaterVertexBuffer* m_waterVBs;
	CWorld* m_world;
	uint m_attrVertexCount;
	byte m_colorMap[MAP_SIZE * MAP_SIZE * 3];
	CObjectArray m_objects;

	void _setVertices();
	void _calculateBounds();
	void _cull();
	void _initPatches();

	friend class CWorld;

#ifdef WORLD_EDITOR
	WORLD_EDITOR_FRIENDS
#endif // WORLD_EDITOR
};

#endif // LANDSCAPE_H