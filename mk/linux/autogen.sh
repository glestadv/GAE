#!/bin/sh

rm -f configure Jamconfig.in build shared_lib glest_game glest_map_editor test data docs maps scenarios techs tilesets

if [ "$1" = "clean" ]; then
	exit
fi

# Correct working directory?
if test ! -f configure.ac ; then
  echo "*** Please invoke this script from directory containing configure.ac."
  exit 1
fi

echo "aclocal..."
#autoheader
aclocal -I mk/autoconf

# generate Jamconfig.in
echo "generating Jamconfig.in ..."
autoconf --trace=AC_SUBST \
  | sed -e 's/configure.ac:[0-9]*:AC_SUBST:\([^:]*\).*/\1 ?= "@\1@" ;/g' \
  > Jamconfig.in
sed -e 's/.*BACKSLASH.*//' -i~ Jamconfig.in
rm Jamconfig.in~
echo 'INSTALL ?= "@INSTALL@" ;' >> Jamconfig.in
echo 'JAMCONFIG_READ = yes ;' >> Jamconfig.in

echo "autoconf"
autoconf

rm -rf autom4te.cache build

mkdir -p /tmp/$(whoami)/gae/0.2.12
ln -s /tmp/$(whoami)/gae/0.2.12 build

# create symlinks to the source dirs

echo "Updating Source symlinks..."

for f in data docs maps scenarios techs tilesets; do
	ln -sf ../../data/game/$f .;
done

ln -sf ../../source/shared_lib .
ln -sf ../../source/glest_game .
ln -sf ../../source/glest_map_editor .

