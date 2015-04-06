///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#include "stdafx.h"
#include "MainFrame.h"
#include "WorldEditor.h"
#include <World.h>
#include <Mover.h>
#include <Ctrl.h>
#include <Region.h>
#include <ModelMng.h>
#include <Path.h>

#include "ui_Random.h"

void CMainFrame::AddPath()
{
	if (!m_world)
		return;

	int i;
	for (i = 0; i < INT_MAX; i++)
		if (m_world->m_paths.find(i) == m_world->m_paths.end())
			break;

	m_world->m_paths[i] = new CPtrArray<CPath>();

	QListWidgetItem* item = new QListWidgetItem("Path " % string::number(i), ui.patrolList);
	item->setData(Qt::UserRole + 1, i);
	ui.patrolList->addItem(item);
}

void CMainFrame::RemovePath()
{
	if (!m_world)
		return;

	m_currentPatrol = -1;
	const int current = ui.patrolList->currentRow();
	if (current == -1)
		return;

	QListWidgetItem* item = ui.patrolList->takeItem(current);
	if (!item)
		return;

	const int index = item->data(Qt::UserRole + 1).toInt();
	Delete(item);

	auto it = m_world->m_paths.find(index);
	if (it != m_world->m_paths.end())
	{
		CObject* obj;
		while (it.value()->GetSize() > 0)
		{
			obj = (CObject*)it.value()->GetAt(0);
			const int find = CWorld::s_selection.Find(obj);
			if (find != -1)
				CWorld::s_selection.RemoveAt(find);
			m_world->DeleteObject(obj);
		}
		Delete(m_world->m_paths[index]);
		m_world->m_paths.remove(index);

		CMover* mover;
		for (int i = 0; i < m_world->m_objects[OT_MOVER].GetSize(); i++)
		{
			mover = (CMover*)m_world->m_objects[OT_MOVER].GetAt(i);
			if (mover->m_patrolIndex == index)
				mover->m_patrolIndex = -1;
		}
	}

	if (!m_editor->IsAutoRefresh())
		m_editor->RenderEnvironment();
}

void CMainFrame::SetCurrentPath(int row)
{
	if (!m_world)
		return;

	QListWidgetItem* item = ui.patrolList->item(row);
	if (!item)
		return;

	const int index = item->data(Qt::UserRole + 1).toInt();

	auto it = m_world->m_paths.find(index);
	if (it != m_world->m_paths.end())
		m_currentPatrol = index;
	else
		m_currentPatrol = -1;
}

void CMainFrame::keyPressEvent(QKeyEvent* event)
{
	if (m_editor && m_world && (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right))
	{
		RotateObjects(event->key());
		event->accept();
	}
}

void CMainFrame::RotateObjects(int key)
{
	if (!m_editor || !m_world)
		return;

	if (key == Qt::Key_Left)
		m_editor->RotateObjects(EDIT_Y, 35.0f);
	if (key == Qt::Key_Right)
		m_editor->RotateObjects(EDIT_Y, -35.0f);

	if (!m_editor->IsAutoRefresh())
		m_editor->RenderEnvironment();
}

void CMainFrame::SetOnLand()
{
	if (!m_world)
		return;

	D3DXVECTOR3 temp;
	CObject* obj;
	for (int i = 0; i < CWorld::s_selection.GetSize(); i++)
	{
		obj = CWorld::s_selection[i];
		temp = obj->m_pos;
		temp.y = m_world->GetHeight(temp.x, temp.z);
		m_world->MoveObject(obj, temp);
		obj->m_basePos = temp;
	}

	if (!m_editor->IsAutoRefresh())
		m_editor->RenderEnvironment();
}

void CMainFrame::RandomScaleAndSize()
{
	if (!m_world)
		return;

	HideDialogs();
	QDialog dialog(this);
	Ui::RandomDialog ui;
	ui.setupUi(&dialog);

	if (dialog.exec() == QDialog::Accepted)
	{
		D3DXVECTOR3 temp;
		CObject* obj;
		float min, max, temp2;

		if (ui.scale->isChecked())
		{
			min = (float)ui.minScale->value();
			max = (float)ui.maxScale->value();
			if (min > max)
			{
				const float temp2 = min;
				min = max;
				max = temp2;
			}

			for (int i = 0; i < CWorld::s_selection.GetSize(); i++)
			{
				obj = CWorld::s_selection[i];
				if (min > max)
					temp2 = max + (float)(rand()) / ((float)RAND_MAX / (min - max));
				else
					temp2 = min + (float)(rand()) / ((float)RAND_MAX / (max - min));
				temp.x = temp2;
				temp.y = temp2;
				temp.z = temp2;
				obj->SetScale(temp);
			}
		}

		if (ui.rotation->isChecked())
		{
			min = (float)ui.minRot->value();
			max = (float)ui.maxRot->value();

			for (int i = 0; i < CWorld::s_selection.GetSize(); i++)
			{
				obj = CWorld::s_selection[i];
				if (min > max)
					temp2 = max + (float)(rand()) / ((float)RAND_MAX / (min - max));
				else
					temp2 = min + (float)(rand()) / ((float)RAND_MAX / (max - min));
				temp.x = obj->m_rot.x;
				temp.y = temp2;
				temp.z = obj->m_rot.z;
				obj->SetRot(temp);
			}
		}
	}

	ShowDialogs();

	if (!m_editor->IsAutoRefresh())
		m_editor->RenderEnvironment();
}

void CMainFrame::DeleteObjects()
{
	if (!m_world)
		return;

	static CObject* test[1024];


	for (int i = 0; i < CWorld::s_selection.GetSize(); i++)
	{
		test[i] = CWorld::s_selection[i];
		m_world->DeleteObject(CWorld::s_selection[i]);
	}
	CWorld::s_selection.RemoveAll();

	if (!m_editor->IsAutoRefresh())
		m_editor->RenderEnvironment();
}

void CMainFrame::CopyObjects()
{
	if (!m_world || CWorld::s_selection.GetSize() < 1)
		return;

	for (int i = 0; i < m_clipboardObjects.GetSize(); i++)
		Delete(m_clipboardObjects[i]);
	m_clipboardObjects.RemoveAll();

	D3DXVECTOR3 center(0, 0, 0);
	for (int i = 0; i < CWorld::s_selection.GetSize(); i++)
		center += CWorld::s_selection[i]->m_pos;
	center /= (float)CWorld::s_selection.GetSize();

	CObject* obj, *newObj;
	for (int i = 0; i < CWorld::s_selection.GetSize(); i++)
	{
		obj = CWorld::s_selection[i];
		newObj = CObject::CreateObject(obj->m_type);

		switch (obj->m_type)
		{
		case OT_MOVER:
			*((CMover*)newObj) = *((CMover*)obj);
			break;
		case OT_CTRL:
			*((CCtrl*)newObj) = *((CCtrl*)obj);
			break;
		case OT_ITEM:
			*((CSpawnObject*)newObj) = *((CSpawnObject*)obj);
			break;
		case OT_REGION:
			*((CRegion*)newObj) = *((CRegion*)obj);
			break;
		case OT_PATH:
			*((CPath*)newObj) = *((CPath*)obj);
			break;
		default:
			*newObj = *obj;
			break;
		}

		newObj->m_model = null;
		newObj->m_world = null;
		newObj->m_pos = obj->m_pos - center;

		if (obj->m_type == OT_MOVER || obj->m_type == OT_ITEM || obj->m_type == OT_CTRL)
		{
			((CSpawnObject*)newObj)->m_rect = QRect(
				((CSpawnObject*)obj)->m_rect.topLeft() - QPoint((int)(obj->m_pos.z + 0.5f), (int)(obj->m_pos.x + 0.5f)),
				((CSpawnObject*)obj)->m_rect.size()
				);
		}
		else if (obj->m_type == OT_REGION)
		{
			((CRegion*)newObj)->m_rect = QRect(
				((CRegion*)obj)->m_rect.topLeft() - QPoint((int)(obj->m_pos.z + 0.5f), (int)(obj->m_pos.x + 0.5f)),
				((CRegion*)obj)->m_rect.size()
				);
		}

		m_clipboardObjects.Append(newObj);
	}
}

void CMainFrame::PasteObjects()
{
	if (!m_world || m_clipboardObjects.GetSize() < 1 || !m_editor->IsPickingTerrain())
		return;

	const D3DXVECTOR3 terrainPos = m_editor->GetTerrainPos();
	CWorld::s_selection.RemoveAll();

	CObject* obj, *newObj;
	for (int i = 0; i < m_clipboardObjects.GetSize(); i++)
	{
		obj = m_clipboardObjects[i];
		if (obj->m_type == OT_PATH && MainFrame->GetCurrentPatrol() == -1)
			continue;

		newObj = CObject::CreateObject(obj->m_type);

		switch (obj->m_type)
		{
		case OT_MOVER:
			*((CMover*)newObj) = *((CMover*)obj);
			break;
		case OT_CTRL:
			*((CCtrl*)newObj) = *((CCtrl*)obj);
			break;
		case OT_ITEM:
			*((CSpawnObject*)newObj) = *((CSpawnObject*)obj);
			break;
		case OT_REGION:
			*((CRegion*)newObj) = *((CRegion*)obj);
			break;
		case OT_PATH:
			*((CPath*)newObj) = *((CPath*)obj);
			break;
		default:
			*newObj = *obj;
			break;
		}

		newObj->SetPos(obj->m_pos + terrainPos);

		if (obj->m_type == OT_MOVER || obj->m_type == OT_ITEM || obj->m_type == OT_CTRL)
		{
			CSpawnObject* dyna = ((CSpawnObject*)newObj);
			dyna->m_rect = QRect(
				QPoint((int)(newObj->m_pos.z + 0.5f), (int)(newObj->m_pos.x + 0.5f)) + dyna->m_rect.topLeft(),
				dyna->m_rect.size()
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
			CRegion* dyna = ((CRegion*)newObj);
			dyna->m_rect = QRect(
				QPoint((int)(newObj->m_pos.z + 0.5f), (int)(newObj->m_pos.x + 0.5f)) + dyna->m_rect.topLeft(),
				dyna->m_rect.size()
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
		else if (obj->m_type == OT_PATH)
		{
			auto it = m_world->m_paths.find(MainFrame->GetCurrentPatrol());
			if (it != m_world->m_paths.end())
			{
				CPath* path = (CPath*)newObj;
				path->m_index = it.value()->GetSize();
				it.value()->Append(path);
			}
		}

		m_world->AddObject(newObj);

		if (CWorld::s_selection.Find(newObj) == -1)
			CWorld::s_selection.Append(newObj);
	}

	if (!m_editor->IsAutoRefresh())
		m_editor->RenderEnvironment();
}

void CMainFrame::CutObjects()
{
	if (!m_world)
		return;

	CopyObjects();
	DeleteObjects();
}

void CMainFrame::HideObjects()
{
	if (!m_world)
		return;

	CObject* obj;
	for (int i = 0; i < CWorld::s_selection.GetSize(); i++)
	{
		obj = CWorld::s_selection[i];
		if (!obj->m_isUnvisible)
		{
			obj->m_isUnvisible = true;

			if (g_global3D.spawnAllMovers
				&& (obj->m_type == OT_MOVER || obj->m_type == OT_ITEM || obj->m_type == OT_CTRL)
				&& ((CSpawnObject*)obj)->m_isRespawn)
				m_world->SpawnObject(obj, false);
		}
	}
	CWorld::s_selection.RemoveAll();

	if (!m_editor->IsAutoRefresh())
		m_editor->RenderEnvironment();
}

void CMainFrame::ShowAllObjects()
{
	if (!m_world)
		return;

	CObject* obj;
	int i, j;
	for (i = 0; i < MAX_OBJTYPE; i++)
	{
		for (j = 0; j < m_world->m_objects[i].GetSize(); j++)
		{
			obj = m_world->m_objects[i].GetAt(j);
			if (obj->m_isUnvisible)
			{
				obj->m_isUnvisible = false;

				if (g_global3D.spawnAllMovers
					&& (obj->m_type == OT_MOVER || obj->m_type == OT_ITEM || obj->m_type == OT_CTRL)
					&& ((CSpawnObject*)obj)->m_isRespawn)
					m_world->SpawnObject(obj, true);
			}
		}
	}

	if (!m_editor->IsAutoRefresh())
		m_editor->RenderEnvironment();
}

void CMainFrame::HideUpstairObjects()
{
	if (!m_world || CWorld::s_selection.GetSize() < 1)
		return;

	D3DXVECTOR3 center(0, 0, 0);
	for (int i = 0; i < CWorld::s_selection.GetSize(); i++)
		center += CWorld::s_selection[i]->m_pos;
	center /= (float)CWorld::s_selection.GetSize();

	CWorld::s_selection.RemoveAll();

	CObject* obj;
	int i, j;
	for (i = 0; i < MAX_OBJTYPE; i++)
	{
		for (j = 0; j < m_world->m_objects[i].GetSize(); j++)
		{
			obj = m_world->m_objects[i].GetAt(j);
			if (!obj->m_isUnvisible)
			{
				if (obj->m_pos.y < center.y)
					continue;

				if (D3DXVec2Length(&(D3DXVECTOR2(obj->m_pos.x, obj->m_pos.z) - D3DXVECTOR2(center.x, center.z))) > 100.0f)
					continue;

				obj->m_isUnvisible = true;

				if (g_global3D.spawnAllMovers
					&& (obj->m_type == OT_MOVER || obj->m_type == OT_ITEM || obj->m_type == OT_CTRL)
					&& ((CSpawnObject*)obj)->m_isRespawn)
					m_world->SpawnObject(obj, false);
			}
		}
	}

	if (!m_editor->IsAutoRefresh())
		m_editor->RenderEnvironment();
}