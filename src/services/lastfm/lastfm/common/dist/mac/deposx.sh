#!/bin/sh
# Author: max@last.fm
# Brief:  otools a binary file that is already in the bundle
# Usage: deposx.sh target_binary $$QMAKE_LIBDIR_QT
################################################################################

if [[ $# < 2 ]]
then 
	echo "Usage: $0 [target] [QT_FRAMEWORKS_DIR]"
	exit 1
fi

QT_FRAMEWORK_DIR=$2

function isQt {
	for x in `echo $QT_FRAMEWORK_DIR/*.framework`
	do
		echo $1 | grep "^$x" && return 0;
	done
	return 1
}

otool -L $1 | while read LINE
do
	x=`echo $LINE | cut -d' ' -f1` #won't work if spaces in dirnames! which is unlikely..
	
	if [ `isQt $x` ]
	then
		module=`echo $x | sed "s|$QT_FRAMEWORK_DIR/\(.*\)\.framework/.*|\1|"`
		install_name_tool -change "$x" \
                      "@executable_path/../Frameworks/$module.framework/Versions/4/$module" \
                      "$1"
	elif [ `echo $x | grep '^[/@]'` ]
	then
		true
	else
		# if not an absolute path and not already name-tooled
		install_name_tool -change "$x" "@executable_path/$x" "$1"
	fi
done
#!/bin/sh
# Author: max@last.fm
# Brief:  otools a binary file that is already in the bundle
# Usage: deposx.sh target_binary $$QMAKE_LIBDIR_QT
################################################################################

if [[ $# < 2 ]]
then 
	echo "Usage: $0 [target] [QT_FRAMEWORKS_DIR]"
	exit 1
fi

QT_FRAMEWORK_DIR=$2

function isQt {
	for x in `echo $QT_FRAMEWORK_DIR/*.framework`
	do
		echo $1 | grep "^$x" && return 0;
	done
	return 1
}

otool -L $1 | while read LINE
do
	x=`echo $LINE | cut -d' ' -f1` #won't work if spaces in dirnames! which is unlikely..
	
	if [ `isQt $x` ]
	then
		module=`echo $x | sed "s|$QT_FRAMEWORK_DIR/\(.*\)\.framework/.*|\1|"`
		install_name_tool -change "$x" \
                      "@executable_path/../Frameworks/$module.framework/Versions/4/$module" \
                      "$1"
	elif [ `echo $x | grep '^[/@]'` ]
	then
		true
	else
		# if not an absolute path and not already name-tooled
		install_name_tool -change "$x" "@executable_path/$x" "$1"
	fi
done
