#!/bin/bash

gen_vs_sln()
{
    qmake -recursive -tp vc Last.fm.pro "CONFIG -= flat" || return $?
    
    FIRST_BIT='
Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "qmake", "qmake", "{780C13D3-8172-4EE0-8FD5-26ED6489851C}"
	ProjectSection(SolutionItems) = preProject'
    
    for x in `common/qmake/findSources.pl pro` .qmake.cache
    do
        FIRST_BIT="$FIRST_BIT
		$x=$x"
    done

    LAST_BIT='
	EndProjectSection
EndProject
Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "common", "common", "{3FDD67B7-DF67-4F22-8545-755D722794BC}"
	ProjectSection(SolutionItems) = preProject
		common\qrc\common.qrc = common\qrc\common.qrc
		lib\DllExportMacro.h = lib\DllExportMacro.h
	EndProjectSection
EndProject'

    echo "$FIRST_BIT$LAST_BIT" >> Last.sln
}


gen_xcodeproj() 
{
    qmake -config debug -spec macx-xcode Last.fm.pro || return $?
    # the only way to add the QtOverride stuff to xcodeprojects, which suggests
    # we should lose it
    perl -pi -e 's|HEADER_SEARCH_PATHS = \(|HEADER_SEARCH_PATHS = \( common/qt/override,|g' *.xcodeproj/project.pbxproj
}
#!/bin/bash

gen_vs_sln()
{
    qmake -recursive -tp vc Last.fm.pro "CONFIG -= flat" || return $?
    
    FIRST_BIT='
Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "qmake", "qmake", "{780C13D3-8172-4EE0-8FD5-26ED6489851C}"
	ProjectSection(SolutionItems) = preProject'
    
    for x in `common/qmake/findSources.pl pro` .qmake.cache
    do
        FIRST_BIT="$FIRST_BIT
		$x=$x"
    done

    LAST_BIT='
	EndProjectSection
EndProject
Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "common", "common", "{3FDD67B7-DF67-4F22-8545-755D722794BC}"
	ProjectSection(SolutionItems) = preProject
		common\qrc\common.qrc = common\qrc\common.qrc
		lib\DllExportMacro.h = lib\DllExportMacro.h
	EndProjectSection
EndProject'

    echo "$FIRST_BIT$LAST_BIT" >> Last.sln
}


gen_xcodeproj() 
{
    qmake -config debug -spec macx-xcode Last.fm.pro || return $?
    # the only way to add the QtOverride stuff to xcodeprojects, which suggests
    # we should lose it
    perl -pi -e 's|HEADER_SEARCH_PATHS = \(|HEADER_SEARCH_PATHS = \( common/qt/override,|g' *.xcodeproj/project.pbxproj
}
