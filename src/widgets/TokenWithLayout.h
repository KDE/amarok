/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#ifndef TOKENWITHLAYOUT_H
#define TOKENWITHLAYOUT_H

#include <QPointer>
#include <Token.h>

class LayoutEditDialog;

class Wrench : public QLabel
{
    Q_OBJECT
public:
    Wrench( QWidget *parent );
protected:
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void mousePressEvent( QMouseEvent *e );
    void mouseReleaseEvent( QMouseEvent *e );
    void paintEvent( QPaintEvent *pe );
signals:
    void clicked();
};

class TokenWithLayoutFactory : public TokenFactory
{
public:
    virtual Token * createToken( const QString &text, const QString &iconName, int value, QWidget *parent = 0 );
};

/**
An extended Token with controls for layouting the token and getting layout values for use outside the Token.

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class TokenWithLayout : public Token
{
    Q_OBJECT
public:
    TokenWithLayout( const QString &text, const QString &iconName, int value, QWidget *parent = 0 );
    ~TokenWithLayout();

    Qt::Alignment alignment();
    void setAlignment( Qt::Alignment alignment );

    bool bold() const;
    bool italic() const;
    bool underline() const;
    inline qreal size() const { return width(); }
    qreal width() const;
    inline bool widthForced() const { return m_widthForced; }
    inline QString prefix() const { return m_prefix; }
    inline QString suffix() const { return m_suffix; }

public slots:
    void setAlignLeft( bool );
    void setAlignCenter( bool );
    void setAlignRight( bool );
    void setBold( bool bold );
    void setItalic( bool italic );
    void setUnderline( bool underline );
    void setPrefix( const QString& );
    void setSuffix( const QString& );
    void setWidth( int width );
    void setWidthForced( bool );

protected:
    virtual void enterEvent(QEvent *);
    virtual bool eventFilter( QObject*, QEvent* );
    virtual void leaveEvent(QEvent *);
    virtual void timerEvent( QTimerEvent* );

private slots:
    void showConfig();

private:

    Qt::Alignment m_alignment;
    bool m_bold;
    bool m_italic;
    bool m_underline;
    bool m_widthForced;
    qreal m_width;
    QString m_prefix, m_suffix;
    Wrench *m_wrench;
    int m_wrenchTimer;
    static QPointer<LayoutEditDialog> m_dialog;

};

#endif
