#!/bin/sh

if [ $# -ne 2 ]
then
	echo "usage: $0 <database name> <schema file>"
	exit 1
fi

dbname=${parameter:-simulator}
schema="$1"

psql -v db="$dbname" "$shema"
