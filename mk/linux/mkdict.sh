# This is a shitty script to create a rough zlib dictonary from a file.
# Ideally, you should give it either a saved game file or a ton of XML from a
# real live network game (i.e., the XML from the network logs) and it will
# output a dictionary file with the most commonly used values at the end of
# the file (like zlib wants them).  Note that numeric values are stripped
# because we don't expect them to be reliable values.  It may also be useful
# to include some words from all installed faction trees, spedifically:
# * unit names
# * skill names
# * command names
# * effect names

crap() {
	tmp_file=/tmp/gae_mkdict.sh.$$
	grep -v 'data value' $1 |
	perl -pe '
		s/\" / \" /g;
		s/\"\>/ \"\>/g;
		s/=\"/=\" /g;
		s:\"/: \"/:g;
		s/\s+/\n/g' |
	egrep -v '^[-0-9]' > $tmp_file
	for word in $(sort -u $tmp_file); do
		count=$(eval grep "'^${word}$'" $tmp_file|wc -l)
		echo "$count ${word}"
	done
	rm $tmp_file
}

crap $1 |sort -n|awk '{print $2}'
