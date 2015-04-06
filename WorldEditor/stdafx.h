///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#ifndef STDAFX_H
#define STDAFX_H

#define APP_NAME	"WorldEditor"
#define WORLD_EDITOR

class CMainFrame;
class CWorldEditor;
class CDialogWorldProperties;
class CNavigator;
class CDialogContinentEdit;
class CObjectPropertiesDialog;
class CDialogEditLandscapes;

#define WORLD_EDITOR_FRIENDS	friend class CMainFrame; friend class CWorldEditor; friend class CNavigator; friend class CDialogWorldProperties; friend class CDialogContinentEdit; friend class CObjectPropertiesDialog; friend class CDialogEditLandscapes;

#include <Common.h>

#include <defineWorld.h>
#include <defineObj.h>
#include <defineAttribute.h>
#include <defineJob.h>

#endif // STDAFX_H