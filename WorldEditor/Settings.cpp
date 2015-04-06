///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#include "stdafx.h"
#include "MainFrame.h"
#include <Project.h>
#include <GameElements.h>
#include "WorldEditor.h"
#include <ModelMng.h>

void CMainFrame::ShowFavoritesMenu(const QPoint& pt)
{
	QStandardItemModel* model = (QStandardItemModel*)ui.gameElementsTree->model();
	QModelIndex index = ui.gameElementsTree->indexAt(pt);
	if (index.isValid())
	{
		QStandardItem* element = (QStandardItem*)model->itemFromIndex(index);
		if (element && (element->type() == GAMEELE_TERRAIN || element->type() == GAMEELE_MODEL))
		{
			if (element->parent() == m_favoritesFolder)
			{
				if (m_favoritesRemoveMenu->exec(ui.gameElementsTree->mapToGlobal(pt)))
					m_favoritesFolder->takeRow(element->row());
			}
			else
			{
				if (m_favoritesAddMenu->exec(ui.gameElementsTree->mapToGlobal(pt)))
				{
					QStandardItem* newItem;
					switch (element->type())
					{
					case GAMEELE_TERRAIN:
						newItem = new CTerrainElement();
						break;
					case GAMEELE_MODEL:
						newItem = new CModelElement();
						break;
					}
					newItem->setData(element->data());
					newItem->setIcon(element->icon());
					newItem->setText(element->text());
					m_favoritesFolder->insertRow(0, newItem);
					ui.gameElementsTree->expand(m_favoritesFolder->index());
				}
			}
		}
	}
}

void CMainFrame::_loadSettings()
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ATools", "WorldEditor");
	if (settings.status() == QSettings::NoError)
	{
		// General
		if (settings.contains("WindowGeometry"))
		{
			restoreGeometry(settings.value("WindowGeometry").toByteArray());
			if (isFullScreen())
				ui.actionPlein_cran->setChecked(true);
		}
		if (settings.contains("WindowState"))
			restoreState(settings.value("WindowState").toByteArray());
		if (settings.contains("LastOpenFiles"))
		{
			m_lastOpenFilenames = settings.value("LastOpenFiles").toStringList();
			_updateLastOpenFiles();
		}
		if (settings.contains("Favorites"))
		{
			QByteArray favoritesBuffer = settings.value("Favorites").toByteArray();
			if (favoritesBuffer.size() > 4)
			{
				QDataStream favoritesData(favoritesBuffer);
				QStandardItem* item;
				int favoritesCount;
				favoritesData >> favoritesCount;
				for (int i = 0; i < favoritesCount; i++)
				{
					item = m_prj->CreateFavorite(favoritesData);
					if (item)
						m_favoritesFolder->insertRow(0, item);
				}
				ui.gameElementsTree->expand(m_favoritesFolder->index());
			}
		}
		if (settings.contains("Language") && settings.value("Language") == "English")
		{
			ui.actionEnglish->setChecked(true);
			SetLanguage(ui.actionEnglish);
		}

		// Edition
		if (settings.contains("EditMode"))
		{
			m_editMode = (EEditMode)settings.value("EditMode").toInt();
			switch (m_editMode)
			{
			case EDIT_TERRAIN_HEIGHT:
				ui.tabTerrain->setCurrentIndex(0);
				break;
			case EDIT_TERRAIN_TEXTURE:
				ui.tabTerrain->setCurrentIndex(1);
				break;
			case EDIT_TERRAIN_COLOR:
				ui.tabTerrain->setCurrentIndex(2);
				break;
			case EDIT_TERRAIN_WATER:
				ui.tabTerrain->setCurrentIndex(3);
				break;
			case EDIT_CONTINENT:
				ui.actionVertices_du_continent->setChecked(true);
				break;
			case EDIT_ADD_OBJECTS:
				ui.actionAjouter_des_objets->setChecked(true);
				break;
			case EDIT_SELECT_OBJECTS:
				ui.actionS_lectionner_des_objets->setChecked(true);
				break;
			case EDIT_MOVE_OBJECTS:
				ui.actionD_placer_les_objets->setChecked(true);
				break;
			case EDIT_ROTATE_OBJECTS:
				ui.actionTourner_les_objets->setChecked(true);
				break;
			case EDIT_SCALE_OBJECTS:
				ui.actionRedimensionner_les_objets->setChecked(true);
				break;
			}
		}
		if (settings.contains("EditAxis"))
		{
			m_editAxis = (EEditAxis)settings.value("EditAxis").toInt();
			switch (m_editAxis)
			{
			case EDIT_X:
				ui.actionEditX->setChecked(true);
				break;
			case EDIT_Y:
				ui.actionEditY->setChecked(true);
				break;
			case EDIT_Z:
				ui.actionEditZ->setChecked(true);
				break;
			case EDIT_XZ:
				ui.actionEditXZ->setChecked(true);
				break;
			}
		}

		// View
		if (settings.contains("GridSize"))
			m_gridSize = settings.value("GridSize").toInt();
		if (settings.contains("GameTime"))
			g_global3D.hour = settings.value("GameTime").toInt();
		if (settings.contains("MsgBox"))
			ui.actionBo_te_de_dialogue_pour_les_erreurs->setChecked(settings.value("MsgBox").toBool());

		if (m_editor && !m_editor->IsAutoRefresh())
			m_editor->RenderEnvironment();
	}
}

void CMainFrame::closeEvent(QCloseEvent* event)
{
	if (m_world)
	{
		QMessageBox::Button result = QMessageBox::question(this, tr("Quitter"), tr("Êtes vous sûr de vouloir quitter ?"));
		if (result != QMessageBox::Yes)
		{
			event->ignore();
			return;
		}
	}

	QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ATools", "WorldEditor");
	if (settings.isWritable())
	{
		// General
		settings.setValue("WindowGeometry", saveGeometry());
		settings.setValue("WindowState", saveState());
		settings.setValue("LastOpenFiles", m_lastOpenFilenames);
		const int favoritesCount = m_favoritesFolder->rowCount();
		QByteArray favoritesBuffer;
		QDataStream favoritesData(&favoritesBuffer, QIODevice::WriteOnly);
		QStandardItem* child;
		favoritesData << favoritesCount;
		for (int i = favoritesCount - 1; i >= 0; i--)
		{
			child = m_favoritesFolder->child(i);
			favoritesData << child->type();
			if (child->type() == GAMEELE_TERRAIN)
				favoritesData << ((CTerrainElement*)child)->GetTerrain();
			else if (child->type() == GAMEELE_MODEL)
			{
				favoritesData << ((CModelElement*)child)->GetModel()->type;
				favoritesData << ((CModelElement*)child)->GetModel()->ID;
			}
		}
		settings.setValue("Favorites", favoritesBuffer);
		settings.setValue("Language", m_inEnglish ? "English" : "French");

		// Edition
		settings.setValue("EditMode", (int)m_editMode);
		settings.setValue("EditAxis", (int)m_editAxis);

		// View
		settings.setValue("GridSize", m_gridSize);
		settings.setValue("GameTime", g_global3D.hour);
		settings.setValue("MsgBox", ui.actionBo_te_de_dialogue_pour_les_erreurs->isChecked());
	}

	QMainWindow::closeEvent(event);
}

void CMainFrame::SetLanguage(QAction* action)
{
	if (action == ui.actionFran_ais)
	{
		m_inEnglish = false;
		m_translator.load("", "");
	}
	else if (action == ui.actionEnglish)
	{
		m_inEnglish = true;
		m_translator.load("worldeditor_en.qm", "platforms/English");
	}

	qApp->installTranslator(&m_translator);
	ui.retranslateUi(this);

	if (m_filename.isEmpty() || !m_world)
		setWindowTitle(tr("WorldEditor"));
	else
		setWindowTitle(tr("WorldEditor - ") % QFileInfo(m_filename).fileName());

	m_objPropertiesAction->setText(tr("Propriétés"));
	m_favoritesAddMenu->actions().at(0)->setText(tr("Ajouter aux favoris"));
	m_favoritesRemoveMenu->actions().at(0)->setText(tr("Retirer des favoris"));
	QStandardItem* root = ((QStandardItemModel*)ui.gameElementsTree->model())->invisibleRootItem();
	root->child(0)->setText(QObject::tr("Favoris"));
	root->child(1)->setText(QObject::tr("Objet"));
	root->child(2)->setText(QObject::tr("Terrain"));
	SetStatusBarInfo(D3DXVECTOR3(0, 0, 0), 0, 0, 0, "", "");
}

bool CMainFrame::MsgBoxForErrors()
{
	return ui.actionBo_te_de_dialogue_pour_les_erreurs->isChecked();
}