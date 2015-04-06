///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#include "stdafx.h"
#include "MainFrame.h"

int main(int argc, char *argv[])
{
	InstallMsgHandler();

	QApplication app(argc, argv);

	CMainFrame* mainFrame = new CMainFrame();
	mainFrame->show();

	const int result = app.exec();

	Delete(mainFrame);
	return result;
}