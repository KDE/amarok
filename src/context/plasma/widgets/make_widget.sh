#!/bin/sh

# usage: make_widget LineEdit

LOWERNAME=`echo $1 | tr [:upper:] [:lower:]`
NAME=$1
CAPNAME=`echo $1 | tr [:lower:] [:upper:]`
NATIVE="Q${NAME}"
HEADER="${LOWERNAME}.h"
SOURCE="${LOWERNAME}.cpp"
BOTH="$HEADER $SOURCE"
QHEADER="Q${HEADER}"

cp template.h $HEADER
cp template.cpp $SOURCE

perl -pi -e "s,<Name>,${NAME},g" $BOTH
perl -pi -e "s,<NAME>,${CAPNAME},g" $BOTH
perl -pi -e "s,<name>,${LOWERNAME},g" $BOTH
perl -pi -e "s,<Native>,$NATIVE,g" $BOTH

echo "#include \"../../plasma/${HEADER}\"" > ../includes/${NAME}

svn add ../includes/${NAME} $BOTH

