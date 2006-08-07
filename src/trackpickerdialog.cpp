/***************************************************************************
    begin                : Sat Sep 6 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler
    email                : wheeler@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <config.h>

#if HAVE_TUNEPIMP

#include <qlabel.h>

#include <kapplication.h>
#include <klistview.h>
#include <klocale.h>

#include "trackpickerdialog.h"
#include "trackpickerdialogbase.h"

#define NUMBER(x) (x == 0 ? QString::null : QString::number(x))

class TrackPickerItem : public KListViewItem
{
public:
    TrackPickerItem(KListView *parent, const KTRMResult &result) :
        KListViewItem(parent, parent->lastChild(),
                      result.title(), result.artist(), result.album(),
                      NUMBER(result.track()), NUMBER(result.year())),
                      m_result(result) {
//QString year;
//if(result.year() == QString::empty()) year = "xx";
//else year = result.year();
//setText(5,"xx");
}
    KTRMResult result() const { return m_result; }

private:
    KTRMResult m_result;
};

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

TrackPickerDialog::TrackPickerDialog(const QString &name, const KTRMResultList &results, QWidget *parent)
        : KDialogBase(parent, name.latin1(), true, QString::null, Ok | Cancel, Ok, true)
{
    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n("MusicBrainz Results") ) );

    m_base = new TrackPickerDialogBase(this);
    setMainWidget(m_base);
    //setSorting with a column that doesn't exist means manual sorting only
    m_base->trackList->setSorting( 5 );

#if HAVE_TUNEPIMP >= 5
    //remove year column, there won't be any results anyway
    m_base->trackList->removeColumn( 4 );
#endif

    m_base->fileLabel->setText(name);
    KTRMResultList::ConstIterator end( results.end() );
    for(KTRMResultList::ConstIterator it = results.begin(); it != end; ++it)
        new TrackPickerItem(m_base->trackList, *it);
//     for( int j = 0; j < 5; j++ ) {
//             m_base->trackList->adjustColumn( j );
//         //resultTable->setColumnStretchable(j,true);
//     }
    m_base->trackList->setSelected(m_base->trackList->firstChild(), true);
  //  m_base->trackList->triggerUpdate();
    setMinimumWidth(kMax(300, width()));
    connect(this, SIGNAL( sigSelectionMade( KTRMResult ) ), parent, SLOT( fillSelected( KTRMResult ) ) );

}


KTRMResult
TrackPickerDialog::result() const
{
    if(m_base->trackList->selectedItem())
        return static_cast<TrackPickerItem *>(m_base->trackList->selectedItem())->result();
    else
        return KTRMResult();
}


void
TrackPickerDialog::accept()
{
    emit sigSelectionMade( result() );
    KDialog::accept();
}


#include "trackpickerdialog.moc"

#endif // HAVE_TUNEPIMP
