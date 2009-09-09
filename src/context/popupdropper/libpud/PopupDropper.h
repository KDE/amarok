/***************************************************************************
 *   Copyright (c) 2008  Jeff Mitchell <mitchell@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef POPUPDROPPER_H
#define POPUPDROPPER_H

#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsView>
#include <QtCore/QStack>

#include "PopupDropper_Export.h"

class QGraphicsSvgItem;
class QGraphicsItem;
class QSvgRenderer;
class QTimeLine;
class QWidget;
class PopupDropper;
class PopupDropperItem;
class PopupDropperPrivate;

class POPUPDROPPER_EXPORT PopupDropper : public QObject
{
    Q_OBJECT

    Q_PROPERTY( Fading fading READ fading WRITE setFading )
    Q_PROPERTY( int overlayLevel READ overlayLevel )
    Q_PROPERTY( int deleteTimeout READ deleteTimeout WRITE setDeleteTimeout )
    Q_PROPERTY( bool standalone READ standalone )
    Q_PROPERTY( bool quitOnDragLeave READ quitOnDragLeave WRITE setQuitOnDragLeave )
    Q_PROPERTY( QColor windowColor READ windowColor WRITE setWindowColor )
    Q_PROPERTY( QBrush windowBackgroundBrush READ windowBackgroundBrush WRITE setWindowBackgroundBrush )
    Q_PROPERTY( QColor baseTextColor READ baseTextColor WRITE setBaseTextColor )
    Q_PROPERTY( QPen hoveredBorderPen READ hoveredBorderPen WRITE setHoveredBorderPen )
    Q_PROPERTY( QBrush hoveredFillBrush READ hoveredFillBrush WRITE setHoveredFillBrush )
    Q_PROPERTY( QString windowTitle READ windowTitle WRITE setWindowTitle )
    Q_PROPERTY( QString svgFile READ svgFile WRITE setSvgFile )
    Q_PROPERTY( QSvgRenderer* svgRenderer READ svgRenderer WRITE setSvgRenderer )
    Q_PROPERTY( int horizontalOffset READ horizontalOffset WRITE setHorizontalOffset )
    Q_PROPERTY( const QTimeLine* fadeHideTimer READ fadeHideTimer )
    Q_PROPERTY( const QTimeLine* fadeShowTimer READ fadeShowTimer )
    Q_PROPERTY( const QSize viewSize READ viewSize )

public:    
    enum Fading { NoFade, FadeIn, FadeOut, FadeInOut };
    Q_ENUMS( Fading )

    explicit PopupDropper( QWidget *parent, bool standalone = false );
    ~PopupDropper();

    int overlayLevel() const;
    void initOverlay( QWidget* parent, PopupDropperPrivate* priv = 0 );

    void addOverlay();

    PopupDropperItem* addSubmenu( PopupDropper** pd, const QString &text );

    bool standalone() const;
    
    void show();
    void showAllOverlays();
    void hideAllOverlays();
    void update();
    void updateAllOverlays();
    bool isHidden() const;
    bool isEmpty( bool allItems = true ) const;
    
    bool quitOnDragLeave() const;
    void setQuitOnDragLeave( bool quit );
    
    int fadeInTime() const;
    void setFadeInTime( const int msecs );    
    int fadeOutTime() const;
    void setFadeOutTime( const int msecs );
    PopupDropper::Fading fading() const;
    void setFading( PopupDropper::Fading fade );
    const QTimeLine* fadeHideTimer() const;
    const QTimeLine* fadeShowTimer() const;

    void setDeleteTimeout( int msecs );
    int deleteTimeout() const;
    
    QColor windowColor() const;
    void setWindowColor( const QColor &window );    
    QBrush windowBackgroundBrush() const;
    void setWindowBackgroundBrush( const QBrush &window );
    QColor baseTextColor() const;
    void setBaseTextColor( const QColor &baseText );
    QColor hoveredTextColor() const;
    void setHoveredTextColor( const QColor &hoveredText );
    QPen hoveredBorderPen() const;
    void setHoveredBorderPen( const QPen &hoveredBorder );
    QBrush hoveredFillBrush() const;
    void setHoveredFillBrush( const QBrush &hoveredFill );
    void setColors( const QColor &window, const QColor &baseText, const QColor &hoveredText, const QColor &hoveredBorder, const QColor &hoveredFill );
    void setPalette( const QColor &window );
    void setPalette( const QColor &window, const QColor &baseText, const QColor &hoveredText, const QColor &hoveredBorder, const QColor &hoveredFill );

    QString windowTitle() const;
    void setWindowTitle( const QString &title );
    
    QString svgFile() const;
    void setSvgFile( const QString &file );
    QSvgRenderer* svgRenderer();
    void setSvgRenderer( QSvgRenderer *renderer );

    void setHorizontalOffset( int pixels );
    int horizontalOffset() const;

    const QSize viewSize() const;

    void addItem( PopupDropperItem *item, bool useSharedRenderer = true );
    void addSeparator( PopupDropperItem *separator = 0 );

Q_SIGNALS:
    void fadeHideFinished();

public Q_SLOTS:
    void clear();
    void hide();
    bool subtractOverlay();

private Q_SLOTS:
    void activateSubmenu();
    void slotHideAllOverlays();

private:
    friend class PopupDropperView;
    friend class PopupDropperPrivate;
    PopupDropperPrivate* d;

    void addOverlay( PopupDropperPrivate* newD );
    void addItem( PopupDropperItem *item, bool useSharedRenderer, bool appendToList );

    QStack<PopupDropperPrivate*> m_viewStack;
};

#endif //POPUPDROPPER_H
