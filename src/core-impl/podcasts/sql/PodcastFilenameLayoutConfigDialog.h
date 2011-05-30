#ifndef AMAROK_PODCASTFILENAMELAYOUTCONFIGDIALOG_H
#define AMAROK_PODCASTFILENAMELAYOUTCONFIGDIALOG_H

#include "SqlPodcastMeta.h"

#include <KDialog>

namespace Ui
{
class PodcastFilenameLayoutConfigWidget;
}

class PodcastFilenameLayoutConfigDialog : public KDialog
{

    Q_OBJECT
public:
    explicit PodcastFilenameLayoutConfigDialog( Podcasts::SqlPodcastChannelPtr channel, QWidget* parent = 0 );
    bool configure();

protected slots:
    void slotApply();

private:
    void init();
    Podcasts::SqlPodcastChannelPtr m_channel;
    Ui::PodcastFilenameLayoutConfigWidget *m_pflc;    
    int choice;
};

#endif