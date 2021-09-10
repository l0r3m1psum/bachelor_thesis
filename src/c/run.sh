#!/bin/sh

general_parameters="../../res/general_params.csv"
map_from_satellite="../../res/turano_cells.csv"
initial_state="../../res/initial_state.csv"
dir_simulation_outputs="../../res/"

./main ${general_parameters} ${map_from_satellite} ${initial_state} ${dir_simulation_outputs} 
