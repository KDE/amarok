/****************************************************************************
** Filename: URLLabel.h
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
** See the file LICENSE.GPL that came with this software distribution or
** visit http://www.gnu.org/copyleft/gpl.html for GPL licensing information.
**
**********************************************************************/

#ifndef URLLABEL_H
#define URLLABEL_H

#include "UnicornDllExportMacro.h"

#include <QUrl>
#include <QLabel>


class QCursor;
class QColor;
class QPixmap;

class UNICORN_DLLEXPORT URLLabel : public QLabel
{
	Q_OBJECT
	Q_PROPERTY(QString url READ url WRITE setURL)
	Q_PROPERTY(QString tipText READ tipText WRITE setTipText )
	Q_PROPERTY(QPixmap altPixmap READ altPixmap WRITE setAltPixmap)
	Q_PROPERTY(bool glowEnabled READ isGlowEnabled WRITE setGlow )
	Q_PROPERTY(bool floatEnabled READ isFloatEnabled WRITE setFloat )
	Q_PROPERTY(bool useTips READ useTips WRITE setUseTips )
	Q_PROPERTY(bool useCursor READ useCursor WRITE setUseCursor )

public:
	URLLabel(QWidget* parent = 0);
	URLLabel(const QString& url, const QString& text, QWidget* parent = 0);

	virtual ~URLLabel();

	const QString& url() const;
	const QString& tipText() const;

	bool useTips() const;
	bool useCursor() const;
	bool isGlowEnabled() const;
	bool isFloatEnabled() const;

	const QPixmap* altPixmap() const;

public slots:
	void setUnderline(bool on = true);

    void setURL( const QString& url );
    void setURL( const QUrl& url ) { setURL( url.toString() ); }

	virtual void setFont(const QFont&);

	void setUseTips(bool on = true);
	void setTipText(const QString& tip);

    void setLinkColor(const QColor& col);
	void setHighlightedColor(const QColor& highcolor);
	void setHighlightedColor(const QString& highcolor);

	void setSelectedColor(const QColor& selcolor);
	void setSelectedColor(const QString& selcolor);

	void setUseCursor(bool on, QCursor* cursor = 0L);

	void setGlow(bool glow = true);
	void setFloat(bool do_float = true);

	void setAltPixmap(const QPixmap& altPix);

    void openURL(const QString& url);
    void openURL();

signals:
	void enteredURL(const QString& url);
	void enteredURL();

	void leftURL(const QString& url);
	void leftURL();

	void leftClickedURL(const QString& url);
	void leftClickedURL();

	void rightClickedURL(const QString& url);
	void rightClickedURL();

	void middleClickedURL(const QString& url);
	void middleClickedURL();

protected:
	virtual void mouseReleaseEvent(QMouseEvent*);
	virtual void enterEvent(QEvent*);
	virtual void leaveEvent(QEvent*);
	virtual bool event(QEvent *e);

private slots:
	void updateColor();

private:
	class Private;
	Private* d;
};

#endif // URLLABEL_H
