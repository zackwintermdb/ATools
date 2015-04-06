///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#ifndef TEXTUREMNG_H
#define TEXTUREMNG_H

class CTexture
{
public:
	CTexture();
	CTexture(LPDIRECT3DDEVICE9 device);
	~CTexture();

	void SetDevice(LPDIRECT3DDEVICE9 device);
	ulong Release();

	bool Load(const string& filename,
		uint mipLevels = 1,
		DWORD filter = D3DX_FILTER_TRIANGLE | D3DX_FILTER_MIRROR,
		DWORD mipFilter = D3DX_FILTER_TRIANGLE | D3DX_FILTER_MIRROR,
		D3DCOLOR colorKey = 0,
		bool nonPowerOfTwo = false);

	operator LPDIRECT3DTEXTURE9() const
	{
		return m_texture;
	}

	LPDIRECT3DTEXTURE9 GetTexture() const
	{
		return m_texture;
	}

	uint GetWidth() const
	{
		return m_width;
	}

	uint GetHeight() const
	{
		return m_height;
	}

private:
	LPDIRECT3DDEVICE9 m_device;
	LPDIRECT3DTEXTURE9 m_texture;
	uint m_width;
	uint m_height;
};

class CTextureMng
{
public:
	static CTextureMng* Instance;

	CTextureMng(LPDIRECT3DDEVICE9 device);
	~CTextureMng();

#ifdef GUI_EDITOR
	CTexture* GetGUITexture(const string& filename);
#else // GUI_EDITOR
	void SetModelTexturePath(const string& path);
	CTexture* GetModelTexture(const string& filename);
#ifndef MODEL_EDITOR
	void SetSfxTexturePath(const string& path);
	string GetSfxTexturePath() const;
	CTexture* GetSfxTexture(const string& filename);
#endif // MODEL_EDITOR
#endif // GUI_EDITOR

#ifdef WORLD_EDITOR
	CTexture* GetTerrainTexture(const string& filename);
	CTexture* GetWeatherTexture(const string& filename);
#endif // WORLD_EDITOR

private:
	LPDIRECT3DDEVICE9 m_device;

#ifdef GUI_EDITOR
	QMap<string, CTexture*> m_guiTextures;
#else // GUI_EDITOR
	string m_modelTexturePath;
	QMap<string, CTexture*> m_modelTextures;
#ifndef MODEL_EDITOR
	string m_sfxTexturePath;
	QMap<string, CTexture*> m_sfxTextures;
#endif // MODEL_EDITOR
#endif // GUI_EDITOR

#ifdef WORLD_EDITOR
	QMap<string, CTexture*> m_terrainTextures;
	QMap<string, CTexture*> m_weatherTextures;
#endif // WORLD_EDITOR
};

#define TextureMng	CTextureMng::Instance

#endif // TEXTUREMNG_H