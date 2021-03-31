#!/usr/bin/bash

PROGRAM='./run.out'
OUT="out/parallel/data_epoch3_guided"

mkdir "$OUT"

for filename in ../data/*.txt; do
  base=$(basename -- "$filename")
  echo "$base"

  # sekvencni
  #  $PROGRAM "$filename" >"$OUT/$base" &

  # paralelni
  $PROGRAM "$filename" > "$OUT/$base"

done
