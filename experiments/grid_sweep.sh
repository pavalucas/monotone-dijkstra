# Grid graph pass: every registered algorithm across grid sizes and weight
# ranges (C). 10 sources x 3 repetitions each; median reported per config.
for dim in 100 300 600; do
  for C in 10 1000 1000000 1000000000; do
    bench --family grid --param rows="$dim" --param cols="$dim" \
          --max-weight "$C" --seed 1 --sources 10 --repetitions 3 \
          --algorithms all
  done
done
