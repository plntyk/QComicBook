/*
 * This file is a part of QComicBook.
 *
 * Copyright (C) 2005-2009 Pawel Stolowski <stolowski@gmail.com>
 *
 * QComicBook is free software; you can redestribute it and/or modify it
 * under terms of GNU General Public License by Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY. See GPL for more details.
 */

#include "PageViewBase.h"
#include "ComicImage.h"
#include <QContextMenuEvent>
#include <QMenu>
#include <QBitmap>
#include <QCursor>
#include <QScrollBar>
#include <QPalette>
#include <limits>
#include "ImageTransformThread.h"
#include "Lens.h"
#include "../ComicBookDebug.h"

using namespace QComicBook;

const float PageViewBase::JUMP_FACTOR = 0.85f;

PageViewBase::PageViewBase(QWidget *parent, int physicalPages, const ViewProperties &props)
    : QGraphicsView(parent)
    , props(props)
    , m_physicalPages(physicalPages)
    , smallcursor(0)
    , lens(0)
{
    setFrameShape(QFrame::NoFrame);
    context_menu = new QMenu(this);
    connect(&this->props, SIGNAL(changed()), this, SLOT(propsChanged()));
    recalculateScrollSpeeds();

    //  setViewport(new QGLWidget);

    scene = new QGraphicsScene(this);
    setScene(scene);

//    setAlignment(Qt::AlignHCenter);
    connect(ImageTransformThread::get(), SIGNAL(jobCompleted(const ImageJobResult &)), this, SLOT(jobCompleted(const ImageJobResult &)));

}

PageViewBase::~PageViewBase()
{
    delete smallcursor;
    ImageTransformThread::get()->cancel();
}

void PageViewBase::setLensZoom(double ratio)
{
    if (lens)
    {
        lens->setZoom(ratio);
    }
}

void PageViewBase::showLens(bool f, double ratio)
{
    if (f)
    {
        if (!lens)
        {
            setMouseTracking(true);
            lens = new Lens(QSize(300, 200), props.background(), ratio);
            scene->addItem(lens);
            lens->setPos(mapToScene(mapFromGlobal(QCursor::pos())));
        }
	else
	{
	    lens->setZoom(ratio);
	}
        setCursor(Qt::BlankCursor); // hide mouse cursor
    }
    else
    {
        if (lens)
        {
            setMouseTracking(false);
            scene->removeItem(lens);
            delete lens;
            lens = 0;
        }
        if (smallcursor)
        {
            setCursor(*smallcursor);
        }
        else
        {
            unsetCursor();
        }
    }
}

void PageViewBase::scrollByDelta(int dx, int dy)
{
	QScrollBar *vbar = verticalScrollBar();
	QScrollBar *hbar = horizontalScrollBar();

	vbar->setValue(vbar->value() + dy);
	hbar->setValue(hbar->value() + dx);
}

void PageViewBase::contextMenuEvent(QContextMenuEvent *e)
{
    //if (imgs > 0)
    context_menu->popup(e->globalPos());
}

void PageViewBase::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons())
    {
        if (lx >= 0)
        {
                const int dx = lx - e->x();
                const int dy = ly - e->y();

		QScrollBar *vbar = verticalScrollBar();
		QScrollBar *hbar = horizontalScrollBar();

		vbar->setValue(vbar->value() + dy);
		hbar->setValue(hbar->value() + dx);
        }
        lx = e->x();
        ly = e->y();
    }
    else
    {
        if (lens)
        {
            lens->setPos(e->pos() + QPointF(horizontalScrollBar()->value(), verticalScrollBar()->value()));
        }
    }
}

void PageViewBase::mousePressEvent(QMouseEvent *e)
{
        if (!smallcursor)
                setCursor(Qt::PointingHandCursor);
}

void PageViewBase::mouseReleaseEvent(QMouseEvent *e)
{
        lx = -1;
	ly = -1;

        if (lens)
        {
            setCursor(Qt::BlankCursor); // hide cursor if lens visible
        }
        else
        {
            if (!smallcursor)
            {
                setCursor(Qt::ArrowCursor);
            }
            // smallcursor enabled - do nothing
        }
}

void PageViewBase::mouseDoubleClickEvent(QMouseEvent *e)
{
	e->accept();
	emit doubleClick();
}

bool PageViewBase::onTop()
{
        return verticalScrollBar()->value() == verticalScrollBar()->minimum();
}

bool PageViewBase::onBottom()
{
        return verticalScrollBar()->value() == verticalScrollBar()->maximum();
}

void PageViewBase::scrollUp()
{
        if (onTop())
        {
                wheelupcnt = wheeldowncnt = 0;
                emit topReached();
        }
        else
        {
		scrollByDelta(0, -spdy);
        }
}

void PageViewBase::scrollDown()
{
        if (onBottom())
        {
                wheelupcnt = wheeldowncnt = 0;
                emit bottomReached();
        }
        else
        {
		scrollByDelta(0, spdy);
        }
}

void PageViewBase::scrollUpFast()
{
        if (onTop())
        {
                emit topReached();
        }
        else
        {
		scrollByDelta(0, -3*spdy);
        }
}

void PageViewBase::scrollDownFast()
{
        if (onBottom())
        {
                emit bottomReached();
        }
        else
        {
		scrollByDelta(0, 3*spdy);
        }
}

void PageViewBase::scrollRight()
{
        scrollByDelta(spdx, 0);
}

void PageViewBase::scrollLeft()
{
        scrollByDelta(-spdx, 0);
}

void PageViewBase::scrollRightFast()
{
        scrollByDelta(3*spdx, 0);
}

void PageViewBase::scrollLeftFast()
{
        scrollByDelta(-3*spdx, 0);
}

void PageViewBase::jumpUp()
{
    if (onTop())
    {
        emit topReached();
    }
    else
    {
        scrollByDelta(0, -static_cast<int>(JUMP_FACTOR * viewport()->height()));
    }
}

void PageViewBase::jumpDown()
{
    if (onBottom())
    {
        emit bottomReached();
    }
    else
    {
        scrollByDelta(0, static_cast<int>(JUMP_FACTOR * viewport()->height()));
    }
}

void PageViewBase::nextFrame()
{
}

void PageViewBase::prevFrame()
{
}

void PageViewBase::setNumOfPages(int n)
{
    m_physicalPages = n;
}

int PageViewBase::numOfPages() const
{
    return m_physicalPages;
}

void PageViewBase::setRotation(Rotation r)
{
    props.setAngle(r);
}

void PageViewBase::rotateRight()
{
        setRotation(QComicBook::Right);
}

void PageViewBase::rotateLeft()
{
        setRotation(QComicBook::Left);
}

void PageViewBase::resetRotation()
{
        setRotation(None);
}

void PageViewBase::setSize(Size s)
{
    props.setSize(s);
}

void PageViewBase::setBackground(const QColor &color)
{
    QPalette palette;
    palette.setColor(backgroundRole(), color);
    setPalette(palette);
    props.setBackground(color);
    if (lens)
    {
        lens->setBackground(color);
    }
}

void PageViewBase::setTwoPagesMode(bool f)
{
    props.setTwoPagesMode(f);
}

void PageViewBase::setMangaMode(bool f)
{
    props.setMangaMode(f);
}

void PageViewBase::setSmallCursor(bool f)
{
    if (f)
    {
        static unsigned char bmp_bits[4*32];
        static unsigned char msk_bits[4*32];

        if (smallcursor)
            return;

        for (int i=0; i<4*32; i++)
        {
            bmp_bits[i] = msk_bits[i] = 0;
        }
        bmp_bits[0] = msk_bits[0] = 0xe0;
        bmp_bits[4] = 0xa0;
        msk_bits[4] = 0xe0;
        bmp_bits[8] = msk_bits[8] = 0xe0;
        const QBitmap bmp = QBitmap::fromData(QSize(32, 32), bmp_bits, QImage::Format_Mono);
        const QBitmap msk = QBitmap::fromData(QSize(32, 32), msk_bits, QImage::Format_Mono);
        smallcursor = new QCursor(bmp, msk, 0, 0);
        if (!lens) // don't set if lens are currently enabled
        {
            setCursor(*smallcursor);
        }
    }
    else
    {
        delete smallcursor;
        smallcursor = NULL;
        if (!lens) // don't unset if lens are currently enabled
        {
            unsetCursor();
        }
    }
}

void PageViewBase::showPageNumbers(bool f)
{
    props.setPageNumbers(f);
}

void PageViewBase::enableScrollbars(bool f)
{
        const Qt::ScrollBarPolicy s = f ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff;
        setVerticalScrollBarPolicy(s);
        setHorizontalScrollBarPolicy(s);
}

QMenu *PageViewBase::contextMenu() const
{
        return context_menu;
}

ViewProperties& PageViewBase::properties()
{
    return props;
}

bool PageViewBase::hasRequest(int page) const
{
    return m_requestedPages.indexOf(page) >= 0;
}

void PageViewBase::addRequest(int page, bool twoPages)
{
    m_requestedPages.append(page);
    if (twoPages)
        emit requestTwoPages(page);
    else
        emit requestPage(page);
}

void PageViewBase::delRequest(int page, bool twoPages, bool cancel)
{
    int idx = m_requestedPages.indexOf(page);
    if (idx >= 0)
    {
        m_requestedPages.removeAt(idx);
        if (cancel)
        {
            if (twoPages)
                emit cancelTwoPagesRequest(page);
            else
                emit cancelPageRequest(page);
        }
    }
}

int PageViewBase::nextPage(int page) const
{
    if (props.twoPagesMode()) //TODO odd number of pages, single 1st page
    {
        page += 2;
    }
    else
    {
        ++page;
    }
    if (page >= m_physicalPages)
    {
        page = -1;
    }
    return page;
}

int PageViewBase::previousPage(int page) const
{
    if (props.twoPagesMode())
    {
        page -= 2;
    }
    else
    {
        --page;
    }
    return page;
}

int PageViewBase::roundPageNumber(int page) const
{
    return props.twoPagesMode() ? page - (page&1) : page;
}

void PageViewBase::delRequests()
{
    m_requestedPages.clear();
}

void PageViewBase::recalculateScrollSpeeds()
{
    spdy = viewport()->height()/10;
    spdx = viewport()->width()/10;
}

void PageViewBase::resizeEvent(QResizeEvent *e)
{
    QGraphicsView::resizeEvent(e);
    //setSceneRect(scene->itemsBoundingRect());
    recalculateScrollSpeeds();
}

void PageViewBase::center(ComicImage *w, bool horizontal, bool vertical)
{
    const int x = horizontal ? (viewport()->width() - w->width()) / 2 : w->x();
    const int y = vertical ? (viewport()->height() - w->height()) / 2 : w->y();
    w->setPos(x, y);
}

void PageViewBase::updateSceneRect()
{
    int x1 = std::numeric_limits<int>::max();
    int x2 = 0;
    int y1 = x1;
    int y2 = 0;

    //
    // find items bounding rect, but skip lens item if present.
    // this is the same QGraphicsScene::itemsBoundingRect(), except for it skips lens.
    foreach (QGraphicsItem *it, items())
    {
        if (it != lens)
        {
            const QRectF itbr(it->boundingRect());

            if (x1 > it->x())
            {
                x1 = it->x();
            }
            if (y1 > it->y())
            {
                y1 = it->y();
            }
            if (it->x() + itbr.right() > x2)
            {
                x2 = it->x() + itbr.right();
            }
            if (it->y() + itbr.bottom() > y2)
            {
                y2 = it->y() + itbr.bottom();
            }
        }
    }
    const QRectF brect(QPointF(x1, y1), QPointF(x2, y2));
    _DEBUG << "scene rect:" << brect;
    setSceneRect(brect);
    viewport()->update();
}
