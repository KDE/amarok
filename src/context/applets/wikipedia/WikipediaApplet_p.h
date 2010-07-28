/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef AMAROK_WIKIPEDIAAPPLET_P_H
#define AMAROK_WIKIPEDIAAPPLET_P_H

#include "WikipediaApplet.h"
#include "ui_wikipediaSettings.h"

#include <KLineEdit>
#include <KUrl>

#include <Plasma/LineEdit>
#include <Plasma/Svg>
#include <Plasma/SvgWidget>
#include <Plasma/WebView>

#include <QGraphicsSceneResizeEvent>
#include <QStack>
#include <QWebFrame>
#include <QWebPage>

class KTemporaryFile;
class WikipediaWebView;

namespace Plasma {
    class DataContainer;
    class IconWidget;
}

class WikipediaAppletPrivate
{
private:
    WikipediaApplet *const q_ptr;
    Q_DECLARE_PUBLIC( WikipediaApplet )

public:
    WikipediaAppletPrivate( WikipediaApplet *parent )
        : q_ptr( parent )
        , css( 0 )
        , dataContainer( 0 )
        , albumIcon( 0 )
        , artistIcon( 0 )
        , backwardIcon( 0 )
        , forwardIcon( 0 )
        , reloadIcon( 0 )
        , settingsIcon( 0 )
        , trackIcon( 0 )
        , webView( 0 )
        , wikipediaLabel( 0 )
        , aspectRatio( 0 )
        , isForwardHistory( false )
        , isBackwardHistory( false )
    {}
    ~WikipediaAppletPrivate() {}

    // functions
    void parseWikiLangXml( const QByteArray &xml );
    qint64 writeStyleSheet( const QByteArray &css );
    void scheduleEngineUpdate();
    void updateNavigationIcons();

    // private slots
    void _linkClicked( const QUrl &url );
    void _loadSettings();

    void _goBackward();
    void _goForward();
    void _gotoArtist();
    void _gotoAlbum();
    void _gotoTrack();

    void _switchToLang( const QString &lang );
    void _reloadWikipedia();

    void _paletteChanged( const QPalette &palette );

    void _getLangMapProgress( qint64 received, qint64 total );
    void _getLangMapFinished( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
    void _getLangMap();

    void _configureLangSelector();
    void _langSelectorItemChanged( QListWidgetItem *item );

    void _pageLoadStarted();
    void _pageLoadProgress( int progress );
    void _pageLoadFinished( bool ok );
    void _searchLineEditTextEdited( const QString &text );
    void _searchLineEditReturnPressed();

    // data members
    enum WikiLangRoles
    {
        PrefixRole = Qt::UserRole + 1,
        UrlPrefixRole = Qt::UserRole + 2,
        LanguageStringRole = Qt::UserRole + 3
    };

    KTemporaryFile *css;
    Plasma::DataContainer *dataContainer;
    Plasma::IconWidget *albumIcon;
    Plasma::IconWidget *artistIcon;
    Plasma::IconWidget *backwardIcon;
    Plasma::IconWidget *forwardIcon;
    Plasma::IconWidget *reloadIcon;
    Plasma::IconWidget *settingsIcon;
    Plasma::IconWidget *trackIcon;
    WikipediaWebView *webView;
    QStack<QUrl> historyBack;
    QStack<QUrl> historyForward;
    QUrl currentUrl;
    QStringList langList;
    TextScrollingWidget *wikipediaLabel;
    Ui::wikipediaSettings ui;
    qreal aspectRatio;
    bool isForwardHistory;
    bool isBackwardHistory;
};

class WikipediaSearchLineEdit : public Plasma::LineEdit
{
    Q_OBJECT

public:
    WikipediaSearchLineEdit( QGraphicsWidget *parent = 0 )
        : Plasma::LineEdit( parent ) {}
    ~WikipediaSearchLineEdit() {}

    void setFocus( Qt::FocusReason reason )
    {
        nativeWidget()->setFocus( reason );
    }

protected:
    void focusOutEvent( QFocusEvent *event )
    {
        hide();
        nativeWidget()->clear();
        parentWidget()->setFocus();
        event->accept();
    }

    void keyPressEvent( QKeyEvent *event )
    {
        if( event->key() == Qt::Key_Escape )
        {
            clearFocus();
            event->accept();
        }
        else
            Plasma::LineEdit::keyPressEvent( event );
    }
};

class WikipediaWebView : public Plasma::WebView
{
    Q_OBJECT

public:
    WikipediaWebView( QGraphicsItem *parent = 0 )
        : Plasma::WebView( parent )
    {
        m_lineEdit = new WikipediaSearchLineEdit( this );
        m_lineEdit->setContentsMargins( 0, 0, 0, 0 );
        m_lineEdit->setClearButtonShown( true );
        m_lineEdit->setVisible( false );

        Plasma::Svg *borderSvg = new Plasma::Svg( this );
        borderSvg->setImagePath( "widgets/scrollwidget" );

        m_topBorder = new Plasma::SvgWidget( this );
        m_topBorder->setSvg( borderSvg );
        m_topBorder->setElementID( "border-top" );
        m_topBorder->setZValue( 900 );
        m_topBorder->resize( -1, 10.0 );
        m_topBorder->show();

        m_bottomBorder = new Plasma::SvgWidget( this );
        m_bottomBorder->setSvg( borderSvg );
        m_bottomBorder->setElementID( "border-bottom" );
        m_bottomBorder->setZValue( 900 );
        m_bottomBorder->resize( -1, 10.0 );
        m_bottomBorder->show();

        page()->parent()->installEventFilter( this );
    }
    ~WikipediaWebView() {}

    Plasma::LineEdit *lineEdit()
    { return m_lineEdit; }

protected:
    bool eventFilter( QObject *obj , QEvent *event )
    {
        if( obj == page()->parent() )
        {
            if( event->type() == QEvent::KeyPress )
            {
                QKeyEvent *keyEvent = static_cast<QKeyEvent*>( event );
                keyPressEvent( keyEvent );
                return true;
            }
            return false;
        }
        return Plasma::WebView::eventFilter( obj, event );
    }

    void keyPressEvent( QKeyEvent *event )
    {
        if( event->key() == Qt::Key_Slash || event->matches( QKeySequence::Find ) )
        {
            qreal offsetX = m_lineEdit->rect().width();
            qreal offsetY = m_lineEdit->rect().height();
            offsetX += mainFrame()->scrollBarGeometry( Qt::Vertical ).width();
            m_lineEdit->setPos( rect().bottomRight() - QPointF( offsetX, offsetY ) );
            m_lineEdit->setFocus( Qt::PopupFocusReason );
            m_lineEdit->show();
            event->accept();
        }
        else
            Plasma::WebView::keyPressEvent( event );
    }

    void resizeEvent( QGraphicsSceneResizeEvent *event )
    {
        Plasma::WebView::resizeEvent( event );
        if( m_topBorder )
        {
            m_topBorder->resize( event->newSize().width(), m_topBorder->size().height() );
            m_bottomBorder->resize( event->newSize().width(), m_bottomBorder->size().height() );
            QPointF bottomPoint = boundingRect().bottomLeft();
            bottomPoint.ry() -= m_bottomBorder->size().height();
            m_bottomBorder->setPos( bottomPoint );
            m_topBorder->setPos( mapFromParent( pos() ) );
        }
    }

private:
    WikipediaSearchLineEdit *m_lineEdit;
    Plasma::SvgWidget *m_topBorder;
    Plasma::SvgWidget *m_bottomBorder;
};

#endif /* AMAROK_WIKIPEDIAAPPLET_P_H */
