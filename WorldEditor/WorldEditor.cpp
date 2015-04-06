///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#include "stdafx.h"
#include "WorldEditor.h"
#include <Object3D.h>
#include <SfxModel.h>
#include <ModelMng.h>
#include <TextureMng.h>
#include <World.h>
#include <Font.h>
#include <Skybox.h>
#include "MainFrame.h"
#include <Landscape.h>
#include <Project.h>
#include <Render2D.h>
#include <Mover.h>
#include <GameElements.h>
#include <Region.h>
#include "ObjectProperties.h"
#include <Path.h>

struct BrushVertex
{
	enum { FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE };
	D3DXVECTOR3 p;
	DWORD c;
};

CWorldEditor::CWorldEditor(QWidget* parent, Qt::WindowFlags flags)
	: CD3DWidget(parent, flags)
{
	m_textureMng = null;
	m_modelMng = null;
	m_world = null;
	m_render2D = null;
	m_pickObject = null;
	m_addObject = null;
	m_editSpawn = -1;
	setMinimumSize(QSize(MINIMAP_SIZE, MINIMAP_SIZE));
}

CWorldEditor::~CWorldEditor()
{
	Destroy();
}

bool CWorldEditor::InitDeviceObjects()
{
	m_textureMng = new CTextureMng(m_device);
	m_modelMng = new CModelMng(m_device);
	m_render2D = new CRender2D(m_device);

	if (!CObject3D::InitStaticDeviceObjects(m_device)
		|| !CSfxModel::InitStaticDeviceObjects(m_device)
		|| !CWorld::InitStaticDeviceObjects(m_device)
		|| !CSkybox::InitStaticDeviceObjects(m_device))
		return false;

	CWorld::s_objNameFont = new CFont(m_device);
	if (!CWorld::s_objNameFont->Create("Verdana", 9, CFont::Normal, D3DCOLOR_ARGB(255, 255, 255, 255), 2, D3DCOLOR_ARGB(255, 60, 60, 60)))
		return false;

	return true;
}

void CWorldEditor::DeleteDeviceObjects()
{
	Delete(CWorld::s_objNameFont);
	CObject3D::DeleteStaticDeviceObjects();
	CSfxModel::DeleteStaticDeviceObjects();
	CWorld::DeleteStaticDeviceObjects();
	CSkybox::DeleteStaticDeviceObjects();
	Delete(m_render2D);
	Delete(m_modelMng);
	Delete(m_textureMng);
}

bool CWorldEditor::RestoreDeviceObjects()
{
	if (!CFont::RestoreStaticDeviceObjects(m_device)
		|| !CWorld::RestoreStaticDeviceObjects(m_device))
		return false;
	return true;
}

void CWorldEditor::InvalidateDeviceObjects()
{
	CFont::InvalidateStaticDeviceObjects();
	CWorld::InvalidateStaticDeviceObjects();
}

bool CWorldEditor::Render()
{
	m_device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, m_world ? m_world->m_fogColor : g_global3D.backgroundColor, 1.0f, 0L);

	if (FAILED(m_device->BeginScene()))
		return false;

	D3DVIEWPORT9 viewport;
	viewport.X = 0;
	viewport.Y = 0;
	viewport.Height = g_global3D.renderMinimap ? MINIMAP_SIZE : height();
	viewport.Width = g_global3D.renderMinimap ? MINIMAP_SIZE : width();
	viewport.MinZ = 0.0f;
	viewport.MaxZ = 1.0f;
	g_global3D.viewport = viewport;
	m_device->SetViewport(&viewport);

	if (m_world)
	{
		if (!g_global3D.renderMinimap)
			_updateAddObjects();

		m_world->Render();

		if (!g_global3D.renderMinimap)
		{
			const EEditMode editMode = MainFrame->GetEditMode();
			if (m_pickingTerrain && (editMode == EDIT_TERRAIN_HEIGHT
				|| editMode == EDIT_TERRAIN_TEXTURE
				|| editMode == EDIT_TERRAIN_COLOR))
			{
				static BrushVertex vertices[1024];
				int vertexCount = 0;

				if (editMode == EDIT_TERRAIN_HEIGHT)
				{
					int mode, radius, hardness, attribute;
					bool rounded, useFixedHeight;
					float fixedHeight;
					MainFrame->GetTerrainHeightEditInfos(mode, rounded, radius, hardness, useFixedHeight, fixedHeight, attribute);

					const D3DXVECTOR3 pos = ((m_editing && MainFrame->GetTerrainHeightEditMode() == 0) ? m_editTerrainPos : m_terrainPos);

					const float posX = (int)(pos.x + ((float)MPU / 2.0f)) / MPU * MPU;
					const float posZ = (int)(pos.z + ((float)MPU / 2.0f)) / MPU * MPU;

					const float radius1 = (float)((radius - 1) * MPU);
					const float radius3 = (((float)(radius - 2) + 0.25f) * MPU);
					const float radius4 = (float)hardness / 100.0f + 0.1f;

					DWORD color;
					float x, z, len, radius2;
					for (x = posX - radius1; x <= posX + radius1; x += MPU)
					{
						for (z = posZ - radius1; z <= posZ + radius1; z += MPU)
						{
							if (m_world->VecInWorld(x, z))
							{
								if (rounded)
								{
									len = sqrt((float)((x - posX) * (x - posX) + (z - posZ) * (z - posZ)));
									if ((len - 1.0f) > radius1)
										continue;
									radius2 = radius1;
								}
								else
								{
									const float absX = abs(x - posX);
									const float absZ = abs(z - posZ);
									if (absX == absZ)
										len = absX + (float)MPU / 2.0f;
									else if (absX > absZ)
										len = absX;
									else
										len = absZ;
									radius2 = radius1 + (float)MPU / 2.0f;
								}

								if (radius > 19 && len < radius3)
									continue;

								if (mode == 1)
								{
									if (len <= radius4 * radius2 || radius <= 1)
										color = 0xffffffff;
									else
										color = 0xffffff00;
								}
								else
									color = 0xffffffff;

								if (mode == 4)
								{
									vertices[vertexCount].p = D3DXVECTOR3(x + ((float)MPU) / 2.0f,
										m_world->GetHeight(x + ((float)MPU) / 2.0f, z + ((float)MPU) / 2.0f),
										z + ((float)MPU) / 2.0f);
								}
								else
									vertices[vertexCount].p = D3DXVECTOR3(x, m_world->GetHeight(x, z), z);
								vertices[vertexCount].c = color;
								vertexCount++;
							}
						}
					}
				}
				else
				{
					const int radius = MainFrame->GetTextureEditRadius();
					const float unity = (float)(MAP_SIZE * MPU) / (float)((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE);
					const float posX = (int)((m_terrainPos.x + (unity / 2.0f)) / unity) * unity;
					const float posZ = (int)((m_terrainPos.z + (unity / 2.0f)) / unity) * unity;
					const float radius1 = (float)((radius - 1) * unity);
					const float radius3 = (float)((radius - 2) * unity);

					float x, z;
					for (x = posX - radius1; x - 0.1f < posX + radius1; x += unity)
					{
						for (z = posZ - radius1; z - 0.1f < posZ + radius1; z += unity)
						{
							if (m_world->VecInWorld(x, z))
							{
								const float len = sqrt((float)((x - posX) * (x - posX) + (z - posZ) * (z - posZ)));
								if (len - 0.1f > radius1 || len + 0.1f < radius3)
									continue;

								vertices[vertexCount].p = D3DXVECTOR3(x, m_world->GetHeight(x, z), z);
								vertices[vertexCount].c = 0xffffffff;
								vertexCount++;
							}
						}
					}
				}

				if (vertexCount > 0)
				{
					m_device->SetFVF(BrushVertex::FVF);
					const float pointSize = 2.0f;
					m_device->SetRenderState(D3DRS_POINTSIZE, *((DWORD*)&pointSize));
					m_device->DrawPrimitiveUP(D3DPT_POINTLIST, vertexCount, vertices, sizeof(BrushVertex));
				}
			}

			if (editMode == EDIT_TERRAIN_TEXTURE
				&& MainFrame->GetEditTexture() != -1)
			{
				const QRect rect(width() - 95, 10, 85, 85);
				m_render2D->RenderRoundRect(rect.adjusted(-1, -1, 2, 2), 0xff000000);

				m_device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
				m_render2D->RenderTexture(rect, Project->GetTerrain(MainFrame->GetEditTexture()), 255, false);
			}

			if (m_selectRect && m_selectRectPos != m_lastMousePos)
			{
				m_render2D->RenderRect(QRect(m_selectRectPos, m_lastMousePos).normalized(), 0xffffff00);
			}
		}
	}

	m_device->EndScene();
	return true;
}

void CWorldEditor::mouseMoveEvent(QMouseEvent * event)
{
	const QPoint mousePos(event->x(), event->y());

	if (!m_world || g_global3D.renderMinimap)
		return;

	const EEditMode editMode = MainFrame->GetEditMode();
	const QPoint mouseMove = m_lastMousePos - mousePos;

	if (m_rotatingCamera)
	{
		m_world->m_cameraAngle.x -= (float)mouseMove.x() / 4.0f;
		m_world->m_cameraAngle.y += (float)mouseMove.y() / 4.0f;

		if (m_world->m_cameraAngle.y > 89.9f)
			m_world->m_cameraAngle.y = 89.9f;
		else if (m_world->m_cameraAngle.y < -89.9f)
			m_world->m_cameraAngle.y = -89.9f;

		event->accept();
	}

	if (m_movingCamera)
	{
		m_world->m_cameraPos -= (m_moveVector * (float)mouseMove.y() + m_moveCrossVector * (float)mouseMove.x());
		MainFrame->UpdateNavigator();
	}

	D3DXVECTOR3 globalMove = m_terrainPos;
	_updateTerrainPos3D(mousePos);
	globalMove -= m_terrainPos;

	CObject* obj;
	if (m_editSpawn != -1)
	{
		if (g_global3D.renderObjects && m_pickingTerrain && CWorld::s_selection.GetSize() >= 1)
		{
			obj = CWorld::s_selection[CWorld::s_selection.GetSize() - 1];
			if (obj->m_visible)
			{
				const QPoint pos((int)(m_terrainPos.z + ((float)MPU / 2.0f)) / MPU * MPU,
					(int)(m_terrainPos.x + ((float)MPU / 2.0f)) / MPU * MPU);

				QRect rect;
				if (obj->m_type == OT_MOVER || obj->m_type == OT_ITEM || obj->m_type == OT_CTRL)
					rect = ((CSpawnObject*)obj)->m_rect;
				else if (obj->m_type == OT_REGION)
					rect = ((CRegion*)obj)->m_rect;

				rect = rect.normalized();

				switch (m_editSpawn)
				{
				case 0:
					rect.setBottomRight(pos);
					break;
				case 1:
					rect.setBottomLeft(pos);
					break;
				case 2:
					rect.setTopRight(pos);
					break;
				case 3:
					rect.setTopLeft(pos);
					break;
				}

				rect = rect.normalized();

				if (obj->m_type == OT_MOVER || obj->m_type == OT_ITEM || obj->m_type == OT_CTRL)
					((CSpawnObject*)obj)->m_rect = rect;
				else if (obj->m_type == OT_REGION)
					((CRegion*)obj)->m_rect = rect;
			}
		}
	}
	else if (m_editing)
	{
		if (m_pickingTerrain)
		{
			switch (editMode)
			{
			case EDIT_TERRAIN_HEIGHT:
				m_world->EditHeight((MainFrame->GetTerrainHeightEditMode() != 0) ? m_terrainPos : m_editTerrainPos, mouseMove, m_editTerrainPos.y);
				break;
			case EDIT_TERRAIN_TEXTURE:
			{
				const float unity = (float)(MAP_SIZE * MPU) / (float)((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE);
				QPoint editTexturePos((int)(m_terrainPos.x / unity + 0.5f), (int)(m_terrainPos.z / unity + 0.5f));
				if (editTexturePos != m_editTexturePos)
				{
					m_world->EditTexture(m_terrainPos);
					m_editTexturePos = editTexturePos;
				}

			}
			break;
			case EDIT_TERRAIN_COLOR:
			{
				const float unity = (float)(MAP_SIZE * MPU) / (float)((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE);
				QPoint editTexturePos((int)(m_terrainPos.x / unity + 0.5f), (int)(m_terrainPos.z / unity + 0.5f));
				if (editTexturePos != m_editTexturePos)
				{
					m_world->EditColor(m_terrainPos);
					m_editTexturePos = editTexturePos;
				}

			}
			break;
			case EDIT_TERRAIN_WATER:
				m_world->EditWater(m_terrainPos);
				break;
			}
		}

		if (editMode == EDIT_SELECT_OBJECTS || editMode == EDIT_MOVE_OBJECTS || editMode == EDIT_SCALE_OBJECTS || editMode == EDIT_ROTATE_OBJECTS)
		{
			EEditAxis axis = MainFrame->GetEditAxis();
			D3DXVECTOR3 temp;
			const int objCount = CWorld::s_selection.GetSize();
			const bool gravity = MainFrame->UseGravity();

			if (editMode == EDIT_ROTATE_OBJECTS || (QApplication::queryKeyboardModifiers() & Qt::AltModifier))
			{
				RotateObjects(axis, (float)(mouseMove.y()) / -3.0f);
			}
			else if (editMode == EDIT_MOVE_OBJECTS && (m_pickingTerrain || axis == EDIT_Y))
			{
				const bool useGrid = MainFrame->UseGrid();
				const int gridSize = MainFrame->GetGridSize();

				for (int i = 0; i < objCount; i++)
				{
					obj = CWorld::s_selection[i];
					temp = obj->m_basePos;

					switch (axis)
					{
					case EDIT_XZ:
						temp.x -= globalMove.x;
						temp.z -= globalMove.z;
						break;
					case  EDIT_X:
						temp.x -= globalMove.x;
						break;
					case EDIT_Z:
						temp.z -= globalMove.z;
						break;
					case EDIT_Y:
						temp.y += (float)mouseMove.y() / 12.0f;
						break;
					}

					if (temp.x < 0.0f)
						temp.x = 0.0f;
					else if (temp.x >= (float)(m_world->m_width * MAP_SIZE * MPU))
						temp.x = (float)(m_world->m_width * MAP_SIZE * MPU) - 0.0001f;
					if (temp.z < 0.0f)
						temp.z = 0.0f;
					else if (temp.z >= (float)(m_world->m_height * MAP_SIZE * MPU))
						temp.z = (float)(m_world->m_height * MAP_SIZE * MPU) - 0.0001f;

					obj->m_basePos = temp;

					if (useGrid)
					{
						switch (axis)
						{
						case EDIT_XZ:
							temp.x = (int)(temp.x + (float)gridSize / 2.0f) / gridSize * gridSize;
							temp.z = (int)(temp.z + (float)gridSize / 2.0f) / gridSize * gridSize;
							break;
						case  EDIT_X:
							temp.x = (int)(temp.x + (float)gridSize / 2.0f) / gridSize * gridSize;
							break;
						case EDIT_Z:
							temp.z = (int)(temp.z + (float)gridSize / 2.0f) / gridSize * gridSize;
							break;
						case EDIT_Y:
							temp.y = (int)(temp.y + (float)gridSize / 2.0f) / gridSize * gridSize;
							break;
						}
					}

					if (gravity)
					{
						temp.y = m_world->GetHeight(temp.x, temp.z);
						obj->m_basePos.y = temp.y;
					}

					m_world->MoveObject(obj, temp);
				}
			}
			else if (editMode == EDIT_SCALE_OBJECTS)
			{
				float factor = 0.0f;

				if (mousePos.y() > m_editBasePos.y())
					factor = (float)(mousePos.y() - m_editBasePos.y()) / -100.0f + 1.0f;
				else
					factor = (float)(m_editBasePos.y() - mousePos.y()) / 100.0f + 1.0f;

				for (int i = 0; i < CWorld::s_selection.GetSize(); i++)
				{
					obj = CWorld::s_selection[i];
					temp = obj->m_scale;

					switch (axis)
					{
					case EDIT_XZ:
						temp = obj->m_baseScale * factor;
						break;
					case  EDIT_X:
						temp.x = obj->m_baseScale.x * factor;
						break;
					case EDIT_Z:
						temp.z = obj->m_baseScale.z * factor;
						break;
					case EDIT_Y:
						temp.y = obj->m_baseScale.y * factor;
						break;
					}

					obj->SetScale(temp);
				}
			}
		}
	}

	_updateMouse3D(mousePos);

	if (!IsAutoRefresh())
		RenderEnvironment();

	m_lastMousePos = mousePos;
}

void CWorldEditor::wheelEvent(QWheelEvent *event)
{
	if (!m_world || g_global3D.renderMinimap)
		return;

	const float phiRadian = m_world->m_cameraAngle.y * M_PI / 180.0f;
	const float thetaRadian = m_world->m_cameraAngle.x * M_PI / 180.0f;
	D3DXVECTOR3 lookAt;
	lookAt.x = cos(phiRadian) * sin(thetaRadian);
	lookAt.y = sin(phiRadian);
	lookAt.z = cos(phiRadian) * cos(thetaRadian);
	m_world->m_cameraPos += lookAt * (float)event->delta() / 60.0f;

	event->accept();

	if (!IsAutoRefresh())
		RenderEnvironment();

	_updateTerrainPos3D(m_lastMousePos);
	_updateMouse3D(m_lastMousePos);

	MainFrame->UpdateNavigator();
}

void CWorldEditor::mousePressEvent(QMouseEvent * event)
{
	const QPoint mousePos(event->x(), event->y());

	if (!m_world || g_global3D.renderMinimap)
		return;

	const EEditMode editMode = MainFrame->GetEditMode();

	if (event->button() == Qt::RightButton)
	{
		m_rotatingCamera = true;
		m_lastRightClickPos = mousePos;
		if (editMode != EDIT_CONTINENT && !MainFrame->IsSelectionLocked())
		{
			if (CWorld::s_selection.Find(m_pickObject) == -1)
				CWorld::s_selection.RemoveAll();
			if (m_pickObject)
			{
				if (CWorld::s_selection.Find(m_pickObject) == -1)
					CWorld::s_selection.Append(m_pickObject);
			}
		}
		event->accept();
	}

	_updateTerrainPos3D(m_lastMousePos);

	if (event->button() == Qt::MiddleButton && m_pickingTerrain)
	{
		m_movingCamera = true;
		m_moveVector = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		m_moveCrossVector = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		const float phiRadian = m_world->m_cameraAngle.y * M_PI / 180.0f;
		const float thetaRadian = m_world->m_cameraAngle.x * M_PI / 180.0f;
		if (m_world->m_cameraAngle.y < -60.0f)
		{
			const float factor = abs(m_world->m_cameraPos.y - m_terrainPos.y) / 800.0f + 0.01f;
			m_moveVector.x = sin(thetaRadian);
			m_moveVector.z = cos(thetaRadian);
			m_moveVector *= factor;
			m_moveCrossVector.x = sin(thetaRadian - M_PI_2);
			m_moveCrossVector.z = cos(thetaRadian - M_PI_2);
			m_moveCrossVector *= factor;
		}
		else
		{
			D3DXVECTOR3 temp(m_terrainPos.x - m_world->m_cameraPos.x, 0.0f, m_terrainPos.z - m_world->m_cameraPos.z);
			const float length = D3DXVec3Length(&temp);
			D3DXVec3Normalize(&temp, &temp);
			D3DXVECTOR3 temp2 = m_terrainPos - m_world->m_cameraPos;
			temp2.y /= 10.0f;
			const float factor = D3DXVec3Length(&temp2) / 200.0f + 0.1f;
			m_moveVector.x = temp.x * factor;
			m_moveVector.z = temp.z * factor;
			m_moveCrossVector.x = cos(phiRadian) * sin(thetaRadian);
			m_moveCrossVector.y = sin(phiRadian);
			m_moveCrossVector.z = cos(phiRadian) * cos(thetaRadian);
			D3DXVec3Cross(&m_moveCrossVector, &m_moveCrossVector, &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
			m_moveCrossVector *= factor / 2.5f;
		}
		event->accept();
	}

	if (event->button() == Qt::LeftButton)
	{
		if (editMode == EDIT_ADD_OBJECTS && m_pickingTerrain)
		{
			CWorld::s_selection.RemoveAll();
			CObject* newObj, *obj;
			for (int i = 0; i < m_addObjects.GetSize(); i++)
			{
				obj = m_addObjects[i];
				if (obj->m_type == OT_PATH && MainFrame->GetCurrentPatrol() == -1)
					continue;

				newObj = CObject::CreateObject(obj->m_type);
				newObj->m_modelID = obj->m_modelID;
				newObj->SetPos(obj->GetPos());
				newObj->SetRot(obj->GetRot());
				if (newObj->Init())
				{
					newObj->ResetScale();
					m_world->AddObject(newObj);

					if (CWorld::s_selection.Find(newObj) == -1)
						CWorld::s_selection.Append(newObj);

					if (obj->m_type == OT_MOVER || obj->m_type == OT_ITEM || obj->m_type == OT_CTRL)
						((CSpawnObject*)newObj)->m_rect = ((CSpawnObject*)obj)->m_rect;
					else if (obj->m_type == OT_REGION)
						((CRegion*)newObj)->m_rect = ((CRegion*)obj)->m_rect;

					if (newObj->m_type == OT_MOVER)
					{
						CMover* mover = (CMover*)newObj;
						mover->m_belligerence = mover->m_moverProp->belligerence;
						mover->m_AIInterface = mover->m_moverProp->AI;
						mover->m_name = mover->m_moverProp->name;
					}

					if (obj->m_type == OT_PATH)
					{
						auto it = m_world->m_paths.find(MainFrame->GetCurrentPatrol());
						if (it != m_world->m_paths.end())
						{
							CPath* path = (CPath*)newObj;
							path->m_index = it.value()->GetSize();
							it.value()->Append(path);
						}
					}
				}
				else
					Delete(newObj);
			}
		}
		else if (editMode == EDIT_SELECT_OBJECTS || editMode == EDIT_MOVE_OBJECTS || editMode == EDIT_SCALE_OBJECTS || editMode == EDIT_ROTATE_OBJECTS)
		{
			bool editObjects = true;
			CObject* obj;

			if (g_global3D.renderObjects && m_pickingTerrain && CWorld::s_selection.GetSize() >= 1)
			{
				obj = CWorld::s_selection[CWorld::s_selection.GetSize() - 1];
				if (obj->m_visible)
				{
					bool spawn = false;
					QRect rect;
					if (obj->m_type == OT_REGION)
					{
						rect = ((CRegion*)obj)->m_rect;
						spawn = true;
					}
					if (g_global3D.renderSpawns && (obj->m_type == OT_MOVER || obj->m_type == OT_ITEM || obj->m_type == OT_CTRL) && ((CSpawnObject*)obj)->m_isRespawn)
					{
						rect = ((CSpawnObject*)obj)->m_rect;
						spawn = true;
					}
					if (spawn)
					{
						rect = rect.normalized();
						rect.setTopLeft((rect.topLeft() - QPoint(MPU / 2, MPU / 2)) / MPU * MPU);
						rect.setBottomRight((rect.bottomRight() - QPoint(MPU / 2, MPU / 2)) / MPU * MPU);
						D3DXVECTOR3 points[4];
						points[0] = D3DXVECTOR3(rect.bottom(), m_world->GetHeight(rect.bottom(), rect.right()), rect.right());
						points[1] = D3DXVECTOR3(rect.bottom(), m_world->GetHeight(rect.bottom(), rect.left()), rect.left());
						points[2] = D3DXVECTOR3(rect.top(), m_world->GetHeight(rect.top(), rect.right()), rect.right());
						points[3] = D3DXVECTOR3(rect.top(), m_world->GetHeight(rect.top(), rect.left()), rect.left());
						D3DXMATRIX identity;
						D3DXVECTOR3 out;
						QRect screenRect;
						D3DXMatrixIdentity(&identity);
						for (int i = 0; i < 4; i++)
						{
							D3DXVec3Project(&out, &points[i], &g_global3D.viewport, &g_global3D.proj, &g_global3D.view, &identity);
							screenRect = QRect(QPoint((int)out.x - 2, (int)out.y - 2), QSize(6, 6));
							if (screenRect.contains(mousePos))
							{
								m_editSpawn = i;
								editObjects = false;
							}
						}
					}
				}
			}

			if (editObjects)
			{
				if (MainFrame->IsSelectionLocked())
				{
					if (CWorld::s_selection.GetSize() > 0)
						m_editing = true;
				}
				else
				{
					if (m_pickObject)
					{
						if (CWorld::s_selection.Find(m_pickObject) != -1)
						{
							m_editing = true;
						}
						else
						{
							if (!(QApplication::queryKeyboardModifiers() & Qt::ControlModifier))
								CWorld::s_selection.RemoveAll();

							if (CWorld::s_selection.Find(m_pickObject) == -1)
								CWorld::s_selection.Append(m_pickObject);
							m_editing = true;
						}
					}
					else
					{
						m_selectRect = true;
						m_selectRectPos = mousePos;
					}
				}
			}

			if (m_editing)
			{
				m_editBasePos = mousePos;
				for (int i = 0; i < CWorld::s_selection.GetSize(); i++)
				{
					obj = CWorld::s_selection[i];
					obj->m_baseScale = obj->m_scale;
					obj->m_basePos = obj->m_pos;
				}
				if (editMode == EDIT_MOVE_OBJECTS && MainFrame->GetEditAxis() != EDIT_Y && !m_pickingTerrain)
					m_editing = false;
			}
		}
		else if (m_pickingTerrain)
		{
			m_editing = true;
			m_editTerrainPos = m_terrainPos;
			const float unity = (float)(MAP_SIZE * MPU) / (float)((PATCH_SIZE - 1) * NUM_PATCHES_PER_SIDE);
			m_editTexturePos = QPoint((int)(m_terrainPos.x / unity + 0.5f), (int)(m_terrainPos.z / unity + 0.5f));

			switch (editMode)
			{
			case EDIT_TERRAIN_HEIGHT:
				if (MainFrame->GetTerrainHeightEditMode() != 0)
					m_world->EditHeight(m_terrainPos, QPoint(0, 0), m_editTerrainPos.y);
				break;
			case EDIT_TERRAIN_TEXTURE:
				m_world->EditTexture(m_terrainPos);
				break;
			case EDIT_TERRAIN_COLOR:
				m_world->EditColor(m_terrainPos);
				break;
			case EDIT_TERRAIN_WATER:
				m_world->EditWater(m_terrainPos);
				break;
			}
		}

		event->accept();
	}

	_updateMouse3D(mousePos);

	if (!IsAutoRefresh())
		RenderEnvironment();

	m_lastMousePos = mousePos;
}

void CWorldEditor::mouseReleaseEvent(QMouseEvent * event)
{
	const QPoint mousePos(event->x(), event->y());
	const EEditMode editMode = MainFrame->GetEditMode();

	if (event->button() == Qt::RightButton)
	{
		if (m_lastRightClickPos == mousePos)
		{
			if (editMode == EDIT_CONTINENT && m_world && m_world->m_continent
				&& m_world->m_continent->vertices.size() > 0)
			{
				m_world->m_continent->vertices.removeLast();
				MainFrame->UpdateContinentVertices();
			}
			else
			{
				MouseLost();
				QMenu* editMenu;
				if (CWorld::s_selection.GetSize() > 0 && (CWorld::s_selection.Find(m_pickObject) != -1 || MainFrame->IsSelectionLocked()))
					editMenu = MainFrame->GetObjectMenu();
				else
					editMenu = MainFrame->GetNoObjectMenu();
				if (editMenu->exec(event->globalPos()) == MainFrame->GetObjPropertiesAction() && CWorld::s_selection.GetSize() > 0 && (CWorld::s_selection.Find(m_pickObject) != -1 || MainFrame->IsSelectionLocked()))
				{
					if (m_pickObject->m_type != OT_OBJ && m_pickObject->m_type != OT_SFX && m_pickObject->m_type != OT_SHIP
						&& (m_pickObject->m_type != OT_REGION || m_pickObject->m_modelID != RI_BEGIN))
					{
						MainFrame->HideDialogs();
						CObjectPropertiesDialog dialog(m_pickObject, this);
						dialog.exec();
						MainFrame->ShowDialogs();
					}
				}
			}
		}

		m_rotatingCamera = false;
		event->accept();
	}
	else if (event->button() == Qt::MiddleButton)
	{
		m_movingCamera = false;
		event->accept();
	}
	else if (event->button() == Qt::LeftButton)
	{
		if (editMode == EDIT_CONTINENT && m_world && m_world->m_continent
			&& m_pickingTerrain && m_world->m_continent->vertices.size() < MAX_CONTINENT_VERTICES)
		{
			m_world->m_continent->vertices.push_back(D3DXVECTOR3((int)(m_terrainPos.x + 0.5f), (int)(m_terrainPos.y + 0.5f), (int)(m_terrainPos.z + 0.5f)));
			MainFrame->UpdateContinentVertices();
		}

		if (m_selectRect)
		{
			if (!(QApplication::queryKeyboardModifiers() & Qt::ControlModifier))
				CWorld::s_selection.RemoveAll();
			if (m_selectRectPos != m_lastMousePos)
				m_world->SelectObjects(QRect(m_selectRectPos, m_lastMousePos).normalized());
		}

		m_editing = false;
		m_selectRect = false;
		m_editSpawn = -1;
		m_editTexturePos = QPoint(-1, -1);
		event->accept();
	}

	if (!IsAutoRefresh())
		RenderEnvironment();
}

void CWorldEditor::MouseLost()
{
	m_rotatingCamera = false;
	m_movingCamera = false;
	m_editing = false;
	m_selectRect = false;
	m_editTexturePos = QPoint(-1, -1);
	m_editSpawn = -1;
}

void CWorldEditor::_updateMouse3D(const QPoint& pt)
{
	D3DXVECTOR3 terrainPos(0, 0, 0);
	int landX = 0, landY = 0;
	int layerCount = 0;
	string objectName, terrain;
	int waterHeight = -1;

	if (m_pickingTerrain)
	{
		CLandscape* land = m_world->GetLand(m_terrainPos);
		if (land)
		{
			const int textureID = land->GetTextureID(m_terrainPos);
			if (textureID != -1)
				terrain = Project->GetTerrainFilename(textureID);

			terrainPos = m_terrainPos;
			landX = land->m_posX / MAP_SIZE;
			landY = land->m_posY / MAP_SIZE;
			layerCount = land->m_layers.GetSize();

			const WaterHeight* water = m_world->GetWaterHeight(m_terrainPos.x, m_terrainPos.z);
			if (water && (water->texture & (byte)(~MASK_WATERFRAME)) == WTYPE_WATER)
				waterHeight = (int)water->height;

			MainFrame->SetLayerInfos(land);
		}
	}

	m_pickObject = m_world->PickObject(pt);
	if (m_pickObject)
		objectName = m_pickObject->GetModelFilename();

	MainFrame->SetStatusBarInfo(m_terrainPos,
		landX,
		landY,
		layerCount,
		objectName,
		terrain,
		waterHeight);
}

void CWorldEditor::_updateTerrainPos3D(const QPoint& pt)
{
	D3DXVECTOR3 mouse3DPos(0, 0, 0);
	m_pickingTerrain = m_world->PickTerrain(pt, mouse3DPos) && m_world->GetLand(mouse3DPos);
	m_terrainPos = m_pickingTerrain ? mouse3DPos : D3DXVECTOR3(0, 0, 0);
}

void CWorldEditor::SetWorld(CWorld* world)
{
	m_world = world;
	m_rotatingCamera = false;
	m_movingCamera = false;
	m_pickingTerrain = false;
	m_terrainPos = D3DXVECTOR3(0, 0, 0);
	m_moveVector = D3DXVECTOR3(0, 0, 0);
	m_moveCrossVector = D3DXVECTOR3(0, 0, 0);
	m_editTexturePos = QPoint(-1, -1);
	m_editing = false;
	m_pickObject = null;
	m_addObjects.RemoveAll();
	m_addObject = null;
	CWorld::s_selection.RemoveAll();
	m_selectRect = false;
	m_editSpawn = -1;

	if (!IsAutoRefresh())
		RenderEnvironment();
}

void CWorldEditor::_updateAddObjects()
{
	int i;
	if (MainFrame->GetEditMode() == EDIT_ADD_OBJECTS)
	{
		if (m_pickingTerrain)
		{
			if (m_addObject != MainFrame->GetAddObject())
			{
				for (i = 0; i < m_addObjects.GetSize(); i++)
					m_world->DeleteObject(m_addObjects[i]);
				m_addObjects.RemoveAll();

				m_addObject = MainFrame->GetAddObject();

				if (m_addObject->type() == GAMEELE_MODEL)
				{
					ModelProp* model = ((CModelElement*)m_addObject)->GetModel();
					CObject* obj = CObject::CreateObject(model->type);
					obj->m_modelID = model->ID;
					obj->m_isReal = false;
					obj->SetPos(m_terrainPos);
					obj->SetRot(D3DXVECTOR3(0, 0, 0));
					if (obj->Init())
					{
						obj->ResetScale();
						if (obj->m_type == OT_MOVER || obj->m_type == OT_ITEM || obj->m_type == OT_CTRL)
						{
							CSpawnObject* dyna = ((CSpawnObject*)obj);
							dyna->m_rect = QRect(
								QPoint((int)(obj->m_pos.z - (float)MPU + 0.5f), (int)(obj->m_pos.x - (float)MPU + 0.5f)),
								QSize(4 * MPU, 4 * MPU)
								);

							if (dyna->m_rect.left() < 0)
								dyna->m_rect.setLeft(0);
							else if (dyna->m_rect.left() >= m_world->m_height * MAP_SIZE * MPU)
								dyna->m_rect.setLeft(m_world->m_height * MAP_SIZE * MPU - 1);
							if (dyna->m_rect.right() < 0)
								dyna->m_rect.setRight(0);
							else if (dyna->m_rect.right() >= m_world->m_height * MAP_SIZE * MPU)
								dyna->m_rect.setRight(m_world->m_height * MAP_SIZE * MPU - 1);
							if (dyna->m_rect.bottom() < 0)
								dyna->m_rect.setBottom(0);
							else if (dyna->m_rect.bottom() >= m_world->m_width * MAP_SIZE * MPU)
								dyna->m_rect.setBottom(m_world->m_width * MAP_SIZE * MPU - 1);
							if (dyna->m_rect.top() < 0)
								dyna->m_rect.setTop(0);
							else if (dyna->m_rect.top() >= m_world->m_width * MAP_SIZE * MPU)
								dyna->m_rect.setTop(m_world->m_height * MAP_SIZE * MPU - 1);
							dyna->m_rect = dyna->m_rect.normalized();
						}
						else if (obj->m_type == OT_REGION)
						{
							CRegion* dyna = ((CRegion*)obj);
							dyna->m_rect = QRect(
								QPoint((int)(obj->m_pos.z - (float)MPU + 0.5f), (int)(obj->m_pos.x - (float)MPU + 0.5f)),
								QSize(4 * MPU, 4 * MPU)
								);

							if (dyna->m_rect.left() < 0)
								dyna->m_rect.setLeft(0);
							else if (dyna->m_rect.left() >= m_world->m_height * MAP_SIZE * MPU)
								dyna->m_rect.setLeft(m_world->m_height * MAP_SIZE * MPU - 1);
							if (dyna->m_rect.right() < 0)
								dyna->m_rect.setRight(0);
							else if (dyna->m_rect.right() >= m_world->m_height * MAP_SIZE * MPU)
								dyna->m_rect.setRight(m_world->m_height * MAP_SIZE * MPU - 1);
							if (dyna->m_rect.bottom() < 0)
								dyna->m_rect.setBottom(0);
							else if (dyna->m_rect.bottom() >= m_world->m_width * MAP_SIZE * MPU)
								dyna->m_rect.setBottom(m_world->m_width * MAP_SIZE * MPU - 1);
							if (dyna->m_rect.top() < 0)
								dyna->m_rect.setTop(0);
							else if (dyna->m_rect.top() >= m_world->m_width * MAP_SIZE * MPU)
								dyna->m_rect.setTop(m_world->m_height * MAP_SIZE * MPU - 1);
							dyna->m_rect = dyna->m_rect.normalized();
						}
						m_world->AddObject(obj);
						m_addObjects.Append(obj);
					}
					else
						Delete(obj);
				}
				else if (m_addObject->type() == GAMEELE_FOLDER)
				{
					for (i = 0; i < m_addObject->rowCount(); i++)
					{
						if (m_addObject->child(i)->type() == GAMEELE_MODEL)
						{
							ModelProp* model = ((CModelElement*)m_addObject->child(i))->GetModel();
							CObject* obj = CObject::CreateObject(model->type);
							obj->m_modelID = model->ID;
							obj->m_isReal = false;
							if (obj->Init())
							{
								obj->SetRot(D3DXVECTOR3(0, 180, 0));
								obj->ResetScale();
								obj->SetPos(m_terrainPos);

								if (obj->m_type == OT_MOVER || obj->m_type == OT_ITEM || obj->m_type == OT_CTRL)
								{
									CSpawnObject* dyna = ((CSpawnObject*)obj);
									dyna->m_rect = QRect(
										QPoint((int)(obj->m_pos.z - (float)MPU + 0.5f), (int)(obj->m_pos.x - (float)MPU + 0.5f)),
										QSize(4 * MPU, 4 * MPU)
										);

									if (dyna->m_rect.left() < 0)
										dyna->m_rect.setLeft(0);
									else if (dyna->m_rect.left() >= m_world->m_height * MAP_SIZE * MPU)
										dyna->m_rect.setLeft(m_world->m_height * MAP_SIZE * MPU - 1);
									if (dyna->m_rect.right() < 0)
										dyna->m_rect.setRight(0);
									else if (dyna->m_rect.right() >= m_world->m_height * MAP_SIZE * MPU)
										dyna->m_rect.setRight(m_world->m_height * MAP_SIZE * MPU - 1);
									if (dyna->m_rect.bottom() < 0)
										dyna->m_rect.setBottom(0);
									else if (dyna->m_rect.bottom() >= m_world->m_width * MAP_SIZE * MPU)
										dyna->m_rect.setBottom(m_world->m_width * MAP_SIZE * MPU - 1);
									if (dyna->m_rect.top() < 0)
										dyna->m_rect.setTop(0);
									else if (dyna->m_rect.top() >= m_world->m_width * MAP_SIZE * MPU)
										dyna->m_rect.setTop(m_world->m_height * MAP_SIZE * MPU - 1);
									dyna->m_rect = dyna->m_rect.normalized();
								}
								else if (obj->m_type == OT_REGION)
								{
									CRegion* dyna = ((CRegion*)obj);
									dyna->m_rect = QRect(
										QPoint((int)(obj->m_pos.z - (float)MPU + 0.5f), (int)(obj->m_pos.x - (float)MPU + 0.5f)),
										QSize(4 * MPU, 4 * MPU)
										);

									if (dyna->m_rect.left() < 0)
										dyna->m_rect.setLeft(0);
									else if (dyna->m_rect.left() >= m_world->m_height * MAP_SIZE * MPU)
										dyna->m_rect.setLeft(m_world->m_height * MAP_SIZE * MPU - 1);
									if (dyna->m_rect.right() < 0)
										dyna->m_rect.setRight(0);
									else if (dyna->m_rect.right() >= m_world->m_height * MAP_SIZE * MPU)
										dyna->m_rect.setRight(m_world->m_height * MAP_SIZE * MPU - 1);
									if (dyna->m_rect.bottom() < 0)
										dyna->m_rect.setBottom(0);
									else if (dyna->m_rect.bottom() >= m_world->m_width * MAP_SIZE * MPU)
										dyna->m_rect.setBottom(m_world->m_width * MAP_SIZE * MPU - 1);
									if (dyna->m_rect.top() < 0)
										dyna->m_rect.setTop(0);
									else if (dyna->m_rect.top() >= m_world->m_width * MAP_SIZE * MPU)
										dyna->m_rect.setTop(m_world->m_height * MAP_SIZE * MPU - 1);
									dyna->m_rect = dyna->m_rect.normalized();
								}
								m_world->AddObject(obj);
								m_addObjects.Append(obj);
							}
							else
								Delete(obj);
						}
					}
				}
			}

			for (i = 0; i < m_addObjects.GetSize(); i++)
				m_world->MoveObject(m_addObjects[i], m_terrainPos);
		}
	}
	else if (m_addObjects.GetSize() > 0)
	{
		for (i = 0; i < m_addObjects.GetSize(); i++)
			m_world->DeleteObject(m_addObjects[i]);
		m_addObjects.RemoveAll();
		m_addObject = null;
	}
}

void CWorldEditor::RotateObjects(int axis, float factor)
{
	if (!m_world || g_global3D.renderMinimap)
		return;

	const int objCount = CWorld::s_selection.GetSize();
	if (!objCount)
		return;

	D3DXVECTOR3 temp, temp2;
	const bool gravity = MainFrame->UseGravity();
	CObject* obj;
	const float s = sin(D3DXToRadian(factor));
	const float c = cos(D3DXToRadian(factor));
	float x, z;

	D3DXVECTOR3 centroid(0, 0, 0);
	if (objCount > 1)
	{
		for (int i = 0; i < objCount; i++)
			centroid += CWorld::s_selection[i]->m_pos;
		centroid /= (float)objCount;
	}

	for (int i = 0; i < objCount; i++)
	{
		obj = CWorld::s_selection[i];
		temp = obj->m_pos;
		temp2 = obj->m_rot;

		if (objCount > 1)
		{
			temp2.y += factor;
			temp.x -= centroid.x;
			temp.z -= centroid.z;
			x = temp.x * c - temp.z * s;
			z = temp.x * s + temp.z * c;
			temp.x = x + centroid.x;
			temp.z = z + centroid.z;

			if (gravity)
				temp.y = m_world->GetHeight(temp.x, temp.z);

			m_world->MoveObject(obj, temp);
			obj->m_basePos = temp;
		}
		else
		{
			switch (axis)
			{
			case EDIT_X:
				temp2.x += factor;
				break;
			case EDIT_XZ:
			case EDIT_Y:
				temp2.y += factor;
				break;
			case EDIT_Z:
				temp2.z += factor;
				break;
			}
		}

		temp2.x = fmod(temp2.x, 360.0f);
		temp2.y = fmod(temp2.y, 360.0f);
		temp2.z = fmod(temp2.z, 360.0f);
		obj->SetRot(temp2);
	}
}

void CWorldEditor::SaveBigmap(const string& filename)
{
	QImage output(m_world->m_width * MINIMAP_SIZE, m_world->m_height * MINIMAP_SIZE, QImage::Format_RGB888);
	QPainter painter;
	painter.begin(&output);

	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = MINIMAP_SIZE;
	rect.bottom = MINIMAP_SIZE;
	LPDIRECT3DSURFACE9 surface;

	g_global3D.renderMinimap = true;
	const D3DXVECTOR3 oldCamPos = m_world->m_cameraPos;
	const bool oldFog = g_global3D.fog;
	const bool oldTerrainLOD = g_global3D.terrainLOD;
	const bool oldAnimate = g_global3D.animate;

	g_global3D.animate = false;
	g_global3D.fog = false;
	g_global3D.terrainLOD = false;
	m_world->m_cameraPos.y = 1000.0f;

	SetAutoRefresh(false);

	int x, y;
	for (x = 0; x < m_world->m_width; x++)
	{
		for (y = 0; y < m_world->m_height; y++)
		{
			m_world->m_cameraPos.x = (float)((x * MAP_SIZE + MAP_SIZE / 2) * MPU) + 0.5f;
			m_world->m_cameraPos.z = (float)((y * MAP_SIZE + MAP_SIZE / 2) * MPU) - 0.5f;

			RenderEnvironment();

			if (m_world->m_width * m_world->m_height > 100 && !m_world->m_filename.isEmpty())
				m_world->_cleanLndFiles();

			qApp->processEvents();

			m_device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
			D3DXSaveSurfaceToFile(L"~minimap_temp.png", D3DXIFF_PNG, surface, null, &rect);
			Release(surface);

			qApp->processEvents();

			QImage input;
			if (input.load("~minimap_temp.png"))
				painter.drawImage(QPoint(x * MINIMAP_SIZE, (m_world->m_height - 1 - y) * MINIMAP_SIZE), input);

			qApp->processEvents();
		}
	}

	SetAutoRefresh(oldAnimate);
	g_global3D.animate = oldAnimate;
	g_global3D.renderMinimap = false;
	m_world->m_cameraPos = oldCamPos;
	g_global3D.fog = oldFog;
	g_global3D.terrainLOD = oldTerrainLOD;

	if (QFileInfo("~minimap_temp.png").exists())
		QFile::remove("~minimap_temp.png");

	qApp->processEvents();
	painter.end();
	output.save(filename);
	qApp->processEvents();

	MainFrame->UpdateNavigator();
	RenderEnvironment();
}

void CWorldEditor::SaveMinimaps()
{
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = MINIMAP_SIZE;
	rect.bottom = MINIMAP_SIZE;
	LPDIRECT3DSURFACE9 surface;

	g_global3D.renderMinimap = true;
	const D3DXVECTOR3 oldCamPos = m_world->m_cameraPos;
	const bool oldFog = g_global3D.fog;
	const bool oldTerrainLOD = g_global3D.terrainLOD;
	const bool oldAnimate = g_global3D.animate;

	g_global3D.animate = false;
	g_global3D.fog = false;
	g_global3D.terrainLOD = false;
	m_world->m_cameraPos.y = 1000.0f;

	wchar_t temp[2048];
	int tempSize;

	QVector<CLandscape*> renderLands;
	for (int i = 0; i < m_world->m_width * m_world->m_height; i++)
		if (m_world->m_lands[i])
			renderLands.push_back(m_world->m_lands[i]);

	SetAutoRefresh(false);

	int x, y;
	for (x = 0; x < m_world->m_width; x++)
	{
		for (y = 0; y < m_world->m_height; y++)
		{
			m_world->m_cameraPos.x = (float)((x * MAP_SIZE + MAP_SIZE / 2) * MPU) + 0.5f;
			m_world->m_cameraPos.z = (float)((y * MAP_SIZE + MAP_SIZE / 2) * MPU) - 0.5f;

			if (!renderLands.contains(m_world->GetLand(m_world->m_cameraPos)))
				continue;

			RenderEnvironment();

			qApp->processEvents();

			const string filename = m_world->m_filename % string().sprintf("%02d-%02d.dds", x, y);
			tempSize = filename.toWCharArray(temp);
			temp[tempSize] = '\0';

			m_device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
			D3DXSaveSurfaceToFile(temp, D3DXIFF_DDS, surface, null, &rect);
			Release(surface);

			qApp->processEvents();
		}
	}

	SetAutoRefresh(oldAnimate);
	g_global3D.animate = oldAnimate;
	g_global3D.renderMinimap = false;
	m_world->m_cameraPos = oldCamPos;
	g_global3D.fog = oldFog;
	g_global3D.terrainLOD = oldTerrainLOD;

	MainFrame->UpdateNavigator();
	RenderEnvironment();
}