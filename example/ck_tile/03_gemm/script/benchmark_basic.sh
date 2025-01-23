#!/bin/sh
EXE="$(find . -name tile_example_gemm_basic -type f | head -n 1)"
VALID=0

for prec in "fp16" "bf16"; do
    for b_matrix_layout in "R" "C"; do
        for m in "64" "512" "1024" "2048"; do
            for n in "512" "1024" "2048"; do
                for k in "64" "512" "1024" "2048"; do
                    $EXE -prec=$prec -m=$m -n=$n -k=$k -a_layout="R" -b_layout="$b_matrix_layout" -c_layout="R" -v=$VALID
                done
            done
        done
    done
done
