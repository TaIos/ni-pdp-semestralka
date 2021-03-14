#!/usr/bin/bash

PROGRAM='./run.out'

for filename in ../data/*.txt; do
  base=$(basename -- "$filename")
  echo "$base"

  # sekvencni
  #  $PROGRAM "$filename" >"out/sequence/$base" &

  # datovy paralelismus
  $PROGRAM "$filename" >"out/parallel/task/$base"

done
