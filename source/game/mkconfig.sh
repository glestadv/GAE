#!/bin/bash
# Written by Daniel Santos (2009) for Glest Advanced Engine
# Script to regenerate global/config.h and global/confg.cpp from config.db
# This may not be the cleanest solution in the world, but it's better than
# hasseling with making 5 changes everytime you add, remove or edit a config
# entry.

CONFIG_VARIABLES=""
CONFIG_GETTERS=""
CONFIG_SETTERS=""
CONFIG_INIT=""
CONFIG_SAVE=""

input_h=global/config.h.template
input_cpp=global/config.cpp.template
output_h=global/config.h
output_cpp=global/config.cpp
input_file=config.db

die() {
	echo "ERROR $*"
	exit
}

CONFIG_VARIABLES="$(egrep -v '^$|^#' $input_file | sort -u | awk '{
	print "\t" $2 " " $1 ";"
}')" || die

CONFIG_GETTERS="$(egrep -v '^$|^#' $input_file | sort -u | awk '{
	print "\t" $2 " get" toupper(substr($1, 1, 1)) substr($1, 2) \
		  "() const" \
		  (substr("\t\t\t\t\t\t\t", (length($1) + length($2)) / 4)) \
		  "{return " $1 ";}"
}')" || die

CONFIG_SETTERS="$(egrep -v '^$|^#' $input_file | sort -u | awk '{
	print "\tvoid set" toupper(substr($1, 1, 1)) substr($1, 2) \
		  "(" $2 " val)" \
		  (substr("\t\t\t\t\t\t\t", (length($1) + length($2) + 2) / 4)) \
		  "{" $1 " = val;}"
}')" || die

CONFIG_INIT="$(egrep -v '^$|^#' $input_file | sort -u | awk '{
	print "\t" $1 " = p->get" toupper(substr($2, 1, 1)) substr($2, 2) \
		  "(\"" toupper(substr($1, 1, 1)) substr($1, 2) \
		  "\"" \
		  ($3 != "-" ? ", " $3 : "") \
		  ($4 != "-" ? ", " $4 : "") \
		  ($5 != "-" ? ", " $5 : "") \
		  ");"
}')" || die

CONFIG_SAVE="$(egrep -v '^$|^#' $input_file | sort -u | awk '{
	print "\tp->set" toupper(substr($2, 1, 1)) substr($2, 2) \
		  "(\"" toupper(substr($1, 1, 1)) substr($1, 2) "\", " $1 ");"
}')" || die

perl -pe "s/CONFIG_VARIABLES/$CONFIG_VARIABLES/g" $input_h |
perl -pe "s/CONFIG_GETTERS/$CONFIG_GETTERS/g" |
perl -pe "s/CONFIG_SETTERS/$CONFIG_SETTERS/g" > $output_h

perl -pe "s/CONFIG_INIT/$CONFIG_INIT/g" $input_cpp |
perl -pe "s/CONFIG_SAVE/$CONFIG_SAVE/g" > $output_cpp

