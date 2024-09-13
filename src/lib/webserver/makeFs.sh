# Script that can be run from this directory to generate the needed fsdata.c file
dir=$(pwd)
python $dir/external/makefs.py -d $dir/../../SystemGUI/dist -o $dir/openace_fsdata.c

