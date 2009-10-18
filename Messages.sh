#! /bin/sh

$XGETTEXT_QT utilities/collectionscanner/CollectionScanner.cpp -o $podir/amarokcollectionscanner_qt.pot
$EXTRACTATTR --attr=layout,name src/data/DefaultPlaylistLayouts.xml >> rc.cpp
