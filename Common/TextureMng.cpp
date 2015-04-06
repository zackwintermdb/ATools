///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#include <stdafx.h>
#include "TextureMng.h"

CTextureMng* TextureMng = null;

CTextureMng::CTextureMng(LPDIRECT3DDEVICE9 device)
	: m_device(device)
{
	TextureMng = this;
#ifndef GUI_EDITOR
	m_modelTexturePath = "Model/";
#ifndef MODEL_EDITOR
	m_sfxTexturePath = "SFX/Texture/";
#endif // MODEL_EDITOR
#endif // GUI_EDITOR
}

CTextureMng::~CTextureMng()
{
#ifdef GUI_EDITOR
	for (auto it = m_guiTextures.begin(); it != m_guiTextures.end(); it++)
		Delete(it.value());
#else // GUI_EDITOR
	for (auto it = m_modelTextures.begin(); it != m_modelTextures.end(); it++)
		Delete(it.value());
#ifndef MODEL_EDITOR
	for (auto it = m_sfxTextures.begin(); it != m_sfxTextures.end(); it++)
		Delete(it.value());
#endif // MODEL_EDITOR
#endif // GUI_EDITOR

#ifdef WORLD_EDITOR
	for (auto it = m_terrainTextures.begin(); it != m_terrainTextures.end(); it++)
		Delete(it.value());
	for (auto it = m_weatherTextures.begin(); it != m_weatherTextures.end(); it++)
		Delete(it.value());
#endif // WORLD_EDITOR

	TextureMng = null;
}

#ifdef GUI_EDITOR
CTexture* CTextureMng::GetGUITexture(const string& filename)
{
	const string name = filename.toLower();

	auto it = m_guiTextures.find(name);
	if (it != m_guiTextures.end())
		return it.value();

	CTexture* texture = new CTexture(m_device);

	bool nonPowerOfTwo = false;
	const string ext = GetExtension(filename);
	if (ext == "tga"
		|| ext == "bmp")
		nonPowerOfTwo = true;

	if (!texture->Load("Theme/Default/" % filename,
		0,
		D3DX_FILTER_TRIANGLE | D3DX_FILTER_MIRROR,
		D3DX_FILTER_TRIANGLE | D3DX_FILTER_MIRROR,
		0xffff00ff,
		nonPowerOfTwo))
	{
		qWarning(("Can't load GUI texture from file 'Theme/Default/" % filename % "'").toLocal8Bit().data());
		Delete(texture);
	}

	m_guiTextures[name] = texture;
	return texture;
}
#else // GUI_EDITOR
void CTextureMng::SetModelTexturePath(const string& path)
{
	m_modelTexturePath = path;
}

CTexture* CTextureMng::GetModelTexture(const string& filename)
{
	const string name = filename.toLower();

	auto it = m_modelTextures.find(name);
	if (it != m_modelTextures.end())
		return it.value();

	string filepath;

#ifdef MODEL_EDITOR
	if (!QFileInfo(m_modelTexturePath % filename).exists())
	{
		if (!QFileInfo(m_modelTexturePath % "Texture/" % filename).exists())
		{
			if (!QFileInfo(m_modelTexturePath % "TextureMid/" % filename).exists())
			{
				if (!QFileInfo(m_modelTexturePath % "TextureLow/" % filename).exists())
				{
					qWarning(("Can't load model texture from file '" % filename % "'").toLocal8Bit().data());
					m_modelTextures[name] = null;
					return null;
				}
				else
					filepath = m_modelTexturePath % "TextureLow/";
			}
			else
				filepath = m_modelTexturePath % "TextureMid/";
		}
		else
			filepath = m_modelTexturePath % "Texture/";
	}
	else
		filepath = m_modelTexturePath;
#else
	if (!QFileInfo(m_modelTexturePath % "TextureMid/" % filename).exists())
	{
		if (!QFileInfo(m_modelTexturePath % "TextureLow/" % filename).exists())
		{
			if (!QFileInfo(m_modelTexturePath % "Texture/" % filename).exists())
			{
				qWarning(("Can't load model texture from file '" % filename % "'").toLocal8Bit().data());
				m_modelTextures[name] = null;
				return null;
			}
			else
				filepath = m_modelTexturePath % "Texture/";
		}
		else
			filepath = m_modelTexturePath % "TextureLow/";
	}
	else
		filepath = m_modelTexturePath % "TextureMid/";
#endif // MODEL_EDITOR

	CTexture* texture = new CTexture(m_device);

	if (!texture->Load(filepath % filename, 4))
	{
		qWarning(("Can't load model texture from file '" % filename % "'").toLocal8Bit().data());
		Delete(texture);
	}

	m_modelTextures[name] = texture;
	return texture;
}

#ifndef MODEL_EDITOR
void CTextureMng::SetSfxTexturePath(const string& path)
{
	m_sfxTexturePath = path;
}

string CTextureMng::GetSfxTexturePath() const
{
	return m_sfxTexturePath;
}

CTexture* CTextureMng::GetSfxTexture(const string& filename)
{
	const string name = filename.toLower();

	auto it = m_sfxTextures.find(name);
	if (it != m_sfxTextures.end())
		return it.value();

	CTexture* texture = new CTexture(m_device);

	if (!texture->Load(m_sfxTexturePath % filename, D3DX_DEFAULT))
	{
		qWarning(("Can't load sfx texture from file '" % filename % "'").toLocal8Bit().data());
		Delete(texture);
	}

	m_sfxTextures[name] = texture;
	return texture;
}
#endif // MODEL_EDITOR
#endif // GUI_EDITOR

#ifdef WORLD_EDITOR
CTexture* CTextureMng::GetTerrainTexture(const string& filename)
{
	const string name = filename.toLower();

	auto it = m_terrainTextures.find(name);
	if (it != m_terrainTextures.end())
		return it.value();

	string filepath;
	if (!QFileInfo("World/TextureMid/" % filename).exists())
	{
		if (!QFileInfo("World/TextureLow/" % filename).exists())
		{
			if (!QFileInfo("World/Texture/" % filename).exists())
			{
				qWarning(("Can't load terrain texture from file '" % filename % "'").toLocal8Bit().data());
				m_terrainTextures[name] = null;
				return null;
			}
			else
				filepath = "World/Texture/";
		}
		else
			filepath = "World/Texturelow/";
	}
	else
		filepath = "World/TextureMid/";

	CTexture* texture = new CTexture(m_device);

	if (!texture->Load(filepath % filename, 4))
	{
		qWarning(("Can't load terrain texture from file '" % filename % "'").toLocal8Bit().data());
		Delete(texture);
	}

	m_terrainTextures[name] = texture;
	return texture;
}

CTexture* CTextureMng::GetWeatherTexture(const string& filename)
{
	const string name = filename.toLower();

	auto it = m_weatherTextures.find(name);
	if (it != m_weatherTextures.end())
		return it.value();

	CTexture* texture = new CTexture(m_device);

	if (!texture->Load("Weather/" % filename))
	{
		qWarning(("Can't load weather texture from file '" % filename % "'").toLocal8Bit().data());
		Delete(texture);
	}

	m_weatherTextures[name] = texture;
	return texture;
}
#endif // WORLD_EDITOR

CTexture::CTexture()
	: m_device(null),
	m_texture(null),
	m_width(0),
	m_height(0)
{
}

CTexture::CTexture(LPDIRECT3DDEVICE9 device)
	: m_device(device),
	m_texture(null),
	m_width(0),
	m_height(0)
{
}

CTexture::~CTexture()
{
	if (m_texture)
		m_texture->Release();
}

ulong CTexture::Release()
{
	if (m_texture)
	{
		m_width = 0;
		m_height = 0;
		ulong r = m_texture->Release();
		m_texture = null;
		return r;
	}
	else
		return 0;
}

void CTexture::SetDevice(LPDIRECT3DDEVICE9 device)
{
	m_device = device;
}

bool CTexture::Load(const string& filename,
	uint mipLevels,
	DWORD filter,
	DWORD mipFilter,
	D3DCOLOR colorKey,
	bool nonPowerOfTwo)
{
	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly))
		return false;

	Release();

	const QByteArray data = file.readAll();

	D3DXIMAGE_INFO infos;
	const HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(m_device, data.constData(),
		(uint)data.size(),
		nonPowerOfTwo ? D3DX_DEFAULT_NONPOW2 : D3DX_DEFAULT,
		nonPowerOfTwo ? D3DX_DEFAULT_NONPOW2 : D3DX_DEFAULT,
		mipLevels,
		0,
		D3DFMT_UNKNOWN,
		D3DPOOL_MANAGED,
		filter,
		mipFilter,
		colorKey,
		&infos, null, &m_texture);

	file.close();

	if (SUCCEEDED(hr))
	{
		m_width = infos.Width;
		m_height = infos.Height;
		return true;
	}

	return false;
}