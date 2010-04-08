#!/bin/bash

VERSION=`autoconf -t AC_INIT | sed -e 's/[^:]*:[^:]*:[^:]*:[^:]*:\([^:]*\):.*/\1/g'`
RELEASENAME=glestae-source
RELEASEDIR="`pwd`/release/$RELEASENAME-$VERSION"

echo "Creating source package in $RELEASEDIR"

rm -rf $RELEASEDIR
mkdir -p $RELEASEDIR
# copy sources
pushd "`pwd`/../../source"
find game/ \( -name "*.cpp" -o -name "*.h" -o -name "*.inl" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find shared_lib/ \( -name "*.cpp" -o -name "*.h" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find map_editor/ \( -name "*.cpp" -o -name "*.h" -o -name "*.xpm" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find g3d_viewer/ \( -name "*.cpp" -o -name "*.h" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find test/ \( -name "*.cpp" -o -name "*.h" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
popd
AUTOCONFSTUFF="configure.ac autogen_pkg.sh Jamrules Jamfile `find mk/jam -name "*.jam"` `find mk/autoconf -maxdepth 1 -name "*.m4" -o -name "config.*" -o -name "*sh"`"

cp -p --parents $AUTOCONFSTUFF $RELEASEDIR
cp -p ../../docs/README* ../../docs/license.txt $RELEASEDIR
cp -p glestadv.ini $RELEASEDIR
#remove backup files
find $RELEASEDIR -name "*~" | xargs rm

pushd $RELEASEDIR
./autogen_pkg.sh
popd

pushd release
PACKAGE="$RELEASENAME-$VERSION.tar.bz2"
echo "creating $PACKAGE"
tar -c --bzip2 -f "$PACKAGE" "$RELEASENAME-$VERSION"
popd



