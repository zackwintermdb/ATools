///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#include "stdafx.h"
#include "DialogEditEffects.h"
#include <Object3D.h>
#include "ModelViewer.h"

CDialogEditEffects::CDialogEditEffects(CModelViewer* modelViewer, CObject3D* obj3D, QWidget *parent)
	: QDialog(parent),
	m_modelViewer(modelViewer),
	m_obj3D(obj3D),
	m_oldBlocks(nullptr),
	m_materialBlockCount(0),
	m_editing(nullptr)
{
	ui.setupUi(this);

	ui.spinOpacity->setEnabled(false);
	connect(ui.check2Sides, SIGNAL(toggled(bool)), this, SLOT(Set2Sides(bool)));
	connect(ui.checkHighlight, SIGNAL(toggled(bool)), this, SLOT(SetHighlight(bool)));
	connect(ui.checkOpacity, SIGNAL(toggled(bool)), this, SLOT(SetOpacity(bool)));
	connect(ui.checkReflect, SIGNAL(toggled(bool)), this, SLOT(SetReflect(bool)));
	connect(ui.checkSelfIlluminate, SIGNAL(toggled(bool)), this, SLOT(SetSelfIlluminate(bool)));
	connect(ui.spinOpacity, SIGNAL(valueChanged(int)), this, SLOT(SetAmount(int)));

	for (int i = 0; i < (m_obj3D->m_LOD ? MAX_GROUP : 1); i++)
		for (int j = 0; j < m_obj3D->m_groups[i].objectCount; j++)
			for (int k = 0; k < m_obj3D->m_groups[i].objects[j].materialBlockCount; k++)
				m_materialBlockCount++;

	m_oldBlocks = new MaterialBlock[m_materialBlockCount];

	QTreeWidgetItem* item, *item2, *item3;
	QList<QTreeWidgetItem*> items;
	int id = 0;
	for (int i = 0; i < (m_obj3D->m_LOD ? MAX_GROUP : 1); i++)
	{
		if (m_obj3D->m_LOD)
			item3 = new QTreeWidgetItem(QStringList(QString("LOD %1").arg(i)));

		for (int j = 0; j < m_obj3D->m_groups[i].objectCount; j++)
		{
			item = new QTreeWidgetItem(QStringList(QString("GMObject %1").arg(j + 1)));
			for (int k = 0; k < m_obj3D->m_groups[i].objects[j].materialBlockCount; k++)
			{
				m_oldBlocks[id] = m_obj3D->m_groups[i].objects[j].materialBlocks[k];

				item2 = new QTreeWidgetItem(QStringList(QString(
					QString(m_obj3D->m_groups[i].objects[j].materials[m_obj3D->m_groups[i].objects[j].materialBlocks[k].materialID].textureName) % " %1").arg(
					k + 1)), id + 1);
				item->addChild(item2);

				id++;
			}

			if (m_obj3D->m_LOD)
				item3->addChild(item);
			else
				items.append(item);
		}

		if (m_obj3D->m_LOD)
			items.append(item3);
	}
	ui.tree->insertTopLevelItems(0, items);
	ui.tree->expandAll();

	connect(ui.tree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(CurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
}

CDialogEditEffects::~CDialogEditEffects()
{
	DeleteArray(m_oldBlocks);
}

void CDialogEditEffects::ResetBlocks()
{
	int id = 0;
	for (int i = 0; i < (m_obj3D->m_LOD ? MAX_GROUP : 1); i++)
	{
		for (int j = 0; j < m_obj3D->m_groups[i].objectCount; j++)
		{
			for (int k = 0; k < m_obj3D->m_groups[i].objects[j].materialBlockCount; k++)
			{
				m_obj3D->m_groups[i].objects[j].materialBlocks[k] = m_oldBlocks[id];
				id++;
			}
		}
	}

	if (!m_modelViewer->IsAutoRefresh())
		m_modelViewer->RenderEnvironment();
}

void CDialogEditEffects::CurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
	if (current->type() > 0)
	{
		int id = 1;
		for (int i = 0; i < (m_obj3D->m_LOD ? MAX_GROUP : 1); i++)
		{
			for (int j = 0; j < m_obj3D->m_groups[i].objectCount; j++)
			{
				for (int k = 0; k < m_obj3D->m_groups[i].objects[j].materialBlockCount; k++)
				{
					if (id == current->type())
					{
						m_editing = nullptr;
						MaterialBlock* editing = &m_obj3D->m_groups[i].objects[j].materialBlocks[k];

						ui.checkReflect->setChecked(editing->effect & XE_REFLECT);
						ui.checkHighlight->setChecked(editing->effect & XE_HIGHLIGHT_OBJ);
						ui.checkOpacity->setChecked(editing->effect & XE_OPACITY);
						if (!ui.checkOpacity->isChecked())
						{
							ui.spinOpacity->setValue(255);
							ui.spinOpacity->setEnabled(false);
						}
						else
						{
							ui.spinOpacity->setEnabled(true);
							ui.spinOpacity->setValue(editing->amount);
						}
						ui.check2Sides->setChecked(editing->effect & XE_2SIDE);
						ui.checkSelfIlluminate->setChecked(editing->effect & XE_SELF_ILLUMINATE);
						ui.spinOpacity->setValue(editing->amount);

						m_editing = editing;
						return;
					}
					id++;
				}
			}
		}
	}
}

void CDialogEditEffects::Set2Sides(bool b)
{
	const int effect = XE_2SIDE;
	if (m_editing)
	{
		if (b)
			m_editing->effect |= effect;
		else
			m_editing->effect &= ~effect;
	}

	if (!m_modelViewer->IsAutoRefresh())
		m_modelViewer->RenderEnvironment();
}

void CDialogEditEffects::SetOpacity(bool b)
{
	const int effect = XE_OPACITY;
	if (m_editing)
	{
		if (b)
		{
			ui.spinOpacity->setEnabled(true);

			if (m_editing->amount == 0)
			{
				m_editing->amount = 255;
				ui.spinOpacity->setValue(255);
			}

			m_editing->effect |= effect;
		}
		else
		{
			m_editing->effect &= ~effect;
			ui.spinOpacity->setEnabled(false);
		}
	}

	if (!m_modelViewer->IsAutoRefresh())
		m_modelViewer->RenderEnvironment();
}

void CDialogEditEffects::SetReflect(bool b)
{
	const int effect = XE_REFLECT;
	if (m_editing)
	{
		if (b)
			m_editing->effect |= effect;
		else
			m_editing->effect &= ~effect;
	}

	if (!m_modelViewer->IsAutoRefresh())
		m_modelViewer->RenderEnvironment();
}

void CDialogEditEffects::SetSelfIlluminate(bool b)
{
	const int effect = XE_SELF_ILLUMINATE;
	if (m_editing)
	{
		if (b)
			m_editing->effect |= effect;
		else
			m_editing->effect &= ~effect;
	}

	if (!m_modelViewer->IsAutoRefresh())
		m_modelViewer->RenderEnvironment();
}

void CDialogEditEffects::SetHighlight(bool b)
{
	const int effect = XE_HIGHLIGHT_OBJ;
	if (m_editing)
	{
		if (b)
			m_editing->effect |= effect;
		else
			m_editing->effect &= ~effect;
	}

	if (!m_modelViewer->IsAutoRefresh())
		m_modelViewer->RenderEnvironment();
}

void CDialogEditEffects::SetAmount(int v)
{
	if (m_editing)
		m_editing->amount = v;

	if (!m_modelViewer->IsAutoRefresh())
		m_modelViewer->RenderEnvironment();
}