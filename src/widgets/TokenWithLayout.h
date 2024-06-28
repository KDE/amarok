/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "widgets/Token.h"

#include <QPointer>

class LayoutEditDialog;

class Wrench : public QLabel
{
    Q_OBJECT
public:
    explicit Wrench( QWidget *parent );
protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEvent* event) override;
#else
    void enterEvent(QEnterEvent* event) override;
#endif
    void leaveEvent(QEvent *) override;
    void mousePressEvent( QMouseEvent *e ) override;
    void mouseReleaseEvent( QMouseEvent *e ) override;
    void paintEvent( QPaintEvent *pe ) override;
Q_SIGNALS:
    void clicked();
};

class TokenWithLayoutFactory : public TokenFactory
{
public:
    Token * createToken( const QString &text, const QString &iconName, qint64 value, QWidget *parent = nullptr ) const override;
};

/**
An extended Token with controls for layouting the token and getting layout values for use outside the Token.

	@author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class TokenWithLayout : public Token
{
    Q_OBJECT
public:
    TokenWithLayout( const QString &text, const QString &iconName, qint64 value, QWidget *parent = nullptr );
    ~TokenWithLayout() override;

    Qt::Alignment alignment();
    bool bold() const;
    bool italic() const;
    bool underline() const;
    inline qreal size() const { return width(); }
    qreal width() const;

    /** Return true if the width is determined by width(). It's automatic otherwise */
    inline bool widthForced() const { return m_width > 0.0; }

    /** Returns the text that is added to the front of the value */
    inline QString prefix() const { return m_prefix; }

    /** Returns the text that is added to the back of the value such as "%" */
    inline QString suffix() const { return m_suffix; }

public Q_SLOTS:
    void setAlignment( Qt::Alignment alignment );
    void setBold( bool bold );
    void setItalic( bool italic );
    void setUnderline( bool underline );
    void setPrefix( const QString& );
    void setSuffix( const QString& );

    /** Set the width of the token to a percentage of the whole line.
     *
     *  @param width A number from 0 to 100. A width of >0 will
     *    automatically set "forced" to true.
     */
    void setWidth( int width );

protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEvent* event) override;
#else
    void enterEvent(QEnterEvent* event) override;
#endif
    bool eventFilter( QObject*, QEvent* ) override;
    void leaveEvent(QEvent *) override;
    void timerEvent( QTimerEvent* ) override;

private Q_SLOTS:
    void showConfig();

private:

    Qt::Alignment m_alignment;
    bool m_bold;
    bool m_italic;
    bool m_underline;
    qreal m_width;
    QString m_prefix, m_suffix;
    Wrench *m_wrench;
    int m_wrenchTimer;
    static QPointer<LayoutEditDialog> m_dialog;

};

#endif
