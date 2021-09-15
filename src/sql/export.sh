#!/bin/sh

if [ $# -ne 3 ]
then
	echo "usage: $0 <database name> <table name> <file name>"
	exit 1
fi

dbname=${parameter:-simulator}
tbname="$2"
fname="$3"

psql -d "$simulator" -c "\\\\copy \"$table\" to '$file' with format csv;"

exit 0

select unnest(data[1:3][2:4]) from maps;
select unnest(data[1:3][2:4]) from results;
select
	array_length(rect[0], 1) as Wstar,
	array_length(rect, 2) as Lstar,
	horizon as h,
	"snapshot freq" as s
	seed,
	tau,
	theta,
	k0,
	k1,
	k2,
	L,
	Delta
from simulations
order by started desc
fetch first row only;

espressione per calcolare gamma
4000*(
	(case
		when forest < 255 then forest + 1
		else 0
	end case)
	-
	(case
		when urbanization <= 100 then urbanization/100
		else 0
	end case)
)
