///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#include "stdafx.h"
#include "MainFrame.h"
#include "WorldEditor.h"
#include <ModelMng.h>
#include <World.h>
#include "Navigator.h"
#include "NewWorld.h"
#include "WorldProperties.h"
#include "ContinentEdit.h"
#include <Skybox.h>

void CMainFrame::HideDialogs()
{
	m_editor->MouseLost();
	if (m_dialogWorldProperties)
		m_dialogWorldProperties->hide();
	if (m_dialogContinentEdit)
		m_dialogContinentEdit->hide();
}

void CMainFrame::ShowDialogs()
{
	if (m_dialogWorldProperties)
		m_dialogWorldProperties->show();
	if (m_dialogContinentEdit)
		m_dialogContinentEdit->show();
}

void CMainFrame::EditWorldProperties()
{
	if (!m_world)
		return;

	if (m_dialogWorldProperties)
		Delete(m_dialogWorldProperties);
	else
	{
		m_dialogWorldProperties = new CDialogWorldProperties(m_world, this);
		connect(m_dialogWorldProperties, SIGNAL(finished(int)), this, SLOT(DialogWorldPropertiesClosed(int)));
		m_dialogWorldProperties->show();
	}
}

void CMainFrame::DialogWorldPropertiesClosed(int result)
{
	m_dialogWorldProperties = null;
	ui.actionPropri_t_s_du_monde->setChecked(false);
}

void CMainFrame::EditContinents()
{
	if (!m_world)
		return;

	if (m_dialogContinentEdit)
	{
		Delete(m_dialogContinentEdit);
		DialogEditContinentsClosed(0);
	}
	else
	{
		m_dialogContinentEdit = new CDialogContinentEdit(m_world, this);
		connect(m_dialogContinentEdit, SIGNAL(finished(int)), this, SLOT(DialogEditContinentsClosed(int)));
		m_dialogContinentEdit->show();
	}
}

void CMainFrame::DialogEditContinentsClosed(int result)
{
	m_dialogContinentEdit = null;
	ui.action_dition_des_continents->setChecked(false);
	if (m_world)
	{
		m_world->m_continent = null;
		if (m_world->m_skybox)
			m_world->m_skybox->LoadTextures();
		m_world->_setLight(false);
		UpdateWorldEditor();
	}
}

void CMainFrame::UpdateContinentVertices()
{
	if (m_dialogContinentEdit)
		m_dialogContinentEdit->UpdateVertexList();
}

void CMainFrame::_openFile(const string& filename)
{
	CloseFile();

	m_world = new CWorld(m_editor->GetDevice());
	if (m_world->Load(filename))
	{
		m_filename = filename;
		setWindowTitle("WorldEditor - " % QFileInfo(filename).fileName());

		m_lastOpenFilenames.removeAll(filename);
		m_lastOpenFilenames.push_front(filename);
		_updateLastOpenFiles();

		QListWidgetItem* item;
		for (auto it = m_world->m_paths.begin(); it != m_world->m_paths.end(); it++)
		{
			item = new QListWidgetItem("Path " % string::number(it.key()), ui.patrolList);
			item->setData(Qt::UserRole + 1, it.key());
			ui.patrolList->addItem(item);
		}
	}
	else
		Delete(m_world);

	m_navigator->SetWorld(m_world);
	m_editor->SetWorld(m_world);
}

void CMainFrame::OpenFile()
{
	HideDialogs();
	const string filename = QFileDialog::getOpenFileName(this, tr("Charger une map"), m_filename.isEmpty() ? "World/" : m_filename, tr("Fichier world (*.wld)"));
	ShowDialogs();

	if (!filename.isEmpty())
		_openFile(filename);
}

void CMainFrame::CloseFile()
{
	Delete(m_world);
	Delete(m_dialogWorldProperties);
	Delete(m_dialogContinentEdit);
	ui.actionPropri_t_s_du_monde->setChecked(false);
	ui.action_dition_des_continents->setChecked(false);
	setWindowTitle("WorldEditor");
	m_filename.clear();
	m_navigator->SetWorld(m_world);
	m_editor->SetWorld(m_world);
	SetStatusBarInfo(D3DXVECTOR3(0, 0, 0), 0, 0, 0, "", "");
	SetLayerInfos(null);
	ui.patrolList->clear();
	m_currentPatrol = -1;
}

void CMainFrame::_updateLastOpenFiles()
{
	ui.menuFichiers_r_cents->clear();
	m_lastOpenFilesActions.clear();

	while (m_lastOpenFilenames.size() > 10)
		m_lastOpenFilenames.pop_back();

	QAction* action;
	for (int i = 0; i < m_lastOpenFilenames.size(); i++)
	{
		action = ui.menuFichiers_r_cents->addAction(string::number(i + 1) % ". " % QFileInfo(m_lastOpenFilenames[i]).fileName());
		m_lastOpenFilesActions[action] = m_lastOpenFilenames[i];
	}
}

void CMainFrame::OpenLastFile(QAction* action)
{
	auto it = m_lastOpenFilesActions.find(action);
	if (it != m_lastOpenFilesActions.end())
		_openFile(it.value());
}

void CMainFrame::NewFile()
{
	HideDialogs();
	CDialogNewWorld dialog(this);
	if (dialog.exec() == QDialog::Accepted)
	{
		CloseFile();
		m_world = new CWorld(m_editor->GetDevice());
		dialog.CreateWorld(m_world);
		setWindowTitle("WorldEditor - " % tr("Nouvelle map"));
		m_navigator->SetWorld(m_world);
		m_editor->SetWorld(m_world);

		ui.action_dition_des_continents->setChecked(false);
		ui.action_dition_des_continents->setChecked(false);
	}
	ShowDialogs();
}

void CMainFrame::SaveBigmap()
{
	if (!m_world)
		return;

	HideDialogs();
	const string filename = QFileDialog::getSaveFileName(this, tr("Enregistrer l'image"), "", tr("Fichier image (*.png *.bmp)"));

	if (!filename.isEmpty() && m_world->m_width * m_world->m_height > 100 && !m_filename.isEmpty())
	{
		if(QMessageBox::question(this, tr("Attention"), tr("Cet map est très grande et son rendu complet va prendre plusieurs minutes.\n" \
			"Afin d'éviter une surconsommation de mémoire, les fichiers vont être chargés et déchargés plusieurs fois.\n" \
			"Le travail non enregistré sera perdu. Souhaitez-vous enregistrer votre travail ?")) == QMessageBox::Yes)
			m_world->Save(m_filename);
	}
	ShowDialogs();

	if (!filename.isEmpty())
		m_editor->SaveBigmap(filename);
}

void CMainFrame::SaveMinimaps()
{
	if (!m_world)
		return;

	if (m_filename.isEmpty())
	{
		HideDialogs();
		m_filename = QFileDialog::getSaveFileName(this, tr("Enregistrer la map"), "World/", tr("Fichier world (*.wld)"));
		ShowDialogs();

		if (!m_filename.isEmpty())
		{
			setWindowTitle("WorldEditor - " % QFileInfo(m_filename).fileName());

			m_lastOpenFilenames.removeAll(m_filename);
			m_lastOpenFilenames.push_front(m_filename);
			_updateLastOpenFiles();

			m_world->Save(m_filename);
		}
	}

	if (!m_filename.isEmpty())
		m_editor->SaveMinimaps();
}

void CMainFrame::SaveFile()
{
	if (!m_world)
		return;

	if (m_filename.isEmpty())
	{
		HideDialogs();
		m_filename = QFileDialog::getSaveFileName(this, tr("Enregistrer la map"), "World/", tr("Fichier world (*.wld)"));
		ShowDialogs();

		if (!m_filename.isEmpty())
		{
			setWindowTitle("WorldEditor - " % QFileInfo(m_filename).fileName());

			m_lastOpenFilenames.removeAll(m_filename);
			m_lastOpenFilenames.push_front(m_filename);
			_updateLastOpenFiles();
		}
	}

	if (!m_filename.isEmpty())
		m_world->Save(m_filename);
}

void CMainFrame::SaveFileAs()
{
	if (!m_world)
		return;

	HideDialogs();
	const string filename = QFileDialog::getSaveFileName(this, tr("Enregistrer la map"), m_filename.isEmpty() ? "World/" : m_filename, tr("Fichier world (*.wld)"));
	ShowDialogs();

	if (!filename.isEmpty())
	{
		m_filename = filename;
		setWindowTitle("WorldEditor - " % QFileInfo(m_filename).fileName());
		m_lastOpenFilenames.removeAll(m_filename);
		m_lastOpenFilenames.push_front(m_filename);
		_updateLastOpenFiles();

		m_world->Save(m_filename);
	}
}

void CMainFrame::dragEnterEvent(QDragEnterEvent* event)
{
	m_dragFilename.clear();

	const QMimeData* mimeData = event->mimeData();

	if (mimeData->hasUrls())
	{
		QStringList pathList;
		QList<QUrl> urlList = mimeData->urls();

		for (int i = 0; i < urlList.size() && i < 32; ++i)
			pathList.append(urlList.at(i).toLocalFile());

		if (pathList.size() > 0)
		{
			for (int i = 0; i < pathList.size(); i++)
			{
				if (GetExtension(pathList[i]) == "wld")
				{
					m_dragFilename = pathList[i];
					event->acceptProposedAction();
					return;
				}
			}

		}
	}

	m_dragFilename.clear();
}

void CMainFrame::dragLeaveEvent(QDragLeaveEvent* event)
{
	event->accept();
}

void CMainFrame::dragMoveEvent(QDragMoveEvent* event)
{
	if (m_dragFilename.size() > 0)
		event->acceptProposedAction();
}

void CMainFrame::dropEvent(QDropEvent* event)
{
	if (m_dragFilename.size() > 0)
		_openFile(m_dragFilename);
}

void CMainFrame::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::WindowStateChange && !isMinimized())
	{
		if (m_editor && !m_editor->IsAutoRefresh())
			m_editor->RenderEnvironment();
	}

	QMainWindow::changeEvent(event);
}