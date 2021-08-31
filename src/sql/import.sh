#!/bin/sh

if [ $# -ne 3 ]
then
	echo "usage: $0 <database name> <table name> <file name>"
	exit 1
fi

dbname=${parameter:-simulator}
tbname="$2"
fname="$3"

psql -d "$simulator" -c "\\\\copy \"$tbname\" from '$fname' with format csv;"
