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

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QStack>

class QGraphicsSvgItem;
class QGraphicsItem;
class QSvgRenderer;
class QTimeLine;
class QWidget;
class PopupDropper;
class PopupDropperAction;
class PopupDropperItem;
class PopupDropperPrivate;

class PopupDropper : public QObject
{
    Q_OBJECT

    Q_PROPERTY( Fading fading READ fading WRITE setFading )
    Q_PROPERTY( int overlayLevel READ overlayLevel )
    Q_PROPERTY( int deleteTimeout READ deleteTimeout WRITE setDeleteTimeout )
    Q_PROPERTY( bool standalone READ standalone )
    Q_PROPERTY( bool quitOnDragLeave READ quitOnDragLeave WRITE setQuitOnDragLeave )
    Q_PROPERTY( QColor windowColor READ windowColor WRITE setWindowColor )
    Q_PROPERTY( QColor textColor READ textColor WRITE setTextColor )
    Q_PROPERTY( QString windowTitle READ windowTitle WRITE setWindowTitle )
    Q_PROPERTY( QString svgFile READ svgFile WRITE setSvgFile )
    Q_PROPERTY( QSvgRenderer* svgRenderer READ svgRenderer WRITE setSvgRenderer )
    Q_PROPERTY( int horizontalOffset READ horizontalOffset WRITE setHorizontalOffset )
    Q_PROPERTY( int totalItems READ totalItems WRITE setTotalItems )
    Q_PROPERTY( const QTimeLine* fadeTimer READ fadeTimer )

public:    
    enum Fading { NoFade, FadeIn, FadeOut, FadeInOut };
    Q_ENUMS( Fading )
    enum HideReason { BackgroundChange, DragLeave, SubtractingOverlay, Unknown };

    PopupDropper( QWidget *parent, bool standalone = false );
    ~PopupDropper();

    int overlayLevel() const;
    void initOverlay( QWidget* parent, PopupDropperPrivate* priv = 0 );

    void addOverlay();
    bool subtractOverlay();

    PopupDropperItem* addSubmenu( PopupDropper** pd, QSvgRenderer* renderer, const QString &elementId, const QString &text );

    bool isValid() const;
    bool standalone() const;
    
    void show();
    void hide( PopupDropper::HideReason = PopupDropper::Unknown );
    bool isHidden() const;
    void clear();
    bool isEmpty() const;
    
    bool quitOnDragLeave() const;
    void setQuitOnDragLeave( bool quit );
    
    int fadeInTime() const;
    void setFadeInTime( const int msecs );    
    int fadeOutTime() const;
    void setFadeOutTime( const int msecs );
    PopupDropper::Fading fading() const;
    void setFading( PopupDropper::Fading fade );
    const QTimeLine* fadeTimer() const;

    void setDeleteTimeout( int msecs );
    int deleteTimeout() const;
    
    void forceUpdate();
    void textUpdated();
    
    QColor windowColor() const;
    void setWindowColor( const QColor &window );
    QColor textColor() const;
    void setTextColor( const QColor &text );
    void setColors( const QColor &window, const QColor &text );
    void setPalette( const QColor &window, const QColor &text );

    QString windowTitle() const;
    void setWindowTitle( const QString &title );
    
    QString svgFile() const;
    void setSvgFile( const QString &file );
    QSvgRenderer* svgRenderer();
    void setSvgRenderer( QSvgRenderer *renderer );

    void setHorizontalOffset( int pixels );
    int horizontalOffset() const;

    int totalItems() const;
    void setTotalItems( int items );
    void addItem( QGraphicsSvgItem *item, bool useSharedRenderer = true );

private slots:
    void activateSubmenu();

private:
    friend class PopupDropperView;
    friend class PopupDropperPrivate;
    PopupDropperPrivate* d;

    void addOverlay( PopupDropperPrivate* newD );
    void addItem( QGraphicsSvgItem *item, bool useSharedRenderer, bool appendToList );

    bool closeAtEndOfHide;
    QStack<PopupDropperPrivate*> m_viewStack;
};

#endif //POPUPDROPPER_H
