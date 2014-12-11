#!/bin/bash

# Some variable names

exec='mpiexec -n 8'
Castro='Castro3d.Linux.g++.gfortran.MPI.ex'
inputs='inputs'
probin='probin'

# Define a function that moves all of the output
# data from a Castro run to the directory in the first argument.

function move_results {

  if [ -d "$1" ]; then
    rm -rf $1/
  fi
  mkdir $1
  mv plt* $1/
  mv *.out $1/
  cp $inputs $1/
  cp $probin $1/

}

# Check if results directory already exists, and if not then create it.

results_dir=results

if [ ! -d $results_dir ]; then
  mkdir $results_dir
fi

# Loop over the resolutions in question

for problem in 1 2
do
  echo "Now doing problem =" $problem
  sed -i "/problem/c problem = $problem" $probin

  if [ ! -d $results_dir/problem$problem ]; then
      mkdir $results_dir/problem$problem
  fi

  for ncell in 16 32 64
  do
    dir=$results_dir/problem$problem/$ncell
    if [ ! -d $dir ]; then
      mkdir $dir
      echo "Now doing ncell =" $ncell
      sed -i "/amr.n_cell/c amr.n_cell = $ncell $ncell $ncell" $inputs
      $exec $Castro $inputs > info.out
      move_results $dir
    fi
  done
done