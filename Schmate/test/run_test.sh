### **tests/run_test.sh**
#!/bin/bash
set -e

BIN=../build/sbert_search
MODEL=../models/sbert.ggml

if [ ! -f "$BIN" ]; then
  echo "Build first: (cd .. && mkdir build && cd build && cmake .. && make -j)"
  exit 1
fi

echo "Running test..."

$BIN $MODEL <<EOF
append AI is a hype market
append he is a researcher in AI
append he likes AI
append he develops in C++
knn AI
radius 0.6 AI
relative AI
adaptive AI
reconstruct_sid 1
quit
EOF

