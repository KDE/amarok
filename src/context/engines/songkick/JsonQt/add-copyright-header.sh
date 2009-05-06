#!/bin/sh
NOTICE_HEADER="/* LICENSE NOTICE"
TMPFILE=$(mktemp)
for file in $(find -name *.cpp -o -name *.h); do
	if ! grep -q "$NOTICE_HEADER" "$file"; then
		echo "$NOTICE_HEADER" > $TMPFILE
		cat COPYING.HEADER >> $TMPFILE
		echo "*/" >> $TMPFILE
		cat "$file" >> $TMPFILE
		cat $TMPFILE > "$file"
	fi
done
rm $TMPFILE
