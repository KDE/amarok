/****************************************************************************
** Filename: URLLabel.cpp
** Last updated [dd/mm/yyyy]: 14/02/2005
**
** QLabel subclass with URL handling and more.
**
** Copyright(C) 2005 Angius Fabrizio. All rights reserved.
**
** Based on the LGPL v.2 licensed KURLLabel from the KDE libraries by
** Kurt Granroth <granroth@kde.org> and Peter Putzer <putzer@kde.org>
**
** Changes made to the KURLLabel code:
**  - link color is no longer taken from KGlobalSettings but from qApp->palette().active().link()
**  - removed virtual_hook() member function
**  - replaced KCursor::handCursor() with QCursor(Qt::PointingHandCursor)
**  - added context menu (see mouseReleaseEvent method)
**
** This file is part of the OSDaB project(http://osdab.sourceforge.net/).
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See the file COPYING that came with this software distribution or
** visit http://www.gnu.org/copyleft/gpl.html for GPL licensing information.
**
**********************************************************************/

#include "URLLabel.h"

#include <QApplication>
#include <QMouseEvent>
#include <QTimer>
#include <QDesktopServices>

#undef LOG
#undef LOGL
#define LOG(x, y)
#define LOGL(x, y)

#ifdef Q_WS_MAC
    #include <ApplicationServices/ApplicationServices.h>
#endif


class URLLabel::Private
{
    public:
        Private(const QString& url, URLLabel* label) :
            URL(url),
            Underline(true),
            LinkColor(qApp->palette().link().color()),
            HighlightedLinkColor(Qt::red),
            Tip(url),
            Cursor(0L),
            UseTips(false),
            UseCursor(false),
            Glow(true),
            Float(false),
            RealUnderline(true),
            Timer(new QTimer(label))
        {
            connect( Timer, SIGNAL( timeout() ), label, SLOT( updateColor() ) );
            connect( label, SIGNAL( leftClickedURL( const QString& ) ),
                     label, SLOT( openURL( const QString& ) ) );
        }

        ~Private()
        {
        }

        QString URL;
        QPixmap AltPixmap;

        bool Underline;
        QColor LinkColor;
        QColor HighlightedLinkColor;

        QString Tip;
        QCursor* Cursor;
        bool UseTips:1;
        bool UseCursor:1;
        bool Glow:1;
        bool Float:1;
        bool RealUnderline:1;
        QPixmap RealPixmap;

        QTimer* Timer;
};


URLLabel::URLLabel( const QString& url, const QString& text, QWidget* parent ) :
    QLabel( !text.isNull() ? text : url, parent ),
    d( new Private( url, this ) )
{
    setFont( font() );
    setCursor( QCursor( Qt::PointingHandCursor ) );
    setLinkColor( d->LinkColor );
}


URLLabel::URLLabel( QWidget* parent ) :
    QLabel( parent ),
    d( new Private( QString::null, this ) )
{
    setFont( font() );
    setCursor( QCursor( Qt::PointingHandCursor ) );
    setLinkColor( d->LinkColor );
}


URLLabel::~URLLabel()
{
delete d;
}


void URLLabel::mouseReleaseEvent( QMouseEvent* e )
{
//     QLabel::mouseReleaseEvent( e );

    setLinkColor( d->HighlightedLinkColor );
    d->Timer->start( 300 );

    switch( e->button() )
    {
        case Qt::LeftButton:
            emit leftClickedURL();
            emit leftClickedURL( d->URL );
            break;

        case Qt::MidButton:
            emit middleClickedURL();
            emit middleClickedURL( d->URL );
            break;

        case Qt::RightButton:
            // commented: Angius Fabrizio (2005-02-14)
            emit rightClickedURL();
            emit rightClickedURL( d->URL );
            break;

        default:
            ; // nothing
    }
}


void URLLabel::setFont( const QFont& f )
{
    QFont newFont = f;
    newFont.setUnderline( d->Underline );

    QLabel::setFont(newFont);
}


void URLLabel::setUnderline(bool on)
{
	d->Underline = on;

	setFont(font());
}


void URLLabel::updateColor()
{
	d->Timer->stop();

	if(!(d->Glow || d->Float) || !rect().contains(mapFromGlobal(QCursor::pos())))
		setLinkColor(d->LinkColor);
}


void URLLabel::setLinkColor(const QColor& col)
{
	QPalette p = palette();
	p.setColor(QPalette::WindowText, col);
	p.setColor(QPalette::Text, col);
	p.setColor(QPalette::Link, col);
	setPalette(p);

	update();
}


void URLLabel::setURL(const QString& url)
{
	if( d->Tip == d->URL ) { // update the tip as well
		d->Tip = url;
		setUseTips( d->UseTips );
	}

	d->URL = url;
}


const QString& URLLabel::url() const
{
	return d->URL;
}


void URLLabel::setUseCursor(bool on, QCursor* cursor)
{
	d->UseCursor = on;
	d->Cursor = cursor;

	if(on)
		{
			if(cursor)
				setCursor(*cursor);
			else
				setCursor(QCursor(Qt::PointingHandCursor));
		}
	else
		unsetCursor();
}


bool URLLabel::useCursor() const
{
	return d->UseCursor;
}


void URLLabel::setUseTips(bool on)
{
	d->UseTips = on;

	if(on)
        setToolTip(d->Tip);
	else
		setToolTip("");
}


void URLLabel::setTipText(const QString& tip)
{
	d->Tip = tip;

	setUseTips(d->UseTips);
}


bool URLLabel::useTips() const
{
	return d->UseTips;
}


const QString& URLLabel::tipText() const
{
	return d->Tip;
}


void URLLabel::setHighlightedColor(const QColor& highcolor)
{
	d->LinkColor = highcolor;

	if(!d->Timer->isActive())
		setLinkColor(highcolor);
}


void URLLabel::setHighlightedColor(const QString& highcolor)
{
	setHighlightedColor(QColor(highcolor));
}


void URLLabel::setSelectedColor(const QColor& selcolor)
{
	d->HighlightedLinkColor = selcolor;

	if(d->Timer->isActive())
		setLinkColor(selcolor);
}


void URLLabel::setSelectedColor(const QString& selcolor)
{
	setSelectedColor(QColor(selcolor));
}


void URLLabel::setGlow(bool glow)
{
	d->Glow = glow;
}


void URLLabel::setFloat(bool do_float)
{
	d->Float = do_float;
}


bool URLLabel::isGlowEnabled() const
{
	return d->Glow;
}


bool URLLabel::isFloatEnabled() const
{
	return d->Float;
}


void URLLabel::setAltPixmap(const QPixmap& altPix)
{
	d->AltPixmap = altPix;
}


const QPixmap* URLLabel::altPixmap() const
{
	return &d->AltPixmap;
}


void URLLabel::enterEvent(QEvent* e)
{
	QLabel::enterEvent(e);

	if(!d->AltPixmap.isNull() && pixmap())
		{
			d->RealPixmap = *pixmap();
			setPixmap(d->AltPixmap);
		}

	if(d->Glow || d->Float)
		{
			d->Timer->stop();

			setLinkColor(d->HighlightedLinkColor);

			d->RealUnderline = d->Underline;

			if(d->Float)
				setUnderline(true);
		}

	emit enteredURL();
	emit enteredURL(d->URL);
}


void URLLabel::leaveEvent(QEvent* e)
{
	QLabel::leaveEvent(e);

	if(!d->AltPixmap.isNull() && pixmap())
		setPixmap(d->RealPixmap);

	if((d->Glow || d->Float) && !d->Timer->isActive())
		setLinkColor(d->LinkColor);

	setUnderline(d->RealUnderline);

	emit leftURL();
	emit leftURL(d->URL);
}


bool URLLabel::event(QEvent *e)
{
/*
	if(e && e->type() == QEvent::PaletteChange)
	{
		// use parentWidget() unless you are a toplevel widget, then try qAapp
		QPalette p = parentWidget() ? parentWidget()->palette() : qApp->palette();
		p.setBrush(QPalette::Base, p.brush(QPalette::Active, QPalette::Window));
		p.setColor(QPalette::WindowText, palette().color(QPalette::WindowText));
		setPalette(p);
		d->LinkColor = qApp->palette().link().color();
		setLinkColor(d->LinkColor);
		return true;
	}
	else
*/
		return QLabel::event(e);
}


void
URLLabel::openURL( const QString& url )
{
    if ( !url.isEmpty() )
    {
        LOGL( 3, "Opening in browser: " << url );
        QDesktopServices::openUrl( url );
    }
}

void
URLLabel::openURL()
{
    openURL( d->URL );
}
