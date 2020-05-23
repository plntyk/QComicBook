/*
 * This file is a part of QComicBook.
 *
 * Copyright (C) 2005-2010 Pawel Stolowski <stolowski@gmail.com>
 *
 * QComicBook is free software; you can redestribute it and/or modify it
 * under terms of GNU General Public License by Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY. See GPL for more details.
 */

#include <QApplication>
#include <QMessageBox>
#include <QSplashScreen>
#include <QTimer>
#include <QLibraryInfo>
#include <QTranslator>
#include <QLocale>
#include "ComicMainWindow.h"
#include "ComicBookSettings.h"
#include "ComicFrameList.h"
#include "ImageJobResult.h"
#include "Thumbnail.h"
#include "Page.h"
#include "config.h"

int main(int argc, char *argv[])
{
	using namespace QComicBook;

	QApplication app(argc, argv);
        app.setApplicationName("QComicBook");
        app.setOrganizationName("PawelStolowski");
        app.setOrganizationDomain("linux-projects.net");

        //
        // load translation
        QTranslator qtTrans;
        qtTrans.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
        app.installTranslator(&qtTrans);

        QTranslator qcomicbookTrans;
        qcomicbookTrans.load("qcomicbook_" + QLocale::system().name(), DATADIR "/i18n");
        app.installTranslator(&qcomicbookTrans);

	const QString errcaption = ComicMainWindow::tr("QComicBook error");

        qRegisterMetaType<Page>("Page");
        qRegisterMetaType<Thumbnail>("Thumbnail");
        qRegisterMetaType<ComicFrameList>("ComicFrameList");
        qRegisterMetaType<ImageJobResult>("ImageJobResult");

	ComicBookSettings::instance().load();


	if (!ComicBookSettings::instance().checkDirs())
        {
		QMessageBox::critical(NULL, errcaption, ComicMainWindow::tr("Can't initialize QComicBook directories"),
				QMessageBox::Ok, QMessageBox::NoButton);
        }

	ComicMainWindow *win = new ComicMainWindow(NULL);
	//app.setMainWidget(win);
	win->show();

	//
	// show splashscreen
	if (ComicBookSettings::instance().showSplash())
	{
		QPixmap splashpix(":/images/qcomicbook-splash.png");
		if (!splashpix.isNull())
		{
                    QSplashScreen *splash = new QSplashScreen(splashpix, Qt::WindowStaysOnTopHint);
                    splash->show();

                    //
                    // close splashscreen after a few seconds
                    QTimer *timer = new QTimer(win);
                    QObject::connect(timer, SIGNAL(timeout()), splash, SLOT(close()));
                    timer->setSingleShot(true);
                    timer->start(2*1000);
		}
	}

	//
	// command line argument
	if (app.arguments().size() > 1)
	{
		win->open(app.arguments().at(1));
	}
	else
	{
		if (ComicBookSettings::instance().showDonationDialog())
		{
			win->showAboutDonating(true);
		}
	}

	return app.exec();
}

