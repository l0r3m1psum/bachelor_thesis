#!/bin/sh
# sh draft.sh turano 4559500 2121000 4569490 2130990
# sh draft.sh turano 4559500 2121000 4569490 2130980

if [ $# -lt 5 ]
then
	echo usage $0 '<map name> <x0> <y0> <x1> <y1>'
	exit 1
fi

debug=
selected_map="$1" # name of the map in the database
selectedX0="$2" # abscissa of the left bottom point in meters
selectedY0="$3" # ordinate of the left bottom point in meters
selectedX1="$4" # abscissa of the top right point in meters
selectedY1="$5" # ordinate of the top right point in meters
general_parameters_file=`pwd`/general_parameters.csv
initial_state_file=`pwd`/initial_state.csv
cells_parameters_file=`pwd`/cells_parameters.csv
output_directory=`pwd`/res
db_name=test
# unit of measure for the coordinates of the rectangle stored in the database,
# done to avoid coordinates that are not multiple of the unit of measure
unit=`echo select unit from maps where name = :\'map_name\' \
	| psql -d $db_name -P tuples_only -v map_name=$selected_map`

if [ "$debug" ]
then
	echo unit: $unit
fi

check_num() {
	if [ "$1" -le 0 ] 2>/dev/null
	then
		echo "$1" must be a number and then greater then 0
		exit 1
	fi
	if [ $(($1%$unit)) -ne 0 ]
	then
		echo $1 must be an even multiple of $unit
		exit 1
	fi
}

check_num "$selectedX0"
check_num "$selectedY0"
check_num "$selectedX1"
check_num "$selectedY1"

if [ $selectedX0 -ge $selectedX1 ]
then
	echo X0: $X0 must be less the X1: $X1
	exit 1
fi

if [ $selectedY0 -ge $selectedY1 ]
then
	echo Y0: $Y0 must be less the Y1: $Y1
	exit 1
fi

map_rect=`echo select '(rect).*' from maps where name = :\'map_name\' \
	| psql -d $db_name -P tuples_only -P format=csv -v map_name=$selected_map`

if [ "$debug" ]
then
	echo map rectangle $map_rect
fi

mapX0=`echo $map_rect | cut -d, -f1`
mapY0=`echo $map_rect | cut -d, -f2`

X0=$((selectedX0/$unit-mapX0+1))
Y0=$((selectedY0/$unit-mapY0+1))
X1=$((selectedX1/$unit-mapX0+1))
Y1=$((selectedY1/$unit-mapY0+1))

if [ "$debug" ]
then
	echo selected range: $X0, $Y0, $X1, $Y1
fi

psql -q -d $db_name -v X0=$X0 -v Y0=$Y0 -v X1=$X1 -v Y1=$Y1 -v selected_map=$selected_map <<EOF
create temporary view requested_data as 
	select (unnest(data[:Y0 : :Y1][:X0 : :X1])).*
	from maps
	where name = :'selected_map';

copy (select
		altimetry,
		forest,
		urbanization,
		water1,
		cast(water2 as int),
		"carta natura",
		"wind angle",
		"wind speed"
	from requested_data) to '$cells_parameters_file' with (format csv);
copy (select 4000*(
	case when forest < 255 then forest + 1 else 0 end
	-
	case when urbanization <= 100 then cast(urbanization as float4)/100 else 0 end
), 1 from requested_data) to '$initial_state_file' with (format csv);
EOF

echo "#Wstar,Lstar,h,s,seed,tau,theta,k0,k1,k2,L,Delta
$(($X1-$X0+1)),$(($Y1-$Y0+1)),100,10,123,0.001,0.4,1,1,1 ,$unit,1" >$general_parameters_file

# insert into simulations (
# 	name, rect, map, horizon, "snapshot freq", seed, tau, theta, k0, k1, k2, L, Delta
# ) values ();
