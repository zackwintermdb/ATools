///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#include "stdafx.h"
#include "MainFrame.h"
#include "ModelViewer.h"
#include <ModelMng.h>
#include <AnimatedMesh.h>
#include <TextureMng.h>
#include <Timeline.h>
#include <Motion.h>
#include <Object3D.h>
#include "Importer.h"
#include "Exporter.h"
#include "DialogEditEffects.h"

CMainFrame::CMainFrame(QWidget *parent)
	: QMainWindow(parent),
	m_mesh(null),
	m_motionList(null),
	m_timeline(null),
	m_inEnglish(false),
	m_languageActionGroup(null)
{
	ui.setupUi(this);

	m_status = new QLabel(tr("Prêt"));
	m_status->setStyleSheet("color: white;");
	ui.statusBar->addWidget(m_status);
	qApp->setStyleSheet("QStatusBar::item { border: none; }");

	ui.menuFen_tres->addAction(ui.dockAnimations->toggleViewAction());
	ui.menuFen_tres->addAction(ui.dockTimeline->toggleViewAction());

	m_actionsLOD = new QActionGroup(this);
	m_actionsLOD->addAction(ui.actionLOD_0);
	m_actionsLOD->addAction(ui.actionLOD_1);
	m_actionsLOD->addAction(ui.actionLOD_2);

	m_timeline = new CTimeline(this);
	ui.dockTimeline->setWidget(m_timeline);
	m_timeline->AddRow(tr("Frappe/Tir"), Qt::red);
	m_timeline->AddRow(tr("Son"), Qt::blue);
	m_timeline->AddRow(tr("Tremblement"), Qt::green);

	m_modelViewer = new CModelViewer(this);
	setCentralWidget(m_modelViewer);

	m_motionList = new QStringListModel(ui.motionList);
	ui.motionList->setModel(m_motionList);

	if (!m_modelViewer->CreateEnvironment())
	{
		Delete(m_modelViewer);
		qFatal("Loading failed !");
	}

	Assimp::Importer importer;
	aiString extList;
	importer.GetExtensionList(extList);
	m_supportedImportFiles = "*.o3d *.dae " % string(extList.C_Str());
	m_supportedImportFiles.replace(';', ' ');

	m_languageActionGroup = new QActionGroup(ui.menuLangage);
	m_languageActionGroup->addAction(ui.actionFran_ais);
	m_languageActionGroup->addAction(ui.actionEnglish);

	_connectWidgets();

	CloseFile();
	_loadSettings();
}

CMainFrame::~CMainFrame()
{
	CloseFile();
	Delete(m_status);
	Delete(m_modelViewer);
	Delete(m_motionList);
	Delete(m_languageActionGroup);
}

void CMainFrame::_connectWidgets()
{
	connect(m_actionsLOD, SIGNAL(triggered(QAction*)), this, SLOT(ChangeLOD(QAction*)));
	connect(ui.action_propos, SIGNAL(triggered()), this, SLOT(About()));
	connect(ui.actionQt, SIGNAL(triggered()), this, SLOT(AboutQt()));
	connect(ui.actionGrille, SIGNAL(triggered(bool)), this, SLOT(ShowGrid(bool)));
	connect(ui.actionObjet_de_collision, SIGNAL(triggered(bool)), this, SLOT(ShowCollObj(bool)));
	connect(ui.actionTextures_additionnelles, SIGNAL(triggered()), this, SLOT(LoadTextureEx()));
	connect(ui.actionJouer, SIGNAL(triggered(bool)), this, SLOT(Play(bool)));
	connect(ui.motionList, SIGNAL(activated(const QModelIndex&)), this, SLOT(PlayMotion(const QModelIndex&)));
	connect(ui.actionStop, SIGNAL(triggered()), this, SLOT(Stop()));
	connect(ui.actionFermer, SIGNAL(triggered()), this, SLOT(CloseFile()));
	connect(ui.actionEnregistrer, SIGNAL(triggered()), this, SLOT(SaveFile()));
	connect(ui.actionOuvrir, SIGNAL(triggered()), this, SLOT(OpenFile()));
	connect(ui.action_diter_les_effets, SIGNAL(triggered()), this, SLOT(EditEffects()));
	connect(ui.actionGuide_d_import, SIGNAL(triggered()), this, SLOT(ImportGuide()));
	connect(ui.actionPlein_cran, SIGNAL(triggered(bool)), this, SLOT(SwitchFullscreen(bool)));
	connect(ui.actionCouleur_du_fond, SIGNAL(triggered()), this, SLOT(SetBackgroundColor()));
	connect(m_timeline, SIGNAL(CurrentFrameChanged(int)), m_modelViewer, SLOT(SetFrame(int)));
	connect(m_timeline, SIGNAL(KeyModified(int, int, bool)), this, SLOT(MotionAttributeModified(int, int, bool)));
	connect(m_languageActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(SetLanguage(QAction*)));
}

void CMainFrame::CloseFile()
{
	if (m_mesh)
	{
		for (int i = 0; i < MAX_ANIMATED_ELEMENTS; i++)
			Delete(m_mesh->m_elements[i].obj);
		Delete(m_mesh->m_motion);
		Delete(m_mesh->m_skeleton);
	}

	Delete(m_mesh);
	setWindowTitle(tr("ModelEditor"));
	m_motionList->setStringList(QStringList());
	m_modelViewer->SetMesh(null);
	m_modelViewer->SetAutoRefresh(false);
	m_timeline->SetFrameCount(0);
	ui.actionJouer->setChecked(false);
	m_assimpImport = false;
	m_meshSex = SEX_SEXLESS;

	if (!m_modelViewer->IsAutoRefresh())
		m_modelViewer->RenderEnvironment();
}

void CMainFrame::SetBackgroundColor()
{
	const QColor color = QColorDialog::getColor(g_global3D.backgroundColor, this, tr("Couleur du fond"));
	if (color.isValid())
	{
		g_global3D.backgroundColor = D3DCOLOR_ARGB(255, color.red(), color.green(), color.blue());
		if (!m_modelViewer->IsAutoRefresh())
			m_modelViewer->RenderEnvironment();
	}
}

void CMainFrame::_loadSettings()
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ATools", "ModelEditor");
	if (settings.status() == QSettings::NoError)
	{
		if (settings.contains("BackgroundColor"))
			g_global3D.backgroundColor = settings.value("BackgroundColor").toUInt();
		if (settings.contains("ShowGrid"))
		{
			g_global3D.grid = settings.value("ShowGrid").toBool();
			ui.actionGrille->setChecked(g_global3D.grid);
		}
		if (settings.contains("ShowCollObj"))
		{
			g_global3D.renderCollisions = settings.value("ShowCollObj").toBool();
			ui.actionObjet_de_collision->setChecked(g_global3D.renderCollisions);
		}
		if (settings.contains("WindowGeometry"))
		{
			restoreGeometry(settings.value("WindowGeometry").toByteArray());
			if (isFullScreen())
				ui.actionPlein_cran->setChecked(true);
		}
		if (settings.contains("WindowState"))
			restoreState(settings.value("WindowState").toByteArray());
		if (settings.contains("Language") && settings.value("Language") == "English")
		{
			ui.actionEnglish->setChecked(true);
			SetLanguage(ui.actionEnglish);
		}

		if (m_modelViewer && !m_modelViewer->IsAutoRefresh())
			m_modelViewer->RenderEnvironment();
	}
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
		m_translator.load("modeleditor_en.qm", "platforms/English");
	}

	qApp->installTranslator(&m_translator);
	ui.retranslateUi(this);
	m_timeline->RenameRow(0, tr("Frappe/Tir"));
	m_timeline->RenameRow(1, tr("Son"));
	m_timeline->RenameRow(2, tr("Tremblement"));
	m_status->setText(tr("Prêt"));

	if (m_filename.isEmpty() || !m_mesh)
		setWindowTitle(tr("ModelEditor"));
	else
		setWindowTitle(tr("ModelEditor - ") % QFileInfo(m_filename).fileName());
}

void CMainFrame::closeEvent(QCloseEvent* event)
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ATools", "ModelEditor");
	if (settings.isWritable())
	{
		settings.setValue("BackgroundColor", (uint)g_global3D.backgroundColor);
		settings.setValue("ShowGrid", g_global3D.grid);
		settings.setValue("ShowCollObj", g_global3D.renderCollisions);
		settings.setValue("WindowGeometry", saveGeometry());
		settings.setValue("WindowState", saveState());
		settings.setValue("Language", m_inEnglish ? "English" : "French");
	}

	QMainWindow::closeEvent(event);
}

void CMainFrame::_openFile(const string& filename)
{
	bool resetView = true;

	const string filenameToLower = QFileInfo(filename).fileName().toLower();
	const string dir = QFileInfo(filename).path() % '/';
	TextureMng->SetModelTexturePath(dir);

	if (GetExtension(filenameToLower) == "o3d")
	{
		ModelMng->SetModelPath(dir);
		bool isPart = true;
		if (!((filenameToLower.startsWith("part_m") && m_meshSex == SEX_MALE)
			|| (filenameToLower.startsWith("part_f") && m_meshSex == SEX_FEMALE)))
		{
			CloseFile();
			m_mesh = new CAnimatedMesh(m_modelViewer->GetDevice());

			if (filenameToLower.startsWith("part_m"))
				m_meshSex = SEX_MALE;
			else if (filenameToLower.startsWith("part_f"))
				m_meshSex = SEX_FEMALE;
			else
			{
				isPart = false;
				m_meshSex = SEX_SEXLESS;
			}
		}
		else
			resetView = false;

		int part = 0;
		if (isPart)
			part = ModelMng->GetModelPart(filenameToLower);

		if (m_mesh->m_elements[part].obj)
			Delete(m_mesh->m_elements[part].obj);

		if (!m_mesh->Load(filenameToLower, part))
		{
			if (!isPart)
			{
				for (int i = 0; i < MAX_ANIMATED_ELEMENTS; i++)
					Delete(m_mesh->m_elements[i].obj);
				Delete(m_mesh->m_motion);
				Delete(m_mesh->m_skeleton);
				Delete(m_mesh);
			}
		}

		if (m_mesh && (filenameToLower.startsWith("part") || filenameToLower.startsWith("mvr")) && m_motionList->rowCount() == 0)
		{
			QStringList list;
			const QDir fileDir(dir);
			const QStringList files = fileDir.entryList(QDir::Files, QDir::Name);

			QString req;

			if (m_meshSex == SEX_MALE)
				req = "mvr_male_";
			else if (m_meshSex == SEX_FEMALE)
				req = "mvr_female_";
			else
				req = QFileInfo(filenameToLower).baseName().section('.', 0) % '_';

			for (int i = 0; i < files.size(); i++)
			{
				const QString name = files[i].toLower();

				if (name.startsWith(req) && name.endsWith("ani"))
				{
					const QString temp = files[i].right(name.size() - req.size());
					list.push_back(temp.left(temp.size() - 4));
				}
			}

			m_motionList->setStringList(list);
		}
	}
	else
	{
		CloseFile();
		m_mesh = new CAnimatedMesh(m_modelViewer->GetDevice());

		CImporter importer(m_mesh);
		if (!importer.Import(filename))
			Delete(m_mesh);
		else
			m_assimpImport = true;
	}

	if (m_mesh)
	{
		if (m_mesh->GetFrameCount() > 0)
		{
			ui.actionJouer->setChecked(true);
			m_modelViewer->SetAutoRefresh(true);

			const int frameCount = m_mesh->GetFrameCount();
			m_timeline->SetFrameCount(frameCount);
			if (frameCount > 0)
			{
				const MotionAttribute* attributes = m_mesh->GetAttributes();
				if (attributes)
				{
					for (int i = 0; i < frameCount; i++)
					{
						if ((int)(attributes[i].frame + 0.5f) == i)
						{
							if (attributes[i].type & MA_HIT)
								m_timeline->AddKey(0, i);
							else if (attributes[i].type & MA_SOUND)
								m_timeline->AddKey(1, i);
							else if (attributes[i].type & MA_QUAKE)
								m_timeline->AddKey(2, i);
						}
					}
				}
			}
		}

		if (resetView)
			m_modelViewer->SetMesh(m_mesh);
		else if (!m_modelViewer->IsAutoRefresh())
			m_modelViewer->RenderEnvironment();

		const string filenameNotLower = QFileInfo(filename).fileName();
		setWindowTitle(tr("ModelEditor - ") % filenameNotLower);
		m_filename = filenameNotLower;
		m_motionName = m_filename.replace("dae", "ani");
	}

	m_status->setText(tr("Prêt"));
}

void CMainFrame::_saveFile(const string& filename)
{
	if (!m_mesh)
		return;

	bool result = false;
	if (GetExtension(filename) == "dae")
	{
		result = CDAEExporter(m_mesh).Export(filename);
	}
	else if (GetExtension(filename) == "ani" && m_mesh->m_motion)
	{
		result = m_mesh->m_motion->Save(filename);
	}
	else if (GetExtension(filename) == "o3d")
	{
		for (int i = 0; i < MAX_ANIMATED_ELEMENTS; i++)
		{
			if (m_mesh->m_elements[i].obj)
			{
				result = m_mesh->m_elements[i].obj->Save(filename);
				break;
			}
		}

		if (!QFileInfo(filename).fileName().toLower().startsWith("part"))
		{
			if (m_mesh->m_skeleton && result)
			{
				string skelName = filename;
				skelName.remove(skelName.size() - 4, 4);
				skelName += ".chr";
				result = m_mesh->m_skeleton->Save(skelName);
			}
		}
	}

	if (result)
		m_status->setText(tr("Fichier enregistré avec succès"));
	else
	{
		qCritical("There was a problem while saving the file");
		m_status->setText(tr("Prêt"));
	}
}

void CMainFrame::OpenFile()
{
	string dir = ModelMng->GetModelPath();
	if (!m_filename.isEmpty())
		dir += m_filename;

	const QString filename = QFileDialog::getOpenFileName(this, tr("Charger un model"), dir, tr("Fichiers 3D (") % m_supportedImportFiles % ")");

	if (!filename.isEmpty())
		_openFile(filename);
}

void CMainFrame::SaveFile()
{
	if (!m_mesh)
		return;

	string filename;

	if (m_mesh->m_motion)
		filename = QFileDialog::getSaveFileName(this, tr("Enregistrer l'animation/le model"), ModelMng->GetModelPath() % m_motionName, tr("Fichier d'animation (*.ani);; Fichier 3D (*.o3d *.dae)"));
	else
		filename = QFileDialog::getSaveFileName(this, tr("Enregistrer le model"), ModelMng->GetModelPath() % m_filename.remove(".o3d").remove(".ani"), tr("Fichier 3D (*.o3d *.dae)"));

	if (!filename.isEmpty())
		_saveFile(filename);
}

void CMainFrame::EditEffects()
{
	if (m_mesh)
	{
		for (int i = 0; i < MAX_ANIMATED_ELEMENTS; i++)
		{
			if (m_mesh->m_elements[i].obj)
			{
				CDialogEditEffects dialog(m_modelViewer, m_mesh->m_elements[i].obj, this);
				if (dialog.exec() != QDialog::Accepted)
					dialog.ResetBlocks();
				return;
			}
		}
	}
}

void CMainFrame::About()
{
	QMessageBox::about(this, tr("À propos"),
		"ATools v" % string::number(VERSION) % '.' % string::number(SUB_VERSION) % tr("\n\nPar Aishiro"));
}

void CMainFrame::AboutQt()
{
	QMessageBox::aboutQt(this, tr("À propos de Qt"));
}

void CMainFrame::ShowGrid(bool show)
{
	g_global3D.grid = show;

	if (!m_modelViewer->IsAutoRefresh())
		m_modelViewer->RenderEnvironment();
}

void CMainFrame::ShowCollObj(bool show)
{
	g_global3D.renderCollisions = show;

	if (!m_modelViewer->IsAutoRefresh())
		m_modelViewer->RenderEnvironment();
}

void CMainFrame::LoadTextureEx()
{
	if (!m_mesh)
		return;

	bool ok = false;
	int id = QInputDialog::getInt(nullptr, tr("Textures additionnelles"), tr("N° des textures :"), m_mesh->GetTextureEx(), 0, 7, 1, &ok);

	if (ok)
	{
		m_mesh->SetTextureEx(id);

		if (!m_modelViewer->IsAutoRefresh())
			m_modelViewer->RenderEnvironment();
	}
}

void CMainFrame::ChangeLOD(QAction* action)
{
	if (action == ui.actionLOD_0)
		m_modelViewer->SetLOD(0);
	else if (action == ui.actionLOD_1)
		m_modelViewer->SetLOD(1);
	else if (action == ui.actionLOD_2)
		m_modelViewer->SetLOD(2);
}

void CMainFrame::MotionAttributeModified(int row, int frame, bool removed)
{
	const int frameCount = m_mesh->GetFrameCount();
	if (frameCount > 0)
	{
		MotionAttribute* attributes = m_mesh->GetAttributes();
		if (attributes && frame < frameCount)
		{
			int type = 0;
			switch (row)
			{
			case 0:
				type = MA_HIT;
				break;
			case 1:
				type = MA_SOUND;
				break;
			case 2:
				type = MA_QUAKE;
				break;
			default:
				return;
			}

			if (removed)
			{
				attributes[frame].type &= ~type;

				if (attributes[frame].type == 0)
					attributes[frame].frame = 0.0f;
			}
			else
			{
				if (type == MA_SOUND)
				{
					bool ok = false;
					int id = QInputDialog::getInt(this, tr("ID du son"), tr("ID du son :"), attributes[frame].soundID, 0, 2147483647, 1, &ok);

					if (!ok)
					{
						m_timeline->RemoveKey(row, frame);
						return;
					}

					attributes[frame].soundID = id;
				}

				attributes[frame].type |= type;
				attributes[frame].frame = (float)frame;
			}
		}
	}
}

void CMainFrame::PlayMotion(const QModelIndex & index)
{
	const QString motion = m_motionList->stringList().at(index.row());

	if (m_mesh)
	{
		string filename;
		if (m_meshSex == SEX_MALE)
			filename = "mvr_male";
		else if (m_meshSex == SEX_FEMALE)
			filename = "mvr_female";
		else
			filename = QFileInfo(m_filename).baseName();

		m_motionName = filename % '_' % motion % ".ani";
		Delete(m_mesh->m_motion);
		m_mesh->SetMotion(m_motionName);

		const int frameCount = m_mesh->GetFrameCount();
		m_timeline->SetFrameCount(frameCount);
		m_timeline->RemoveAllKeys();
		if (frameCount > 0)
		{
			const MotionAttribute* attributes = m_mesh->GetAttributes();
			if (attributes)
			{
				for (int i = 0; i < frameCount; i++)
				{
					if ((int)(attributes[i].frame + 0.5f) == i)
					{
						if (attributes[i].type & MA_HIT)
							m_timeline->AddKey(0, i);
						else if (attributes[i].type & MA_SOUND)
							m_timeline->AddKey(1, i);
						else if (attributes[i].type & MA_QUAKE)
							m_timeline->AddKey(2, i);
					}
				}
			}
		}

		m_modelViewer->SetAutoRefresh(true);
		ui.actionJouer->setChecked(true);
	}
}

void CMainFrame::Play(bool play)
{
	if (play)
	{
		if (!m_mesh || m_mesh->GetFrameCount() == 0)
			ui.actionJouer->setChecked(false);
		else
			m_modelViewer->SetAutoRefresh(true);
	}
	else
		m_modelViewer->SetAutoRefresh(false);

	m_modelViewer->RenderEnvironment();
}

void CMainFrame::Stop()
{
	ui.actionJouer->setChecked(false);
	m_modelViewer->SetAutoRefresh(false);
	ui.motionList->clearSelection();

	if (m_mesh)
	{
		m_modelViewer->SetFrame(0);
		m_timeline->SetCurrentFrame(0);

		if (!m_assimpImport && m_mesh->m_motion)
		{
			Delete(m_mesh->m_motion);
			m_mesh->m_frameCount = 0;
			m_mesh->m_currentFrame = 0.0f;
			if (m_mesh->m_skeleton)
				m_mesh->m_skeleton->ResetBones(m_mesh->m_bones, m_mesh->m_invBones);
			m_timeline->SetFrameCount(0);
		}
	}
	else
	{
		m_timeline->SetFrameCount(0);
	}

	m_modelViewer->RenderEnvironment();
}

void CMainFrame::ImportGuide()
{
	const string guide = tr("<span style=\"color: red;\">Evitez les espaces et caractères spéciaux (accents...) dans les noms des os et des meshs.</span><br /><br />" \
		"- Nom du mesh qui fini par \"coll\" : objet de collision <span style=\"color: red;\">(1 seul par model)</span><br />" \
		"- Nom du mesh qui fini par \"light\" : lampe (visible uniquement la nuit)<br />" \
		"- Nom du mesh qui commence par \"LOD1\" : appartient au 2<sup>ème</sup> niveau de détails<br />" \
		"- Nom du mesh qui commence par \"LOD2\" : appartient au 3<sup>ème</sup> niveau de détails<br /><br />" \
		"<span style=\"color: green;\">Utilisez de préférence le format COLLADA (.dae) avec 3DS Max pour créer des models.</span>");

	QMessageBox::information(this, tr("Guide d'import"), guide);
}

void CMainFrame::SwitchFullscreen(bool fullscreen)
{
	if (fullscreen)
		showFullScreen();
	else
		showNormal();
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
				if (m_supportedImportFiles.contains(GetExtension(pathList[i])))
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
		if (m_modelViewer && !m_modelViewer->IsAutoRefresh())
			m_modelViewer->RenderEnvironment();
	}

	QMainWindow::changeEvent(event);
}