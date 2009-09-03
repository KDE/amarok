/****************************************************************************************
 * Copyright (c) 2002 KFile Authors <kde@kde.org>                                       *
 * Copyright (c) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>                         *
 * Copyright (c) 2007 Mirko Stocker <me@misto.ch>                                       *
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

#ifndef AMAROK_KBOOKMARKHANDLER_H_
#define AMAROK_KBOOKMARKHANDLER_H_

#include <KBookmarkManager>
#include <KBookmarkMenu>
#include <QTextStream>
#include <QByteArray>


class QTextStream;
class KMenu;

namespace FileBrowser {
  class Widget;
}

class KBookmarkHandler : public QObject, public KBookmarkOwner
{
    Q_OBJECT

  public:
    explicit KBookmarkHandler( FileBrowser::Widget *parent, KMenu* kpopupmenu = 0 );
    ~KBookmarkHandler();

    // KBookmarkOwner interface:
    virtual QString currentUrl() const;
    virtual QString currentTitle() const;

    KMenu *menu() const
    {
      return m_menu;
    }
    virtual void openBookmark( const KBookmark &, Qt::MouseButtons, Qt::KeyboardModifiers );

  signals:
    void openUrl( const KUrl& url );

  private slots:
    void slotNewBookmark( const QString& text, const QByteArray& url,
                          const QString& additionalInfo );
    void slotNewFolder( const QString& text, bool open,
                        const QString& additionalInfo );
    void newSeparator();
    void endFolder();

  private:
    FileBrowser::Widget *mParent;
    KMenu *m_menu;
    KBookmarkMenu *m_bookmarkMenu;

    QTextStream *m_importStream;

    //class KBookmarkHandlerPrivate *d;
};

#endif // _KBOOKMARKHANDLER_H_
// kate: space-indent on; indent-width 2; replace-tabs on;

