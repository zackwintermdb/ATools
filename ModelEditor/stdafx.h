///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#ifndef STDAFX_H
#define STDAFX_H

#define APP_NAME	"ModelEditor"
#define MODEL_EDITOR

class CImporter;
class CMainFrame;
class CDAEExporter;
class CDialogEditEffects;

#define MODEL_EDITOR_FRIENDS	friend class CImporter; friend class CMainFrame; friend class CDAEExporter; friend class CDialogEditEffects;

#include <Common.h>

#include <qdom.h>

#endif // STDAFX_H