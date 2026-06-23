# Full pass: every registered algorithm across all graph families and weight
# ranges (C). 5 sources x 3 repetitions each; median reported per config.
sources=5
reps=3

for C in 10 1000 1000000 1000000000; do
  for dim in 300 600; do
    bench --family grid --param rows="$dim" --param cols="$dim" \
          --max-weight "$C" --seed 1 --sources "$sources" --repetitions "$reps" \
          --algorithms all
  done

  bench --family sparse --param vertices=250000 --param edges=750000 \
        --max-weight "$C" --seed 1 --sources "$sources" --repetitions "$reps" \
        --algorithms all

  bench --family dense --param vertices=2000 --param density=30 \
        --max-weight "$C" --seed 1 --sources "$sources" --repetitions "$reps" \
        --algorithms all

  bench --family layered --param layers=400 --param width=300 --param degree=4 \
        --max-weight "$C" --seed 1 --sources "$sources" --repetitions "$reps" \
        --algorithms all
done
