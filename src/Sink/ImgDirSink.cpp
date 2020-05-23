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

#include "ImgDirSink.h"
#include "ComicBookSettings.h"
#include "ImageFormatsInfo.h"
#include <QImage>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

using namespace QComicBook;

//
// maximum size of description file (won't load files larger than that)
const int ImgDirSink::MAX_TEXTFILE_SIZE = 65535;

ImgDirSink::ImgDirSink(bool dirs, int cacheSize): ImgSink(cacheSize), dirpath(QString()), DirReader(QDir::DirsLast|QDir::Name|QDir::IgnoreCase, 6)
{
}

ImgDirSink::ImgDirSink(const QString &path, bool dirs, int cacheSize): ImgSink(cacheSize), dirpath(QString()), DirReader(QDir::DirsLast|QDir::Name|QDir::IgnoreCase, 6)
{
	open(path);
}

ImgDirSink::ImgDirSink(const ImgDirSink &sink, int cacheSize): ImgSink(cacheSize), DirReader(QDir::DirsLast|QDir::Name|QDir::IgnoreCase, 6)
{
	dirpath = sink.dirpath;
	imgfiles = sink.imgfiles;
	txtfiles = sink.txtfiles;
	otherfiles = sink.otherfiles;
	dirs = sink.dirs;
	timestamps = sink.timestamps;
}

ImgDirSink::~ImgDirSink()
{
        close();
}

QString ImgDirSink::memPrefix(int &s)
{
        QString mempfix;
        if (s < 1024)
        {
                mempfix = tr("bytes");
        }
        else if (s < 1024*1024)
        {
                s /= 1024;
                mempfix = tr("Kbytes");
        }
        else
        {
                s /= 1024*1024;
                mempfix = tr("Mbytes");
        }
        return mempfix;
}

bool ImgDirSink::fileHandler(const QFileInfo &finfo)
{
	const QString fname = finfo.fileName();
	if (knownImageExtension(fname))
	{
		imgfiles.append(finfo.absoluteFilePath());
		timestamps.insert(finfo.absoluteFilePath(), FileStatus(finfo.lastModified()));
		return true;
	}
	if (fname.endsWith(".nfo", Qt::CaseInsensitive) || fname == "file_id.diz")
	{
		txtfiles.append(finfo.absoluteFilePath());
		return true;
	}
	otherfiles.append(finfo.absoluteFilePath());
	return false;
}

int ImgDirSink::open(const QString &path)
{
        int status;
        emit progress(0, 1);
        QFileInfo info(path);
        if (!info.exists())
                status = SINKERR_NOTFOUND;
        else
        {
                if (info.isDir())
                {
                        if (info.isReadable() && info.isExecutable())
                        {
                                dirpath = path;
				visit(path);
                                status = (numOfImages() > 0) ? 0 : SINKERR_EMPTY;
                        }
                        else
                                status = SINKERR_ACCESS;
                }
                else status = SINKERR_NOTDIR;
        }
        setComicBookName(path, dirpath);
        if (status == 0)
                emit progress(1, 1);
        return status;
}

void ImgDirSink::close()
{
        listmtx.lock();
        dirpath = QString();
        imgfiles.clear();
        txtfiles.clear();
        otherfiles.clear();
        dirs.clear();
        listmtx.unlock();
}

QString ImgDirSink::getFullFileName(int page) const
{
	return page < numOfImages() ? imgfiles[page] : QString();
}

QStringList ImgDirSink::getDescription() const
{
	if (desc.count() == 0) //read files only once
	{
		for (QStringList::const_iterator it = txtfiles.begin(); it!=txtfiles.end(); it++)
		{
			QFileInfo finfo(*it);
			QFile f(*it);
			if (f.open(QIODevice::ReadOnly) && (f.size() < MAX_TEXTFILE_SIZE))
			{
				QString cont;
				QTextStream str(&f);
				while (!str.atEnd())
					cont += str.readLine() + "\n";
				f.close();
				desc.append(finfo.fileName()); //append file name
				desc.append(cont); //and contents
			}
		}
	}
        return desc;
}

QImage ImgDirSink::image(unsigned int num, int &result)
{
	result = SINKERR_LOADERROR;

	listmtx.lock();
	const int imgcnt = imgfiles.count();
	QImage im;

	if (num < imgcnt)
	{
		const QString fname = imgfiles[num];
		listmtx.unlock();

		if (!im.load(fname))
			result = 1;
		else
			result = 0;

		/*const QFileInfo finf(fname);

		f (rimg.load(fname))
		{
			result = 0;
			if (timestamps[fname] != finf.lastModified())
				timestamps[fname].set(finf.lastModified(), true);
		}*/

	}
	else
	{
		listmtx.unlock();
		result = 0;
	}

	const Page page(num, im);
	return page;
}

int ImgDirSink::numOfImages() const
{
        listmtx.lock();
        const int n = imgfiles.count();
        listmtx.unlock();
        return n;
}

QStringList ImgDirSink::getAllfiles() const
{
        listmtx.lock();
        QStringList l = imgfiles + txtfiles + otherfiles;
        listmtx.unlock();
        return l;
}

QStringList ImgDirSink::getAlldirs() const
{
        return dirs;
}

QStringList ImgDirSink::getAllimgfiles() const
{
        listmtx.lock();
        const QStringList l = imgfiles;
        listmtx.unlock();
        return l;
}

bool ImgDirSink::timestampDiffers(int page) const
{
	if (page < 0 || page > numOfImages())
		return false;
	listmtx.lock();
	const QString fname = imgfiles[page];
	listmtx.unlock();
	QFileInfo f(fname);
	return f.lastModified() != timestamps[fname];
}

bool ImgDirSink::hasModifiedFiles() const
{
	//
	// check timestamps of all files
	for (QMap<QString, FileStatus>::ConstIterator it = timestamps.begin(); it != timestamps.end(); ++it)
	{
		QFileInfo finf(it.key());
		if ((*it).isModified() || *it != finf.lastModified())
			return true;
	}
	return false;
}

bool ImgDirSink::supportsNext() const
{
	return false;
}

QString ImgDirSink::getNext() const
{
	return QString();
}

QString ImgDirSink::getPrevious() const
{
	return QString();
}

void ImgDirSink::removeThumbnails(int days)
{
        if (days < 1)
                return;

        const QDateTime currdate = QDateTime::currentDateTime();

        QDir dir(ComicBookSettings::instance().thumbnailsDir(), "*.jpg", QDir::Unsorted, QDir::Files|QDir::NoSymLinks);
        const QStringList files = dir.entryList();
        for (QStringList::const_iterator it = files.begin(); it!=files.end(); it++)
        {
                QFileInfo finfo(dir.absoluteFilePath(*it));
                if (finfo.lastModified().daysTo(currdate) > days)
                        dir.remove(*it);
        }
}

bool ImgDirSink::knownImageExtension(const QString &path)
{
    foreach (QString ext, ImageFormatsInfo::instance().extensions())
    {
        if (path.endsWith(ext, Qt::CaseInsensitive))
        {
            return true;
        }
    }
    return false;
}

QString ImgDirSink::getKnownImageExtension(const QString &path)
{
    foreach (QString ext, ImageFormatsInfo::instance().extensions())
    {
        if (path.endsWith(ext, Qt::CaseInsensitive))
            return ext;
    }
    return QString();
}

QStringList ImgDirSink::getKnownImageExtensionsList()
{
    QStringList list;
    foreach (QString ext, ImageFormatsInfo::instance().extensions())
    {
        QString p("*");
        p.append(ext);
        list << p;
    }
    return list;
}

