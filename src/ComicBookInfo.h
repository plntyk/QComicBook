/*
 * This file is a part of QComicBook.
 *
 * Copyright (C) 2005-2006 Pawel Stolowski <pawel.stolowski@wp.pl>
 *
 * QComicBook is free software; you can redestribute it and/or modify it
 * under terms of GNU General Public License by Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY. See GPL for more details.
 */

#ifndef __CBINFO_H
#define __CBINFO_H

#include <QDialog>
#include <QFont>

class QStringList;
class QTabWidget;

namespace QComicBook
{
	class ImgSink;

	class ComicBookInfo: public QDialog
	{
	    Q_OBJECT

		private:
			QTabWidget *tabs;

		protected:
			QFont font;

			void setupGeneralTab(ImgSink &sink);
			void setupDescriptionTabs(const ImgSink &sink);

		public:
			ComicBookInfo(QWidget *parent, ImgSink &sink, const QFont& f);
			virtual ~ComicBookInfo();
	};
}

#endif

