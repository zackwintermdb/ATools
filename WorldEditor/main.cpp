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

	QSplashScreen* splash = new QSplashScreen(QPixmap(":/MainFrame/Splashscreen.png"));
	splash->show();
	app.processEvents();

	CMainFrame* mainFrame = new CMainFrame();

	mainFrame->show();
	splash->finish(mainFrame);
	Delete(splash);

	mainFrame->UpdateWorldEditor();
	const int result = app.exec();
	
	Delete(mainFrame);
	return result;
}