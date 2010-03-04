/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

//Plasma applet for showing photos from flickr

#ifndef COVERBLING_APPLET_H
#define COVERBLING_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "EngineObserver.h"
#include "PhotoBrowser.h"

class TextScrollingWidget;
class KConfigDialog;
class PhotosScrollWidget;
class QGraphicsSimpleTextItem;
class QGraphicsProxyWidget;
class RatingWidget;
class QGraphicsPixmapItem;

namespace Plasma
{
    class IconWidget;
}
class MyGraphicItem:public QObject, public QGraphicsPixmapItem
{
	Q_OBJECT
	public:
	  MyGraphicItem(const QPixmap & pixmap, QGraphicsItem * parent = 0, QGraphicsScene * scene = 0);
	  enum {Type = 65536}; //UserType should be greater then = 65535
	signals:
  		void clicked();
	private:
	  void mousePressEvent(QGraphicsSceneMouseEvent * event);
};
class CoverBlingApplet : public Context::Applet, public EngineObserver
{
        Q_OBJECT

    public:
        CoverBlingApplet( QObject* parent, const QVariantList& args );
        ~CoverBlingApplet();

        void    init();
        void    paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );
        
    public slots:
	void 	slotAlbumQueryResult( QString collectionId, Meta::AlbumList albums);
	void	slideChanged(int islideindex);
        void	playAlbum(int islideindex);
    protected :
	virtual void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );
    private:
	PhotoBrowser * m_pictureflow;
	QGraphicsProxyWidget * m_layout;
	RatingWidget* m_ratingWidget;
	QGraphicsSimpleTextItem* m_label;

	MyGraphicItem* m_blingtofirst;
	MyGraphicItem* m_blingtolast;
	MyGraphicItem* m_blingfastback;
	MyGraphicItem* m_blingfastforward;
};
K_EXPORT_AMAROK_APPLET( coverbling, CoverBlingApplet )

#endif /* COVERBLING_APPLET_H */
