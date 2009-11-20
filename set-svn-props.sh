#!/bin/bash

textExtensions="ac cpp db dict h html ini inl jam lng m4 pl sh txt xml"
textFileNames="AUTHORS COPYING COPYRIGHTS ChangeLog Jamfile Jamrules README"
executableExtensions="sh pl"
executableFileName=""

die() {
	echo "ERROR: $@"
	exit 1
}

orIzeString() {
	echo "$@" | perl -pe 's/\s+/\|/g'
}

findFiles() {
	local extensions="$1"
	local fileNames="$2"
	local pattern=""
	if [ -z "${fileNames}" ]; then
		pattern=".*\.($(orIzeString "${extensions}"))\$"
	else
		pattern="^.*/((.*\.($(orIzeString "${extensions}")))|$(orIzeString "${fileNames}"))\$"
	fi

	find -type f |
	grep -v /.svn/ |
	egrep "${pattern}" || die 
}

svn propset svn:executable '*' $(findFiles "${executableExtensions}" "${executableFileName}")

for f in $(findFiles "${textExtensions}" "${textFileNames}"); do
	if ! svn propset svn:eol-style native $f; then
		crlf $f;
		svn propset svn:eol-style native $f || die
	fi
done

