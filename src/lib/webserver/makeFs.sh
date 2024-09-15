# Script that can be run from this directory to generate the needed openace_fsdata.c file for teh WebGUI
dir=$(pwd)
python $dir/external/makefs.py -d $dir/../../SystemGUI/dist -o $dir/openace_fsdata.c

