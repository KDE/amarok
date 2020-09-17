#!/bin/bash
DATENOW=$(date "+%s"); for i in 18.04_bionic 19.10_eoan 20.04_focal; do version=$(echo $i|cut -d_ -f1); code=$(echo $i|cut -d_ -f2); cat << EOF > ${version}/new_cl_entry; cat ${version}/new_cl_entry ${version}/debian/changelog > ${version}/new_cl; mv ${version}/new_cl ${version}/debian/changelog; rm ${version}/new_cl*; done
amarok (2:2.9.70+1SNAPSHOT$(date -d "@${DATENOW}" +"%Y%m%d%H%M%S%z")-0ubuntu1ppa1~ubuntu${version}.1) ${code}; urgency=medium

  * Fix QIODevice::OpenMode values that are not present at QT 5.8 

 -- Pedro de Carvalho Gomes <pedrogomes81@gmail.com>  $(date -R -d "@${DATENOW}")

EOF
