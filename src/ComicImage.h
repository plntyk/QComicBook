/*
 * This file is a part of QComicBook.
 *
 * Copyright (C) 2005-2012 Pawel Stolowski <stolowski@gmail.com>
 *
 * QComicBook is free software; you can redestribute it and/or modify it
 * under terms of GNU General Public License by Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY. See GPL for more details.
 */

#ifndef __COMIC_IMAGE_H
#define __COMIC_IMAGE_H

#include <QWidget>
#include <QSize>
#include <QMatrix>
#include <QGraphicsItem>
#include <QPixmap>
#include "JobSource.h"
#include "Counted.h"

class QPixmap;
class QPainter;

namespace QComicBook
{
	class PageViewBase;

	class ComicImage: public QGraphicsItem, public JobSource, public Counted<ComicImage>
	{
        public:
            ComicImage(PageViewBase *parent);
            virtual ~ComicImage();

            virtual void dispose();
            virtual bool isDisposed() const;

            bool isInView(int vy1, int vy2) const;
            void setSourceSize(int w, int h);
            QSize getSourceSize() const;
            QSize getScaledSize() const;
            const QPixmap* pixmap() const;
            QRectF boundingRect() const;

            void requestRedraw();
            PageViewBase* view() const;
            void recalcScaledSize();
            int width() const;
            int height() const;

            virtual void propsChanged() = 0;

        protected:
            void paint(QPainter *painter, const QStyleOptionGraphicsItem *opt, QWidget *widget = 0);
            void redraw(const QImage &img);
            virtual void requestRedraw(const QSize& requestedSize, const QMatrix &rotationMatrix);

        private:
            PageViewBase *m_view;
            int xoff, yoff;
            QMatrix rmtx;
            QSize m_sourceSize; //image size without scaling
            QSize m_scaledSize; //image size with scaling and rotation
            QPixmap *m_pixmap;
	};
}

#endif
