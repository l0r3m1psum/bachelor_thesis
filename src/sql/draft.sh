#!/bin/sh
# sh draft.sh turano 455950 212100 456949 213099

# TODO
# if [ $# -lt 5 ]
# then
# 	echo usage...
# 	exit 1
# fi

debug=on
selected_map="$1"
selectedX0="$2"
selectedY0="$3"
selectedX1="$4"
selectedY1="$5"
general_parameters_file=`pwd`/general_parameters.csv
initial_state_file=`pwd`/initial_state.csv
cells_parameters_file=`pwd`/cells_parameters.csv
output_directory=`pwd`/res
db_name=test

check_num() {
	if ! [ "$1" -gt 0 ] 2>/dev/null
	then
		echo "$1" must be a number and then greater then 0
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

X0=$((selectedX0-mapX0+1))
Y0=$((selectedY0-mapY0+1))
X1=$((selectedX1-mapX0+1))
Y1=$((selectedY1-mapY0+1))

if [ "$debug" ]
then
	echo selected range: $X0, $Y0, $X1, $Y1
fi

psql -q -d $db_name -v X0=$X0 -v Y0=$Y0 -v X1=$X1 -v Y1=$Y1 -v selected_map=$selected_map <<EOF
create temporary view requested_data as 
	select (unnest(data[:Y0 : :Y1][:X0 : :X1])).*
	from maps
	where name = :'selected_map';

copy (select * from requested_data) to '$cells_parameters_file';
copy (select 4000*(
	case when forest < 255 then forest + 1 else 0 end
	-
	case when urbanization <= 100 then urbanization/100 else 0 end
), 1 from requested_data) to '$initial_state_file'
EOF

# Ora devo calcolare Wstar e Lstar in addition to L
echo "#Wstar,Lstar,h,s,seed,tau,theta,k0,k1,k2,L,Delta
$(($X1-$X0+1)),$(($Y1-$Y0+1)),100,10,123,0.001,0.4,1,1,1 ,1,1" >$general_parameters_file

# insert into simulations (
# 	name, rect, map, horizon, "snapshot freq", seed, tau, theta, k0, k1, k2, L, Delta
# ) values ();

exit 0

# ora manca generare i "dati" e i parametri, in futuro saranno messi in due file
# diversi, e lanciare il simulatore
main $general_parameters_file $cells_parameters_file $initial_state_file $output_directory
