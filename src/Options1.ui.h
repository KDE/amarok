//Released under GPLv2 or later. (C) 2005 Ian Monroe <ian@monroe.nu>
/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/

#include <config.h>

#include "amarokconfig.h"
#include "moodbar.h"
#include "starmanager.h"

#include <kcolordialog.h>
#include <kstandarddirs.h>

void Options1::init()
{
    slotUpdateMoodFrame();
    //kcfg_CustomRatingsColors->setChecked( AmarokConfig::customRatingsColors() );
    //slotUpdateRatingsFrame();

    QStringList browsers;
    browsers << "konqueror" << "firefox" << "opera" << "galeon" << "epiphany"
             << "safari" << "mozilla";

    // Remove browsers which are not actually installed
    for( QStringList::Iterator it = browsers.begin(), end = browsers.end(); it != end; ) {
        if( KStandardDirs::findExe( *it ).isEmpty() )
            it = browsers.erase( it );
        else
            ++it;
    }
#ifdef Q_WS_MAC
    if ( KStandardDirs::findExe( "open" ) != QString::null )
        browsers.prepend( i18n( "Default Browser" ) );
#else
    if ( KStandardDirs::findExe( "kfmclient" ) != QString::null )
        browsers.prepend( i18n( "Default KDE Browser" ) );
#endif

    kComboBox_browser->insertStringList( browsers );
    kLineEdit_customBrowser->setText( AmarokConfig::externalBrowser() );
    int index = browsers.findIndex( AmarokConfig::externalBrowser() );
    if( index >= 0 )
        kComboBox_browser->setCurrentItem( index );
    else if( AmarokConfig::externalBrowser() ==
#ifdef Q_WS_MAC
            "open"
#else
            "kfmclient openURL"
#endif
      )
    {
        kComboBox_browser->setCurrentItem( 0 );
    }
    else
    {
        checkBox_customBrowser->setChecked( true );
    }
}

void Options1::slotUpdateMoodFrame()
{
    if( Moodbar::executableExists() )
    {
        moodbarHelpLabel->hide();
        moodFrame->setEnabled(true);

        kcfg_MakeMoodier->setEnabled(kcfg_ShowMoodbar->isChecked());
        kcfg_AlterMood->setEnabled(kcfg_ShowMoodbar->isChecked() && kcfg_MakeMoodier->isChecked());
        kcfg_MoodsWithMusic->setEnabled(kcfg_ShowMoodbar->isChecked());
    }

    else
    {
        moodbarHelpLabel->show();
        kcfg_ShowMoodbar->setChecked(false);
        moodFrame->setEnabled(false);
    }
}

/*
void Options1::slotUpdateRatingsFrame()
{
    kcfg_CustomRatingsColors->setEnabled( kcfg_UseRatings->isChecked() );
    bool enableStars = kcfg_UseRatings->isChecked() && kcfg_CustomRatingsColors->isChecked();
    kcfg_FixedHalfStarColor->setEnabled( enableStars );

    AmarokConfig::setCustomRatingsColors( enableStars );

    StarManager::instance()->reinitStars();
    ratingsFrame->setEnabled( enableStars );

    fivestar_1->setPixmap( *StarManager::instance()->getStar( 5 ) );
    fivestar_2->setPixmap( *StarManager::instance()->getStar( 5 ) );
    fivestar_3->setPixmap( *StarManager::instance()->getStar( 5 ) );
    fivestar_4->setPixmap( *StarManager::instance()->getStar( 5 ) );
    fivestar_5->setPixmap( *StarManager::instance()->getStar( 5 ) );
    fivestar_1->setEnabled( enableStars );
    fivestar_2->setEnabled( enableStars );
    fivestar_3->setEnabled( enableStars );
    fivestar_4->setEnabled( enableStars );
    fivestar_5->setEnabled( enableStars );

    fourstar_1->setPixmap( *StarManager::instance()->getStar( 4 ) );
    fourstar_2->setPixmap( *StarManager::instance()->getStar( 4 ) );
    fourstar_3->setPixmap( *StarManager::instance()->getStar( 4 ) );
    fourstar_4->setPixmap( *StarManager::instance()->getStar( 4 ) );
    fourstar_1->setEnabled( enableStars );
    fourstar_2->setEnabled( enableStars );
    fourstar_3->setEnabled( enableStars );
    fourstar_4->setEnabled( enableStars );

    threestar_1->setPixmap( *StarManager::instance()->getStar( 3 ) );
    threestar_2->setPixmap( *StarManager::instance()->getStar( 3 ) );
    threestar_3->setPixmap( *StarManager::instance()->getStar( 3 ) );
    threestar_1->setEnabled( enableStars );
    threestar_2->setEnabled( enableStars );
    threestar_3->setEnabled( enableStars );

    twostar_1->setPixmap( *StarManager::instance()->getStar( 2 ) );
    twostar_2->setPixmap( *StarManager::instance()->getStar( 2 ) );
    twostar_1->setEnabled( enableStars );
    twostar_2->setEnabled( enableStars );

    onestar_1->setPixmap( *StarManager::instance()->getStar( 1 ) );
    onestar_1->setEnabled( enableStars );

    halfstar->setPixmap( *StarManager::instance()->getHalfStar() );
    halfstar->setEnabled( enableStars );
}

void Options1::slotFixedHalfStarColor()
{
    bool checked = kcfg_FixedHalfStarColor->isChecked();
    AmarokConfig::setFixedHalfStarColor( kcfg_FixedHalfStarColor->isChecked() );
    slotUpdateRatingsFrame();
}

void Options1::slotPickColorHalf()
{
    QColor halfStar;
    int result = KColorDialog::getColor( halfStar );
    if( result == KColorDialog::Accepted )
    {
        AmarokConfig::setStarColorHalf( halfStar );
        StarManager::instance()->setHalfColor( halfStar );
        slotUpdateRatingsFrame();
    }
}

void Options1::slotPickColorOne()
{
    QColor oneStar;
    int result = KColorDialog::getColor( oneStar );
    if( result == KColorDialog::Accepted )
    {
        AmarokConfig::setStarColorOne( oneStar );
        StarManager::instance()->setColor( 1, oneStar );
        slotUpdateRatingsFrame();
    }
}

void Options1::slotPickColorTwo()
{
    QColor twoStar;
    int result = KColorDialog::getColor( twoStar );
    if( result == KColorDialog::Accepted )
    {
        AmarokConfig::setStarColorTwo( twoStar );
        StarManager::instance()->setColor( 2, twoStar );
        slotUpdateRatingsFrame();
    }
}

void Options1::slotPickColorThree()
{
    QColor threeStar;
    int result = KColorDialog::getColor( threeStar );
    if( result == KColorDialog::Accepted )
    {
        AmarokConfig::setStarColorThree( threeStar );
        StarManager::instance()->setColor( 3, threeStar );
        slotUpdateRatingsFrame();
    }
}

void Options1::slotPickColorFour()
{
    QColor fourStar;
    int result = KColorDialog::getColor( fourStar );
    if( result == KColorDialog::Accepted )
    {
        AmarokConfig::setStarColorFour( fourStar );
        StarManager::instance()->setColor( 4, fourStar );
        slotUpdateRatingsFrame();
    }
}

void Options1::slotPickColorFive()
{
    QColor fiveStar;
    int result = KColorDialog::getColor( fiveStar );
    if( result == KColorDialog::Accepted )
    {
        AmarokConfig::setStarColorFive( fiveStar );
        StarManager::instance()->setColor( 5, fiveStar );
        slotUpdateRatingsFrame();
    }
}
*/
