#!/bin/bash

DATENOW=$(date "+%s");
declare -A VERSION_CODENAME
VERSION_CODENAME[18.04]="bionic"
VERSION_CODENAME[20.04]="focal"
VERSION_CODENAME[22.04]="jammy"
VERSION_CODENAME[23.04]="lunar"

COMMENT=${1:-Release of v${DATENOW}}

for version in ${!VERSION_CODENAME[@]}; do
  code=${VERSION_CODENAME[${version}]}
  cat << EOF > ${version}/new_cl_entry;
amarok (2:2.9.71+1SNAPSHOT$(date -d "@${DATENOW}" +"%Y%m%d%H%M%S%z")-0ubuntu1ppa1~ubuntu${version}.1) ${code}; urgency=medium

  * ${COMMENT}

 -- Pedro de Carvalho Gomes <pedrogomes81@gmail.com>  $(date -R -d "@${DATENOW}")

EOF
 cat ${version}/new_cl_entry ${version}/debian/changelog > ${version}/new_cl
 mv ${version}/new_cl ${version}/debian/changelog
 rm ${version}/new_cl*;
done

