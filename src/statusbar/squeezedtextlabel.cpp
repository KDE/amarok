/***************************************************************************
 *   Copyright (C) 2000 Ronny Standtke <Ronny.Standtke@gmx.de>             *
 *             (C) 2005 Gábor Lehel <illissius@gmail.com>                  *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "squeezedtextlabel.h"
#include <qsimplerichtext.h>
#include <qwindowdefs.h> //QCOORD_MAX
#include <qtooltip.h>
#include <kdebug.h>

namespace KDE {

SqueezedTextLabel::SqueezedTextLabel( const QString &text , QWidget *parent, const char *name )
 : QLabel ( parent, name ) {
  setSizePolicy(QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
  fullText = text;
  squeezeTextToLabel();
}

SqueezedTextLabel::SqueezedTextLabel( QWidget *parent, const char *name )
 : QLabel ( parent, name ) {
  setSizePolicy(QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
}

void SqueezedTextLabel::resizeEvent( QResizeEvent * ) {
  squeezeTextToLabel();
}

QSize SqueezedTextLabel::minimumSizeHint() const
{
  QSize sh = QLabel::minimumSizeHint();
  sh.setWidth(-1);
  return sh;
}

QSize SqueezedTextLabel::sizeHint() const
{
  return QSize(contentsRect().width(), QLabel::sizeHint().height());
}

void SqueezedTextLabel::setText( const QString &text ) {
  fullText = text;
  squeezeTextToLabel();
}


//QSimpleRichText suck
//in more detail, we only want the widthUsed(), which doesn't normally work, see below
class MySimpleRichText: public QSimpleRichText
{
    public:
    MySimpleRichText( const QString &text, const QFont &font )
        : QSimpleRichText( text, font )
    {
        setWidth( QCOORD_MAX ); //by default it's like 150-something, always. wtf?
    }
};

void SqueezedTextLabel::squeezeTextToLabel()
{
    if( MySimpleRichText( fullText, font() ).widthUsed() > width() )
    {
        QString text;
        const int w = width() - fontMetrics().width( "..." );
        if( w < 0 )
        {
            text = "..";
            while( fontMetrics().width( text ) > width() && !text.isEmpty() )
                text.remove( text.length() - 1, 1);
        }
        else
        {
            text = fullText;
            do
            {
                int pos = text.length() - 1;
                bool breakagain = false; //isn't there a better way to handle this?
                while( text[pos] == '>' ) //don't remove parts of <tags>, it's not nice
                {
                    const int lpos = pos;
                    while( text[pos] != '<' && pos >= 0  )
                        --pos;
                    if( pos == 0 ) //text is only tags
                    {
                        breakagain = true;
                        break;
                    }
                    else
                        --pos;
                    if( pos < 0 ) //didn't find an opening <
                    {
                        pos = lpos;
                        break;
                    }
                }
                if( breakagain )
                    break;
                text.remove( pos, 1 );                                    //paranoia
            } while( MySimpleRichText( text, font() ).widthUsed() > w && !text.isEmpty() );
            text += "...";
        }
        QLabel::setText( text );
	QToolTip::remove( this );
        QToolTip::add( this, fullText );
    }
    else
    {
        QLabel::setText( fullText );
        QToolTip::remove( this );
        QToolTip::hide();
    }
}

void SqueezedTextLabel::setAlignment( int alignment )
{
  // save fullText and restore it
  QString tmpFull(fullText);
  QLabel::setAlignment(alignment);
  fullText = tmpFull;
}


}

#include "squeezedtextlabel.moc"
