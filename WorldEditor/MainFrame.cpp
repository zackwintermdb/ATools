///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#include "stdafx.h"
#include "MainFrame.h"
#include "WorldEditor.h"
#include "Navigator.h"
#include <Project.h>
#include <GameElements.h>
#include <ModelMng.h>
#include <Object.h>

CMainFrame* MainFrame = null;

CMainFrame::CMainFrame(QWidget *parent)
	: QMainWindow(parent),
	m_prj(null),
	m_editor(null),
	m_world(null),
	m_navigator(null),
	m_favoritesAddMenu(null),
	m_favoritesRemoveMenu(null),
	m_favoritesFolder(null),
	m_curX(null),
	m_curY(null),
	m_curZ(null),
	m_curLand(null),
	m_curLayerCount(null),
	m_curObject(null),
	m_curTexture(null),
	m_waterHeight(null),
	m_editTerrainTextureColor(127, 127, 127),
	m_actionEditMode(null),
	m_editMode(EDIT_TERRAIN_HEIGHT),
	m_editTexture(-1),
	m_dialogWorldProperties(null),
	m_dialogContinentEdit(null),
	m_addObject(null),
	m_editAxis(EDIT_XZ),
	m_gridSize(1),
	m_objectMenu(null),
	m_noObjectMenu(null),
	m_inEnglish(false),
	m_languageActionGroup(null),
	m_currentPatrol(-1)
{
	MainFrame = this;

	m_prj = new CProject();
	if (!m_prj->Load("Masquerade.prj"))
	{
		Delete(m_prj);
		qFatal("Can't load project !");
	}

	ui.setupUi(this);

	m_curX = new QLabel();
	m_curX->setMinimumWidth(80);
	m_curX->setStyleSheet("color: white;");
	ui.statusbar->addWidget(m_curX);
	m_curY = new QLabel();
	m_curY->setMinimumWidth(80);
	m_curY->setStyleSheet("color: white;");
	ui.statusbar->addWidget(m_curY);
	m_curZ = new QLabel();
	m_curZ->setMinimumWidth(80);
	m_curZ->setStyleSheet("color: white;");
	ui.statusbar->addWidget(m_curZ);
	m_curLand = new QLabel();
	m_curLand->setMinimumWidth(80);
	m_curLand->setStyleSheet("color: white;");
	ui.statusbar->addWidget(m_curLand);
	m_curLayerCount = new QLabel();
	m_curLayerCount->setMinimumWidth(140);
	m_curLayerCount->setStyleSheet("color: white;");
	ui.statusbar->addWidget(m_curLayerCount);
	m_curObject = new QLabel();
	m_curObject->setMinimumWidth(230);
	m_curObject->setStyleSheet("color: white;");
	ui.statusbar->addWidget(m_curObject);
	m_curTexture = new QLabel();
	m_curTexture->setMinimumWidth(230);
	m_curTexture->setStyleSheet("color: white;");
	ui.statusbar->addWidget(m_curTexture);
	m_waterHeight = new QLabel();
	m_waterHeight->setMinimumWidth(120);
	m_waterHeight->setStyleSheet("color: white;");
	ui.statusbar->addWidget(m_waterHeight);

	m_editor = new CWorldEditor(this);
	setCentralWidget(m_editor);

	if (!m_editor->CreateEnvironment())
	{
		Delete(m_editor);
		qFatal("Loading failed !");
	}

	QIcon iconAdd;
	iconAdd.addFile(":/MainFrame/Resources/add.png");
	QIcon iconRemove;
	iconRemove.addFile(":/MainFrame/Resources/remove.png");
	QIcon iconFavorites;
	iconFavorites.addFile(":/MainFrame/Resources/icon_favorites.png");
	m_prj->FillTreeView(ui.gameElementsTree);
	QStandardItemModel* model = (QStandardItemModel*)ui.gameElementsTree->model();
	m_favoritesFolder = new CRootFolderElement();
	m_favoritesFolder->setText(QObject::tr("Favoris"));
	m_favoritesFolder->setIcon(iconFavorites);
	model->invisibleRootItem()->insertRow(0, m_favoritesFolder);
	m_favoritesAddMenu = new QMenu(this);
	m_favoritesAddMenu->addAction(iconAdd, tr("Ajouter aux favoris"));
	m_favoritesRemoveMenu = new QMenu(this);
	m_favoritesRemoveMenu->addAction(iconRemove, tr("Retirer des favoris"));

	m_prj->FillWaterComboBox(ui.editWaterComboBox);

	m_navigator = new CNavigator(ui.dockNavigator);
	ui.dockNavigator->setWidget(m_navigator);

	ui.menuFen_tres->addAction(ui.dockNavigator->toggleViewAction());
	ui.menuFen_tres->addAction(ui.dockGameElements->toggleViewAction());
	ui.menuFen_tres->addAction(ui.dockTerrainEdit->toggleViewAction());
	ui.menuFen_tres->addAction(ui.dockPatrolEditor->toggleViewAction());

	m_actionEditMode = new QActionGroup(this);
	m_actionEditMode->addAction(ui.actionEau_et_nuages_edit);
	m_actionEditMode->addAction(ui.actionHauteur_du_terrain);
	m_actionEditMode->addAction(ui.actionCouleur_du_terrain);
	m_actionEditMode->addAction(ui.actionTexture_du_terrain);
	m_actionEditMode->addAction(ui.actionVertices_du_continent);
	m_actionEditMode->addAction(ui.actionAjouter_des_objets);
	m_actionEditMode->addAction(ui.actionS_lectionner_des_objets);
	m_actionEditMode->addAction(ui.actionD_placer_les_objets);
	m_actionEditMode->addAction(ui.actionTourner_les_objets);
	m_actionEditMode->addAction(ui.actionRedimensionner_les_objets);

	m_actionEditAxis = new QActionGroup(this);
	m_actionEditAxis->addAction(ui.actionEditX);
	m_actionEditAxis->addAction(ui.actionEditY);
	m_actionEditAxis->addAction(ui.actionEditZ);
	m_actionEditAxis->addAction(ui.actionEditXZ);

	m_objectMenu = new QMenu(m_editor);
	m_objectMenu->addAction(ui.actionSupprimer_les_objets);
	QIcon iconProperties;
	iconProperties.addFile(":/MainFrame/Resources/settings.png");
	m_objPropertiesAction = m_objectMenu->addAction(iconProperties, tr("Propriétés"));
	m_objectMenu->addSeparator();
	m_objectMenu->addAction(ui.actionCacher_les_objets);
	m_objectMenu->addAction(ui.actionCacher_les_objets_au_dessus);
	m_objectMenu->addSeparator();
	m_objectMenu->addAction(ui.actionRamener_sur_le_sol);
	m_objectMenu->addAction(ui.actionTaille_et_rotation_al_atoires);
	m_objectMenu->addSeparator();
	m_objectMenu->addAction(ui.actionCopier);
	m_objectMenu->addAction(ui.actionCouper);

	m_noObjectMenu = new QMenu(m_editor);
	m_noObjectMenu->addAction(ui.actionAfficher_tous_les_objets_cach_s);
	m_noObjectMenu->addSeparator();
	m_noObjectMenu->addAction(ui.actionColler);

	m_languageActionGroup = new QActionGroup(ui.menuLangage);
	m_languageActionGroup->addAction(ui.actionFran_ais);
	m_languageActionGroup->addAction(ui.actionEnglish);

	_connectWidgets();

	setGeometry(50, 50, 50 + 1024, 50 + 668);
	ui.dockNavigator->setFloating(true);
	ui.dockNavigator->setGeometry(1000, 500, 140, 140);
	ui.dockTerrainEdit->setFloating(true);
	ui.dockTerrainEdit->setGeometry(800, 200, 341, 260);
	ui.dockPatrolEditor->setFloating(true);
	ui.dockPatrolEditor->hide();

	g_global3D.light = true;

	_loadSettings();
	CloseFile();
}

CMainFrame::~CMainFrame()
{
	CloseFile();

	for (int i = 0; i < m_clipboardObjects.GetSize(); i++)
		Delete(m_clipboardObjects[i]);
	m_clipboardObjects.RemoveAll();

	Delete(m_noObjectMenu);
	Delete(m_objectMenu);
	Delete(m_actionEditAxis);
	Delete(m_actionEditMode);
	Delete(m_navigator);
	Delete(m_prj);
	Delete(m_editor);
	Delete(m_favoritesAddMenu);
	Delete(m_favoritesRemoveMenu);
	Delete(m_curX);
	Delete(m_curY);
	Delete(m_curZ);
	Delete(m_curLand);
	Delete(m_curLayerCount);
	Delete(m_curObject);
	Delete(m_curTexture);
	Delete(m_languageActionGroup);

	MainFrame = null;
}

void CMainFrame::_connectWidgets()
{
	connect(ui.actionFermer, SIGNAL(triggered()), this, SLOT(CloseFile()));
	connect(ui.actionOuvrir, SIGNAL(triggered()), this, SLOT(OpenFile()));
	connect(ui.actionNouveau, SIGNAL(triggered()), this, SLOT(NewFile()));
	connect(ui.menuFichiers_r_cents, SIGNAL(triggered(QAction*)), this, SLOT(OpenLastFile(QAction*)));
	connect(ui.action_propos, SIGNAL(triggered()), this, SLOT(About()));
	connect(ui.actionQt, SIGNAL(triggered()), this, SLOT(AboutQt()));
	connect(ui.actionG_n_rer_une_image, SIGNAL(triggered()), this, SLOT(SaveBigmap()));
	connect(ui.actionG_n_rer_les_minimaps, SIGNAL(triggered()), this, SLOT(SaveMinimaps()));
	connect(ui.actionGrille_des_unit_s, SIGNAL(triggered(bool)), this, SLOT(ShowGridUnity(bool)));
	connect(ui.actionGrille_des_patchs, SIGNAL(triggered(bool)), this, SLOT(ShowGridPatch(bool)));
	connect(ui.actionGrille_des_landscapes, SIGNAL(triggered(bool)), this, SLOT(ShowGridLand(bool)));
	connect(ui.actionAnimer, SIGNAL(toggled(bool)), this, SLOT(Animate(bool)));
	connect(ui.actionTerrain, SIGNAL(triggered(bool)), this, SLOT(ShowTerrain(bool)));
	connect(ui.actionTerrain_LOD, SIGNAL(triggered(bool)), this, SLOT(EnableTerrainLOD(bool)));
	connect(ui.actionEau_et_nuages, SIGNAL(triggered(bool)), this, SLOT(ShowWater(bool)));
	connect(ui.actionAttributs_du_terrain, SIGNAL(triggered(bool)), this, SLOT(ShowTerrainAttributes(bool)));
	connect(ui.actionPlein_cran, SIGNAL(triggered(bool)), this, SLOT(SwitchFullscreen(bool)));
	connect(ui.actionLumi_re, SIGNAL(triggered(bool)), this, SLOT(ShowLight(bool)));
	connect(ui.actionBrouillard, SIGNAL(triggered(bool)), this, SLOT(ShowFog(bool)));
	connect(ui.actionCiel, SIGNAL(triggered(bool)), this, SLOT(ShowSkybox(bool)));
	connect(ui.actionHeure_en_jeu, SIGNAL(triggered()), this, SLOT(SetGameTime()));
	connect(ui.actionObjets, SIGNAL(triggered(bool)), this, SLOT(ShowObjects(bool)));
	connect(ui.actionObjets_LOD, SIGNAL(triggered(bool)), this, SLOT(EnableObjectLOD(bool)));
	connect(ui.actionNoms_des_movers, SIGNAL(triggered(bool)), this, SLOT(ShowMoverNames(bool)));
	connect(ui.actionZones_de_spawn, SIGNAL(triggered(bool)), this, SLOT(ShowSpawnZones(bool)));
	connect(ui.actionSpawn_complet, SIGNAL(triggered(bool)), this, SLOT(SpawnAllMonsters(bool)));
	connect(ui.actionVue_de_face, SIGNAL(triggered(bool)), this, SLOT(SetCameraTopView()));
	connect(ui.actionVue_de_c_t, SIGNAL(triggered(bool)), this, SLOT(SetCameraSideView()));
	connect(ui.actionType_Objet, SIGNAL(triggered(bool)), this, SLOT(ShowObj(bool)));
	connect(ui.actionType_SFX, SIGNAL(triggered(bool)), this, SLOT(ShowSFX(bool)));
	connect(ui.actionType_Region, SIGNAL(triggered(bool)), this, SLOT(ShowRegions(bool)));
	connect(ui.actionType_item, SIGNAL(triggered(bool)), this, SLOT(ShowItem(bool)));
	connect(ui.actionType_control, SIGNAL(triggered(bool)), this, SLOT(ShowCtrl(bool)));
	connect(ui.actionType_PNJ, SIGNAL(triggered(bool)), this, SLOT(ShowNPC(bool)));
	connect(ui.actionType_mover_monstres, SIGNAL(triggered(bool)), this, SLOT(ShowMonster(bool)));
	connect(ui.actionCollisions, SIGNAL(triggered(bool)), this, SLOT(ShowCollision(bool)));
	connect(ui.gameElementsTree, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowFavoritesMenu(const QPoint&)));
	connect(ui.actionEnregistrer, SIGNAL(triggered()), this, SLOT(SaveFile()));
	connect(ui.actionEnregistrer_sous, SIGNAL(triggered()), this, SLOT(SaveFileAs()));
	connect(ui.editTerrainChangeTextureColo, SIGNAL(clicked()), this, SLOT(EditTerrainChoseTextureColor()));
	connect(ui.tabTerrain, SIGNAL(currentChanged(int)), this, SLOT(EditTerrainChangeMode(int)));
	connect(m_actionEditMode, SIGNAL(triggered(QAction*)), this, SLOT(ChangeEditMode(QAction*)));
	connect(ui.editTerrainDeleteLayer, SIGNAL(clicked()), this, SLOT(DeleteLandLayer()));
	connect(ui.gameElementsTree, SIGNAL(clicked(const QModelIndex &)), this, SLOT(SelectGameElement(const QModelIndex &)));
	connect(ui.gameElementsTree, SIGNAL(activated(const QModelIndex &)), this, SLOT(SelectGameElement(const QModelIndex &)));
	connect(ui.editTerrainLayerList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(SetEditTexture(QListWidgetItem*)));
	connect(ui.actionPropri_t_s_du_monde, SIGNAL(triggered()), this, SLOT(EditWorldProperties()));
	connect(ui.optimizeWater, SIGNAL(clicked()), this, SLOT(OptimizeWater()));
	connect(ui.waterFillMap, SIGNAL(clicked()), this, SLOT(FillAllMapWithWater()));
	connect(ui.action_dition_des_continents, SIGNAL(triggered()), this, SLOT(EditContinents()));
	connect(m_actionEditAxis, SIGNAL(triggered(QAction*)), this, SLOT(ChangeEditAxis(QAction*)));
	connect(ui.actionRamener_sur_le_sol, SIGNAL(triggered()), this, SLOT(SetOnLand()));
	connect(ui.actionTaille_de_la_grille, SIGNAL(triggered()), this, SLOT(SetGridSize()));
	connect(ui.actionTaille_et_rotation_al_atoires, SIGNAL(triggered()), this, SLOT(RandomScaleAndSize()));
	connect(ui.actionSupprimer_les_objets, SIGNAL(triggered()), this, SLOT(DeleteObjects()));
	connect(ui.actionCopier, SIGNAL(triggered()), this, SLOT(CopyObjects()));
	connect(ui.actionCouper, SIGNAL(triggered()), this, SLOT(CutObjects()));
	connect(ui.actionColler, SIGNAL(triggered()), this, SLOT(PasteObjects()));
	connect(ui.actionCacher_les_objets, SIGNAL(triggered()), this, SLOT(HideObjects()));
	connect(ui.actionAfficher_tous_les_objets_cach_s, SIGNAL(triggered()), this, SLOT(ShowAllObjects()));
	connect(ui.actionCacher_les_objets_au_dessus, SIGNAL(triggered()), this, SLOT(HideUpstairObjects()));
	connect(m_languageActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(SetLanguage(QAction*)));
	connect(ui.addPatrol, SIGNAL(clicked()), this, SLOT(AddPath()));
	connect(ui.removePatrol , SIGNAL(clicked()), this, SLOT(RemovePath()));
	connect(ui.patrolList, SIGNAL(currentRowChanged(int)), this, SLOT(SetCurrentPath(int)));
	connect(ui.actionListe, SIGNAL(triggered()), this, SLOT(GenerateList()));
}