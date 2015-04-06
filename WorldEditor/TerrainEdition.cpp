///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#include "stdafx.h"
#include <World.h>
#include "MainFrame.h"
#include <Landscape.h>

void CWorld::EditHeight(const D3DXVECTOR3& pos, const QPoint& mousePos, float baseHeight)
{
	int mode, radius, hardness, attribute;
	bool rounded, useFixedHeight;
	float fixedHeight;
	MainFrame->GetTerrainHeightEditInfos(mode, rounded, radius, hardness, useFixedHeight, fixedHeight, attribute);

	float heightAttribute = 0.0f;
	if (mode == 4)
	{
		switch (attribute)
		{
		case 1:
			heightAttribute = HGT_NOWALK;
			break;
		case 2:
			heightAttribute = HGT_NOFLY;
			break;
		case 3:
			heightAttribute = HGT_NOMOVE;
			break;
		case 4:
			heightAttribute = HGT_DIE;
			break;
		}
	}

	const float cameraFactor = D3DXVec3Length(&(m_cameraPos - pos)) / 180.0f;
	const float radius1 = (float)((radius - 1) * MPU);
	const D3DXVECTOR3 terrainPos((int)(pos.x + ((float)MPU / 2.0f)) / MPU * MPU, 0.0f, (int)(pos.z + ((float)MPU / 2.0f)) / MPU * MPU);
	const float baseHeight2 = useFixedHeight ? fixedHeight : baseHeight;
	const float radius4 = (float)hardness / 100.0f + 0.1f;
	QVector<CLandscape*> updateLands;
	QVector<D3DXVECTOR3> updateHeights;
	CLandscape* land;
	float x, z, x2, z2, x3, z3, heightMove;
	float len, radius2;
	int i, pointCount, mX, mZ;

	for (x = terrainPos.x - radius1; x <= terrainPos.x + radius1; x += MPU)
	{
		for (z = terrainPos.z - radius1; z <= terrainPos.z + radius1; z += MPU)
		{
			if (VecInWorld(x, z))
			{
				if (rounded)
				{
					len = sqrt((float)((x - terrainPos.x) * (x - terrainPos.x) + (z - terrainPos.z) * (z - terrainPos.z)));
					if ((len - 1.0f) > radius1)
						continue;
					radius2 = radius1;
				}
				else
				{
					const float absX = abs(x - terrainPos.x);
					const float absZ = abs(z - terrainPos.z);
					if (absX == absZ)
						len = absX + (float)MPU / 2.0f;
					else if (absX > absZ)
						len = absX;
					else
						len = absZ;
					radius2 = radius1 + (float)MPU / 2.0f;
				}

				x2 = x / MPU;
				z2 = z / MPU;

				mX = int(x2 / MAP_SIZE);
				mZ = int(z2 / MAP_SIZE);
				if (x2 == m_width * MAP_SIZE)
					mX--;
				if (z2 == m_height * MAP_SIZE)
					mZ--;

				const int offset = mX + mZ * m_width;
				if (offset < 0 || offset >= m_width * m_height)
					continue;
				land = m_lands[offset];
				if (!land)
					continue;

				const int heightOffset = ((int)x2 - (mX * MAP_SIZE)) + (((int)z2 - (mZ * MAP_SIZE)) * (MAP_SIZE + 1));
				if (heightOffset < 0 || heightOffset >= (MAP_SIZE + 1) * (MAP_SIZE + 1))
					continue;

				if (mode == 4)
				{
					land->m_heightMap[heightOffset] = land->GetHeight(heightOffset) + heightAttribute;
					if (!updateLands.contains(land))
						updateLands.push_back(land);
				}
				else
				{
					heightMove = 0.0f;

					if (mode == 0)
					{
						heightMove = (float)mousePos.y() * 0.2f * cameraFactor;
						if (radius > 1)
							heightMove *= (1.0f - ((len - 1.0f) / radius1) * ((len - 1.0f) / radius2));
					}
					else if (mode == 1)
					{
						if (len <= radius4 * radius2 || radius <= 1 || (radius4 * radius2 - radius2) == 0.0f)
							heightMove = baseHeight2;
						else
						{
							float a = (len - radius2) / (radius4 * radius2 - radius2) * 0.4f;
							if (a < 0.0f)
								a = 0.0f;
							else if (a > 0.5f)
								a = 0.5f;
							heightMove = land->GetHeight(heightOffset) * (1.0f - a) + baseHeight2 * a;
						}
						heightMove -= land->GetHeight(heightOffset);
					}
					else if (mode == 2)
					{
						pointCount = 0;
						for (x3 = x - MPU; x3 <= x + MPU; x3 += MPU)
						{
							for (z3 = z - MPU; z3 <= z + MPU; z3 += MPU)
							{
								if (VecInWorld(x3, z3))
								{
									heightMove += GetHeight(x3, z3);
									pointCount++;
								}
							}
						}

						if (pointCount > 0)
						{
							heightMove /= (float)pointCount;
							heightMove -= land->GetHeight(heightOffset);
						}
					}
					else if (mode == 3)
					{
						heightMove = (float)(rand()) / (float)(RAND_MAX / 0.3f);
						if (heightMove >= 0.15f)
							heightMove = -(heightMove - 0.15f);
					}

					heightMove += land->GetHeight(heightOffset);
					if (heightMove < 0.0f)
						heightMove = 0.0f;
					else if (heightMove > 999.0f)
						heightMove = 999.0f;
					heightMove += land->GetHeightAttribute(heightOffset);

					if (mode == 2)
						updateHeights.push_back(D3DXVECTOR3(x, heightMove, z));
					else
						SetHeight(x, z, heightMove);

					if (land->GetHeightAttribute(heightOffset) >= HGT_NOWALK)
					{
						if (!updateLands.contains(land))
							updateLands.push_back(land);
					}
				}
			}
		}
	}

	if (mode == 2)
	{
		for (i = 0; i < updateHeights.size(); i++)
			SetHeight(updateHeights[i].x, updateHeights[i].z, updateHeights[i].y);
	}

	for (i = 0; i < updateLands.size(); i++)
		updateLands[i]->MakeAttributesVertexBuffer();
}

void CWorld::SetHeight(float x, float z, float height)
{
	if (!VecInWorld(x, z))
		return;

	x /= (float)m_MPU;
	z /= (float)m_MPU;

	int mX = int(x / MAP_SIZE);
	int mZ = int(z / MAP_SIZE);
	if (x == m_width * MAP_SIZE)
		mX--;
	if (z == m_height * MAP_SIZE)
		mZ--;

	const int offset = mX + mZ * m_width;
	if (offset < 0 || offset >= m_width * m_height)
		return;

	CLandscape* land = m_lands[offset];
	if (!land)
		return;

	x -= mX * MAP_SIZE;
	z -= mZ * MAP_SIZE;

	const int rX = (int)x;
	const int rZ = (int)z;

	land->SetHeight(rX, rZ, height);

	if (rX == 0 && mX > 0 && rZ == 0 && mZ > 0)
	{
		land = m_lands[(mX - 1) + (mZ - 1) * m_width];
		if (land)
			land->SetHeight(MAP_SIZE, MAP_SIZE, height);
	}
	if (rX == 0 && mX > 0)
	{
		land = m_lands[(mX - 1) + mZ * m_width];
		if (land)
			land->SetHeight(MAP_SIZE, rZ, height);
	}
	if (rZ == 0 && mZ > 0)
	{
		land = m_lands[mX + (mZ - 1) * m_width];
		if (land)
			land->SetHeight(rX, MAP_SIZE, height);
	}
}

void CLandscape::SetHeight(int x, int z, float height)
{
	m_heightMap[x + z * (MAP_SIZE + 1)] = height;

	const int tx = x / PATCH_SIZE;
	const int tz = z / PATCH_SIZE;

	m_patches[tz][tx].m_dirty = true;
	if (tx > 0)
		m_patches[tz][tx - 1].m_dirty = true;
	if (tz > 0)
		m_patches[tz - 1][tx].m_dirty = true;
	if (tz > 0 && tx > 0)
		m_patches[tz - 1][tx - 1].m_dirty = true;
	m_dirty = true;
}

void CWorld::EditWater(const D3DXVECTOR3& pos)
{
	const int mX = int((pos.x / (float)MPU) / PATCH_SIZE);
	const int mZ = int((pos.z / (float)MPU) / PATCH_SIZE);
	if (mX < 0 || mZ < 0 || mX >= m_width * NUM_PATCHES_PER_SIDE || mZ >= m_height * NUM_PATCHES_PER_SIDE)
		return;

	int mode, size, tx, tz, landX, landZ, offset;
	byte height, texture;

	MainFrame->GetWaterEditInfos(mode, height, texture, size);
	WaterHeight newWaterHeight;
	CLandscape* land;
	QVector<CLandscape*> updateLands;

	for (tx = mX - size + 1; tx < mX + size; tx++)
	{
		for (tz = mZ - size + 1; tz < mZ + size; tz++)
		{
			landX = tx / NUM_PATCHES_PER_SIDE;
			landZ = tz / NUM_PATCHES_PER_SIDE;

			if (landX < 0 || landX >= m_width || landZ < 0 || landZ >= m_height)
				continue;

			land = m_lands[landX + landZ * m_width];
			if (!land)
				return;

			offset = (tx - landX * NUM_PATCHES_PER_SIDE) + (tz - landZ * NUM_PATCHES_PER_SIDE) * NUM_PATCHES_PER_SIDE;
			if (offset < 0 || offset >= NUM_PATCHES_PER_SIDE * NUM_PATCHES_PER_SIDE)
				continue;

			WaterHeight newWaterHeight = land->m_waterHeight[offset];

			if (mode == WTYPE_NONE)
				newWaterHeight.texture = WTYPE_NONE;
			else if (mode == WTYPE_CLOUD)
				newWaterHeight.texture = WTYPE_CLOUD;
			else if (mode == WTYPE_WATER)
			{
				newWaterHeight.height = height;
				newWaterHeight.texture = WTYPE_WATER;
				newWaterHeight.texture |= (texture << 2);
			}

			if (newWaterHeight.height != land->m_waterHeight[offset].height
				|| newWaterHeight.texture != land->m_waterHeight[offset].texture)
			{
				land->m_waterHeight[offset] = newWaterHeight;
				if (!updateLands.contains(land))
					updateLands.push_back(land);
			}
		}
	}

	for (int i = 0; i < updateLands.size(); i++)
		updateLands[i]->MakeWaterVertexBuffer();
}

void CMainFrame::OptimizeWater()
{
	if (!m_world)
		return;

	int x, z, X, Z;
	CLandscape* land;
	for (x = 0; x < m_world->m_width; x++)
	{
		for (z = 0; z < m_world->m_height; z++)
		{
			land = m_world->m_lands[x + z * m_world->m_width];
			if (land)
			{
				for (X = 0; X < NUM_PATCHES_PER_SIDE; X++)
				{
					for (Z = 0; Z < NUM_PATCHES_PER_SIDE; Z++)
					{
						const int minHeight = (int)(land->m_patches[Z][X].GetMinHeight() + 0.5f);
						if ((land->m_waterHeight[X + Z * NUM_PATCHES_PER_SIDE].texture == WTYPE_CLOUD
							&& minHeight > 80) ||
							((land->m_waterHeight[X + Z * NUM_PATCHES_PER_SIDE].texture & (byte)(~MASK_WATERFRAME)) == WTYPE_WATER
							&& minHeight >(int)land->m_waterHeight[X + Z * NUM_PATCHES_PER_SIDE].height))
							land->m_waterHeight[X + Z * NUM_PATCHES_PER_SIDE].texture = WTYPE_NONE;
					}
				}

				land->MakeWaterVertexBuffer();
			}
		}
	}

	UpdateWorldEditor();
}

void CMainFrame::FillAllMapWithWater()
{
	if (!m_world)
		return;

	int mode, size;
	byte height, texture;

	MainFrame->GetWaterEditInfos(mode, height, texture, size);

	WaterHeight newWaterHeight;
	memset(&newWaterHeight, 0, sizeof(WaterHeight));

	if (mode == WTYPE_NONE)
		newWaterHeight.texture = WTYPE_NONE;
	else if (mode == WTYPE_CLOUD)
		newWaterHeight.texture = WTYPE_CLOUD;
	else if (mode == WTYPE_WATER)
	{
		newWaterHeight.height = height;
		newWaterHeight.texture = WTYPE_WATER;
		newWaterHeight.texture |= (texture << 2);
	}

	int x, z, X, Z;
	CLandscape* land;
	for (x = 0; x < m_world->m_width; x++)
	{
		for (z = 0; z < m_world->m_height; z++)
		{
			land = m_world->m_lands[x + z * m_world->m_width];
			if (land)
			{
				for (X = 0; X < NUM_PATCHES_PER_SIDE; X++)
				{
					for (Z = 0; Z < NUM_PATCHES_PER_SIDE; Z++)
					{
						const int minHeight = (int)(land->m_patches[Z][X].GetMinHeight() + 0.5f);
						if ((mode == WTYPE_CLOUD && minHeight <= 80) || (mode == WTYPE_WATER && minHeight <= (int)height))
							land->m_waterHeight[X + Z * NUM_PATCHES_PER_SIDE] = newWaterHeight;
						else
							land->m_waterHeight[X + Z * NUM_PATCHES_PER_SIDE].texture = WTYPE_NONE;
					}
				}

				land->MakeWaterVertexBuffer();
			}
		}
	}

	UpdateWorldEditor();
}

void CWorld::EditColor(const D3DXVECTOR3& pos)
{
	int radius, hardness;
	QColor color;
	MainFrame->GetTextureColorEditInfos(radius, hardness, color);

	const float unity = (float)(MAP_SIZE * MPU) / (float)((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE);
	const float posX = (int)((pos.x + (unity / 2.0f)) / unity) * unity;
	const float posZ = (int)((pos.z + (unity / 2.0f)) / unity) * unity;
	const float radius1 = (float)((radius - 1) * unity);
	const float hardRadius = 1.0f - ((float)hardness / 100.0f);

	QVector<CLandscape*> updateLands;
	CLandscape* land;
	float x, z, x2, z2, colorFactor;
	int textureOffset, i, mX, mZ;
	QColor finalColor, oldColor;
	for (x = posX - radius1; x - 0.1f < posX + radius1; x += unity)
	{
		for (z = posZ - radius1; z - 0.1f < posZ + radius1; z += unity)
		{
			if (VecInWorld(x + 0.1f, z + 0.1f))
			{
				const float len = sqrt((float)((x - posX) * (x - posX) + (z - posZ) * (z - posZ)));
				if (len - 0.1f > radius1)
					continue;

				x2 = x / MPU;
				z2 = z / MPU;

				mX = (int)(x2 / MAP_SIZE);
				mZ = (int)(z2 / MAP_SIZE);
				if (x2 == m_width * MAP_SIZE)
					mX--;
				if (z2 == m_height * MAP_SIZE)
					mZ--;

				const int offset = mX + mZ * m_width;
				if (offset < 0 || offset >= m_width * m_height)
					continue;
				land = m_lands[offset];
				if (!land)
					continue;

				x2 -= land->m_posX;
				z2 -= land->m_posY;
				x2 *= MPU;
				z2 *= MPU;

				const int textureOffsetX = (int)(x2 / unity + 0.5f);
				const int textureOffsetZ = (int)(z2 / unity + 0.5f);

				textureOffset = textureOffsetX + textureOffsetZ * MAP_SIZE;
				if (textureOffset < 0 || textureOffset >= MAP_SIZE * MAP_SIZE)
					continue;

				if (radius <= 1)
					colorFactor = color.alphaF();
				else
					colorFactor = (1.0f - (len / radius1) * 2.0f * hardRadius) * color.alphaF();

				if (colorFactor < 0.0f)
					colorFactor = 0.0f;
				else if (colorFactor > 1.0f)
					colorFactor = 1.0f;
				oldColor = QColor(land->m_colorMap[textureOffset * 3],
					land->m_colorMap[textureOffset * 3 + 1],
					land->m_colorMap[textureOffset * 3 + 2]);
				finalColor.setRgbF(oldColor.redF() * (1.0f - colorFactor) + color.redF() * colorFactor,
					oldColor.greenF() * (1.0f - colorFactor) + color.greenF() * colorFactor,
					oldColor.blueF() * (1.0f - colorFactor) + color.blueF() * colorFactor);

				land->m_colorMap[textureOffset * 3] = finalColor.red();
				land->m_colorMap[textureOffset * 3 + 1] = finalColor.green();
				land->m_colorMap[textureOffset * 3 + 2] = finalColor.blue();
				if (!updateLands.contains(land))
					updateLands.push_back(land);

				if (textureOffsetX <= 0 && mX > 0 && textureOffsetZ <= 0 && mZ > 0)
				{
					land = m_lands[(mX - 1) + (mZ - 1) * m_width];
					if (land)
					{
						textureOffset = ((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE) + ((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE) * MAP_SIZE;
						land->m_colorMap[textureOffset * 3] = finalColor.red();
						land->m_colorMap[textureOffset * 3 + 1] = finalColor.green();
						land->m_colorMap[textureOffset * 3 + 2] = finalColor.blue();
						if (!updateLands.contains(land))
							updateLands.push_back(land);
					}
				}
				if (textureOffsetX >= (PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE && mX < m_width - 1 && textureOffsetZ <= 0 && mZ > 0)
				{
					land = m_lands[(mX + 1) + (mZ - 1) * m_width];
					if (land)
					{
						textureOffset = ((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE) * MAP_SIZE;
						land->m_colorMap[textureOffset * 3] = finalColor.red();
						land->m_colorMap[textureOffset * 3 + 1] = finalColor.green();
						land->m_colorMap[textureOffset * 3 + 2] = finalColor.blue();
						if (!updateLands.contains(land))
							updateLands.push_back(land);
					}
				}
				if (textureOffsetX <= 0 && mX > 0 && textureOffsetZ >= (PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE && mZ < m_height - 1)
				{
					land = m_lands[(mX - 1) + (mZ + 1) * m_width];
					if (land)
					{
						textureOffset = ((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE);
						land->m_colorMap[textureOffset * 3] = finalColor.red();
						land->m_colorMap[textureOffset * 3 + 1] = finalColor.green();
						land->m_colorMap[textureOffset * 3 + 2] = finalColor.blue();
						if (!updateLands.contains(land))
							updateLands.push_back(land);
					}
				}
				if (textureOffsetX >= (PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE && mX < m_width - 1 && textureOffsetZ >= (PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE && mZ < m_height - 1)
				{
					land = m_lands[(mX + 1) + (mZ + 1) * m_width];
					if (land)
					{
						textureOffset = 0;
						land->m_colorMap[textureOffset * 3] = finalColor.red();
						land->m_colorMap[textureOffset * 3 + 1] = finalColor.green();
						land->m_colorMap[textureOffset * 3 + 2] = finalColor.blue();
						if (!updateLands.contains(land))
							updateLands.push_back(land);
					}
				}
				if (textureOffsetX <= 0 && mX > 0)
				{
					land = m_lands[(mX - 1) + mZ * m_width];
					if (land)
					{
						textureOffset = ((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE) + textureOffsetZ * MAP_SIZE;
						land->m_colorMap[textureOffset * 3] = finalColor.red();
						land->m_colorMap[textureOffset * 3 + 1] = finalColor.green();
						land->m_colorMap[textureOffset * 3 + 2] = finalColor.blue();
						if (!updateLands.contains(land))
							updateLands.push_back(land);
					}
				}
				if (textureOffsetX >= (PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE && mX < m_width - 1)
				{
					land = m_lands[(mX + 1) + mZ * m_width];
					if (land)
					{
						textureOffset = textureOffsetZ * MAP_SIZE;
						land->m_colorMap[textureOffset * 3] = finalColor.red();
						land->m_colorMap[textureOffset * 3 + 1] = finalColor.green();
						land->m_colorMap[textureOffset * 3 + 2] = finalColor.blue();
						if (!updateLands.contains(land))
							updateLands.push_back(land);
					}
				}
				if (textureOffsetZ <= 0 && mZ > 0)
				{
					land = m_lands[mX + (mZ - 1) * m_width];
					if (land)
					{
						textureOffset = textureOffsetX + ((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE) * MAP_SIZE;
						land->m_colorMap[textureOffset * 3] = finalColor.red();
						land->m_colorMap[textureOffset * 3 + 1] = finalColor.green();
						land->m_colorMap[textureOffset * 3 + 2] = finalColor.blue();
						if (!updateLands.contains(land))
							updateLands.push_back(land);
					}
				}
				if (textureOffsetZ >= (PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE && mZ < m_height - 1)
				{
					land = m_lands[mX + (mZ + 1) * m_width];
					if (land)
					{
						textureOffset = textureOffsetX;
						land->m_colorMap[textureOffset * 3] = finalColor.red();
						land->m_colorMap[textureOffset * 3 + 1] = finalColor.green();
						land->m_colorMap[textureOffset * 3 + 2] = finalColor.blue();
						if (!updateLands.contains(land))
							updateLands.push_back(land);
					}
				}
			}
		}
	}

	for (i = 0; i < updateLands.size(); i++)
		updateLands[i]->UpdateTextureLayers();
}

void CWorld::EditTexture(const D3DXVECTOR3& pos)
{
	int textureID, radius, hardness, alpha;
	bool emptyMode;
	MainFrame->GetTextureEditInfos(textureID, radius, hardness, alpha, emptyMode);

	if (!emptyMode && textureID == -1)
		return;

	const float unity = (float)(MAP_SIZE * MPU) / (float)((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE);
	const float posX = (int)((pos.x + (unity / 2.0f)) / unity) * unity;
	const float posZ = (int)((pos.z + (unity / 2.0f)) / unity) * unity;
	const float radius1 = (float)((radius - 1) * unity);
	const float hardRadius = emptyMode ? 0.0f : 1.0f - ((float)hardness / 100.0f);

	bool updateLandInfos = false;
	CLandscape* currentLandInfos = MainFrame->GetCurrentInfoLand();
	QVector<CLandscape*> updateLands;
	CLandscape* land;
	LandLayer* layer;
	float x, z, x2, z2;
	float alphaFactor, finalAlpha;
	int i, mX, mZ, j, k, offX1, offX2, offZ1, offZ2;
	for (x = posX - radius1; x - 0.1f < posX + radius1; x += unity)
	{
		for (z = posZ - radius1; z - 0.1f < posZ + radius1; z += unity)
		{
			if (VecInWorld(x + 0.1f, z + 0.1f))
			{
				const float len = sqrt((float)((x - posX) * (x - posX) + (z - posZ) * (z - posZ)));
				if (len - 0.1f > radius1)
					continue;

				x2 = x / MPU;
				z2 = z / MPU;

				mX = (int)(x2 / MAP_SIZE);
				mZ = (int)(z2 / MAP_SIZE);
				if (x2 == m_width * MAP_SIZE)
					mX--;
				if (z2 == m_height * MAP_SIZE)
					mZ--;

				const int offset = mX + mZ * m_width;
				if (offset < 0 || offset >= m_width * m_height)
					continue;
				land = m_lands[offset];
				if (!land)
					continue;

				x2 -= land->m_posX;
				z2 -= land->m_posY;
				x2 *= MPU;
				z2 *= MPU;

				const int textureOffsetX = (int)(x2 / unity + 0.5f);
				const int textureOffsetZ = (int)(z2 / unity + 0.5f);

				if (!updateLands.contains(land))
					updateLands.push_back(land);

				if (emptyMode)
				{
					const int patchOffsetX = (textureOffsetX == ((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE)) ? (NUM_PATCHES_PER_SIDE - 1) : textureOffsetX / (PATCH_SIZE - 1);
					const int patchOffsetZ = (textureOffsetZ == ((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE)) ? (NUM_PATCHES_PER_SIDE - 1) : textureOffsetZ / (PATCH_SIZE - 1);
					const int patchOffset = patchOffsetX + patchOffsetZ * NUM_PATCHES_PER_SIDE;
					if (patchOffset < 0 || patchOffset >= NUM_PATCHES_PER_SIDE * NUM_PATCHES_PER_SIDE)
						continue;

					for (i = 0; i < land->m_layers.GetSize(); i++)
					{
						if (i > 0)
						{
							offX1 = patchOffsetX * (PATCH_SIZE - 1) + 1;
							if (patchOffsetX > 0 && !land->m_layers[i]->patchEnable[(patchOffsetX - 1) + patchOffsetZ * NUM_PATCHES_PER_SIDE])
								offX1--;
							offX2 = (patchOffsetX + 1) * (PATCH_SIZE - 1);
							if (patchOffsetX < (NUM_PATCHES_PER_SIDE - 1) && !land->m_layers[i]->patchEnable[(patchOffsetX + 1) + patchOffsetZ * NUM_PATCHES_PER_SIDE])
								offX2++;
							offZ1 = patchOffsetZ * (PATCH_SIZE - 1) + 1;
							if (patchOffsetZ > 0 && !land->m_layers[i]->patchEnable[patchOffsetX + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE])
								offZ1--;
							offZ2 = (patchOffsetZ + 1) * (PATCH_SIZE - 1);
							if (patchOffsetZ < (NUM_PATCHES_PER_SIDE - 1) && !land->m_layers[i]->patchEnable[patchOffsetX + (patchOffsetZ + 1) * NUM_PATCHES_PER_SIDE])
								offZ2++;

							for (j = offX1; j < offX2; j++)
								for (k = offZ1; k < offZ2; k++)
									land->m_layers[i]->alphaMap[j + k * MAP_SIZE] = 0;
						}

						land->m_layers[i]->patchEnable[patchOffset] = false;
					}
				}
				else
				{
					if (radius <= 1)
						alphaFactor = 1.0f;
					else
						alphaFactor = (1.0f - (len / radius1) * hardRadius) + 0.2f;

					if (alphaFactor < 0.0f)
						alphaFactor = 0.0f;
					else if (alphaFactor > 1.0f)
						alphaFactor = 1.0f;
					finalAlpha = ((float)alpha / 255.0f) * alphaFactor;
					if (finalAlpha < 0.0f)
						finalAlpha = 0.0f;
					else if (finalAlpha > 1.0f)
						finalAlpha = 1.0f;

					const byte realAlpha = (byte)(finalAlpha * 255.0f);

					if (land->SetLayerAlpha(textureOffsetX,
						textureOffsetZ,
						textureID, realAlpha) && land == currentLandInfos)
						updateLandInfos = true;

					if (textureOffsetX <= 0 && mX > 0 && textureOffsetZ <= 0 && mZ > 0)
					{
						land = m_lands[(mX - 1) + (mZ - 1) * m_width];
						if (land)
						{
							if (land->SetLayerAlpha(((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE),
								((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE),
								textureID, realAlpha) && land == currentLandInfos)
								updateLandInfos = true;
							if (!updateLands.contains(land))
								updateLands.push_back(land);
						}
					}
					if (textureOffsetX >= (PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE && mX < m_width - 1 && textureOffsetZ <= 0 && mZ > 0)
					{
						land = m_lands[(mX + 1) + (mZ - 1) * m_width];
						if (land)
						{
							if (land->SetLayerAlpha(0,
								((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE),
								textureID, realAlpha) && land == currentLandInfos)
								updateLandInfos = true;
							if (!updateLands.contains(land))
								updateLands.push_back(land);
						}
					}
					if (textureOffsetX <= 0 && mX > 0 && textureOffsetZ >= (PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE && mZ < m_height - 1)
					{
						land = m_lands[(mX - 1) + (mZ + 1) * m_width];
						if (land)
						{
							if (land->SetLayerAlpha(((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE),
								0,
								textureID, realAlpha) && land == currentLandInfos)
								updateLandInfos = true;
							if (!updateLands.contains(land))
								updateLands.push_back(land);
						}
					}
					if (textureOffsetX >= (PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE && mX < m_width - 1 && textureOffsetZ >= (PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE && mZ < m_height - 1)
					{
						land = m_lands[(mX + 1) + (mZ + 1) * m_width];
						if (land)
						{
							if (land->SetLayerAlpha(0,
								0,
								textureID, realAlpha) && land == currentLandInfos)
								updateLandInfos = true;
							if (!updateLands.contains(land))
								updateLands.push_back(land);
						}
					}
					if (textureOffsetX <= 0 && mX > 0)
					{
						land = m_lands[(mX - 1) + mZ * m_width];
						if (land)
						{
							if (land->SetLayerAlpha(((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE),
								textureOffsetZ,
								textureID, realAlpha) && land == currentLandInfos)
								updateLandInfos = true;
							if (!updateLands.contains(land))
								updateLands.push_back(land);
						}
					}
					if (textureOffsetX >= (PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE && mX < m_width - 1)
					{
						land = m_lands[(mX + 1) + mZ * m_width];
						if (land)
						{
							if (land->SetLayerAlpha(0,
								textureOffsetZ,
								textureID, realAlpha) && land == currentLandInfos)
								updateLandInfos = true;
							if (!updateLands.contains(land))
								updateLands.push_back(land);
						}
					}
					if (textureOffsetZ <= 0 && mZ > 0)
					{
						land = m_lands[mX + (mZ - 1) * m_width];
						if (land)
						{
							if (land->SetLayerAlpha(textureOffsetX,
								((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE),
								textureID, realAlpha) && land == currentLandInfos)
								updateLandInfos = true;
							if (!updateLands.contains(land))
								updateLands.push_back(land);
						}
					}
					if (textureOffsetZ >= (PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE && mZ < m_height - 1)
					{
						land = m_lands[mX + (mZ + 1) * m_width];
						if (land)
						{
							if (land->SetLayerAlpha(textureOffsetX,
								0,
								textureID, realAlpha) && land == currentLandInfos)
								updateLandInfos = true;
							if (!updateLands.contains(land))
								updateLands.push_back(land);
						}
					}
				}
			}
		}
	}

	bool deleteLayer;
	for (i = 0; i < updateLands.size(); i++)
	{
		for (j = 0; j < updateLands[i]->m_layers.GetSize(); j++)
		{
			layer = updateLands[i]->m_layers[j];
			deleteLayer = true;
			for (k = 0; k < NUM_PATCHES_PER_SIDE * NUM_PATCHES_PER_SIDE; k++)
			{
				if (layer->patchEnable[k])
				{
					deleteLayer = false;
					break;
				}
			}
			if (deleteLayer)
			{
				Release(layer->lightMap);
				Delete(layer);
				updateLands[i]->m_layers.RemoveAt(j);
				j--;
				if (updateLands[i] == currentLandInfos)
					updateLandInfos = true;
			}
		}
		updateLands[i]->UpdateTextureLayers();
	}

	if (updateLandInfos)
	{
		MainFrame->SetLayerInfos(null);
		MainFrame->SetLayerInfos(currentLandInfos);
	}
}

bool CLandscape::SetLayerAlpha(int x, int z, int textureID, byte alpha)
{
	const byte MinAlpha = 30;

	bool updateLandInfos = false;
	const int oldLayerCount = m_layers.GetSize();
	LandLayer* layer = GetLayer(textureID);
	if (oldLayerCount != m_layers.GetSize())
		updateLandInfos = true;

	const int textureOffset = x + z * MAP_SIZE;
	if (textureOffset < 0 || textureOffset >= MAP_SIZE * MAP_SIZE)
		return updateLandInfos;

	const int patchOffsetX = (x == ((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE)) ? (NUM_PATCHES_PER_SIDE - 1) : x / (PATCH_SIZE - 1);
	const int patchOffsetZ = (z == ((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE)) ? (NUM_PATCHES_PER_SIDE - 1) : z / (PATCH_SIZE - 1);
	const int patchOffset = patchOffsetX + patchOffsetZ * NUM_PATCHES_PER_SIDE;
	if (patchOffset < 0 || patchOffset >= NUM_PATCHES_PER_SIDE * NUM_PATCHES_PER_SIDE)
		return updateLandInfos;

	layer->patchEnable[patchOffset] = true;

	if (x % (PATCH_SIZE - 1) == 0 && patchOffsetX > 0)
		layer->patchEnable[(patchOffsetX - 1) + patchOffsetZ * NUM_PATCHES_PER_SIDE] = true;
	if (z % (PATCH_SIZE - 1) == 0 && patchOffsetZ > 0)
		layer->patchEnable[patchOffsetX + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE] = true;
	if (z % (PATCH_SIZE - 1) == 0 && patchOffsetZ > 0 && x % (PATCH_SIZE - 1) == 0 && patchOffsetX > 0)
		layer->patchEnable[(patchOffsetX - 1) + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE] = true;

	if (layer->alphaMap[textureOffset] < alpha)
		layer->alphaMap[textureOffset] = alpha;

	int i, j, k, totalAlpha = alpha;
	LandLayer* tempLayer;
	for (i = m_layers.GetSize() - 1; i >= 0; i--)
	{
		if (m_layers[i] == layer)
			break;
		tempLayer = m_layers[i];

		if (tempLayer->alphaMap[textureOffset] >= MinAlpha)
		{
			if (((int)tempLayer->alphaMap[textureOffset] + totalAlpha) >= 255)
			{
				if (totalAlpha < 255)
				{
					if ((byte)(255 - totalAlpha) < tempLayer->alphaMap[textureOffset])
						tempLayer->alphaMap[textureOffset] = (byte)(255 - totalAlpha);
					totalAlpha += tempLayer->alphaMap[textureOffset];
				}
				else
					tempLayer->alphaMap[textureOffset] = 0;
			}
		}

		tempLayer->patchEnable[patchOffsetX + patchOffsetZ * NUM_PATCHES_PER_SIDE] = false;
		for (j = patchOffsetX * (PATCH_SIZE - 1); j <= (patchOffsetX + 1) * (PATCH_SIZE - 1); j++)
		{
			for (k = patchOffsetZ * (PATCH_SIZE - 1); k <= (patchOffsetZ + 1) * (PATCH_SIZE - 1); k++)
			{
				if (tempLayer->alphaMap[j + k * MAP_SIZE] >= MinAlpha)
				{
					tempLayer->patchEnable[patchOffsetX + patchOffsetZ * NUM_PATCHES_PER_SIDE] = true;
					break;
				}
			}
			if (tempLayer->patchEnable[patchOffsetX + patchOffsetZ * NUM_PATCHES_PER_SIDE])
				break;
		}
		if (x % (PATCH_SIZE - 1) == 0 && patchOffsetX > 0)
		{
			tempLayer->patchEnable[(patchOffsetX - 1) + patchOffsetZ * NUM_PATCHES_PER_SIDE] = false;
			for (j = (patchOffsetX - 1) * (PATCH_SIZE - 1); j <= patchOffsetX * (PATCH_SIZE - 1); j++)
			{
				for (k = patchOffsetZ * (PATCH_SIZE - 1); k <= (patchOffsetZ + 1) * (PATCH_SIZE - 1); k++)
				{
					if (tempLayer->alphaMap[j + k * MAP_SIZE] >= MinAlpha)
					{
						tempLayer->patchEnable[(patchOffsetX - 1) + patchOffsetZ * NUM_PATCHES_PER_SIDE] = true;
						break;
					}
				}
				if (tempLayer->patchEnable[(patchOffsetX - 1) + patchOffsetZ * NUM_PATCHES_PER_SIDE])
					break;
			}
		}
		if (z % (PATCH_SIZE - 1) == 0 && patchOffsetZ > 0)
		{
			tempLayer->patchEnable[patchOffsetX + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE] = false;
			for (j = patchOffsetX * (PATCH_SIZE - 1); j <= (patchOffsetX + 1) * (PATCH_SIZE - 1); j++)
			{
				for (k = (patchOffsetZ - 1) * (PATCH_SIZE - 1); k <= patchOffsetZ * (PATCH_SIZE - 1); k++)
				{
					if (tempLayer->alphaMap[j + k * MAP_SIZE] >= MinAlpha)
					{
						tempLayer->patchEnable[patchOffsetX + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE] = true;
						break;
					}
				}
				if (tempLayer->patchEnable[patchOffsetX + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE])
					break;
			}
		}
		if (z % (PATCH_SIZE - 1) == 0 && patchOffsetZ > 0 && x % (PATCH_SIZE - 1) == 0 && patchOffsetX > 0)
		{
			tempLayer->patchEnable[(patchOffsetX - 1) + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE] = false;
			for (j = (patchOffsetX - 1) * (PATCH_SIZE - 1); j <= patchOffsetX * (PATCH_SIZE - 1); j++)
			{
				for (k = (patchOffsetZ - 1) * (PATCH_SIZE - 1); k <= patchOffsetZ * (PATCH_SIZE - 1); k++)
				{
					if (tempLayer->alphaMap[j + k * MAP_SIZE] >= MinAlpha)
					{
						tempLayer->patchEnable[(patchOffsetX - 1) + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE] = true;
						break;
					}
				}
				if (tempLayer->patchEnable[(patchOffsetX - 1) + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE])
					break;
			}
		}
	}

	if (totalAlpha >= (255 - MinAlpha) && layer != m_layers[0])
	{
		for (i = 1; i < m_layers.GetSize(); i++)
		{
			if (m_layers[i] == layer)
				break;
			tempLayer = m_layers[i];
			tempLayer->alphaMap[textureOffset] = 0;
			tempLayer->patchEnable[patchOffsetX + patchOffsetZ * NUM_PATCHES_PER_SIDE] = false;
			for (j = patchOffsetX * (PATCH_SIZE - 1); j <= (patchOffsetX + 1) * (PATCH_SIZE - 1); j++)
			{
				for (k = patchOffsetZ * (PATCH_SIZE - 1); k <= (patchOffsetZ + 1) * (PATCH_SIZE - 1); k++)
				{
					if (tempLayer->alphaMap[j + k * MAP_SIZE] >= MinAlpha)
					{
						tempLayer->patchEnable[patchOffsetX + patchOffsetZ * NUM_PATCHES_PER_SIDE] = true;
						break;
					}
				}
				if (tempLayer->patchEnable[patchOffsetX + patchOffsetZ * NUM_PATCHES_PER_SIDE])
					break;
			}
			if (x % (PATCH_SIZE - 1) == 0 && patchOffsetX > 0)
			{
				tempLayer->patchEnable[(patchOffsetX - 1) + patchOffsetZ * NUM_PATCHES_PER_SIDE] = false;
				for (j = (patchOffsetX - 1) * (PATCH_SIZE - 1); j <= patchOffsetX * (PATCH_SIZE - 1); j++)
				{
					for (k = patchOffsetZ * (PATCH_SIZE - 1); k <= (patchOffsetZ + 1) * (PATCH_SIZE - 1); k++)
					{
						if (tempLayer->alphaMap[j + k * MAP_SIZE] >= MinAlpha)
						{
							tempLayer->patchEnable[(patchOffsetX - 1) + patchOffsetZ * NUM_PATCHES_PER_SIDE] = true;
							break;
						}
					}
					if (tempLayer->patchEnable[(patchOffsetX - 1) + patchOffsetZ * NUM_PATCHES_PER_SIDE])
						break;
				}
			}
			if (z % (PATCH_SIZE - 1) == 0 && patchOffsetZ > 0)
			{
				tempLayer->patchEnable[patchOffsetX + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE] = false;
				for (j = patchOffsetX * (PATCH_SIZE - 1); j <= (patchOffsetX + 1) * (PATCH_SIZE - 1); j++)
				{
					for (k = (patchOffsetZ - 1) * (PATCH_SIZE - 1); k <= patchOffsetZ * (PATCH_SIZE - 1); k++)
					{
						if (tempLayer->alphaMap[j + k * MAP_SIZE] >= MinAlpha)
						{
							tempLayer->patchEnable[patchOffsetX + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE] = true;
							break;
						}
					}
					if (tempLayer->patchEnable[patchOffsetX + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE])
						break;
				}
			}
			if (z % (PATCH_SIZE - 1) == 0 && patchOffsetZ > 0 && x % (PATCH_SIZE - 1) == 0 && patchOffsetX > 0)
			{
				tempLayer->patchEnable[(patchOffsetX - 1) + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE] = false;
				for (j = (patchOffsetX - 1) * (PATCH_SIZE - 1); j <= patchOffsetX * (PATCH_SIZE - 1); j++)
				{
					for (k = (patchOffsetZ - 1) * (PATCH_SIZE - 1); k <= patchOffsetZ * (PATCH_SIZE - 1); k++)
					{
						if (tempLayer->alphaMap[j + k * MAP_SIZE] >= MinAlpha)
						{
							tempLayer->patchEnable[(patchOffsetX - 1) + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE] = true;
							break;
						}
					}
					if (tempLayer->patchEnable[(patchOffsetX - 1) + (patchOffsetZ - 1) * NUM_PATCHES_PER_SIDE])
						break;
				}
			}
		}
	}

	return updateLandInfos;
}