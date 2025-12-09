#!/bin/bash
# perf_pid_test.sh - Comprehensive PID Performance Test Suite

# perf 경로 설정
PERF="./perf"

KERNEL_VERSION=$(uname -r)
RESULT_DIR="perf_results_${KERNEL_VERSION}_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULT_DIR"

echo "=========================================="
echo "PID Performance Test Suite"
echo "Kernel: $KERNEL_VERSION"
echo "Date: $(date)"
echo "=========================================="

# 권한 설정
sudo sysctl -w kernel.perf_event_paranoid=-1 > /dev/null

# 사용 가능한 이벤트 확인 및 저장
echo "Checking available events..."
$PERF list > "$RESULT_DIR/available_events.txt"

# 1. Process Creation Test
echo ""
echo "[Test 1/5] Process Creation (fork)"
sudo $PERF stat -o "$RESULT_DIR/stat_fork.txt" -e task-clock,context-switches,page-faults,cycles,instructions -r 10 stress-ng --fork 8 --timeout 30s --metrics-brief 2>&1 | tee "$RESULT_DIR/fork_output.txt"

# 2. Clone Test
echo ""
echo "[Test 2/5] Clone (with namespace)"
sudo $PERF stat -o "$RESULT_DIR/stat_clone.txt" -e cycles,instructions,context-switches -r 10 stress-ng --clone 4 --timeout 30s --metrics-brief 2>&1 | tee "$RESULT_DIR/clone_output.txt"

# 3. Context Switching
echo ""
echo "[Test 3/5] Context Switching"
sudo $PERF stat -o "$RESULT_DIR/stat_switch.txt" -e context-switches,cpu-migrations -r 10 stress-ng --switch 8 --timeout 30s --metrics-brief 2>&1 | tee "$RESULT_DIR/switch_output.txt"

# 4. Detailed Profile
echo ""
echo "[Test 4/5] Detailed CPU Profiling"
sudo $PERF record -o "$RESULT_DIR/perf.data" -e cycles -g -a stress-ng --fork 4 --timeout 20s

# perf.data가 정상적으로 생성되었는지 확인
if [ -s "$RESULT_DIR/perf.data" ]; then
    echo "Generating reports..."
    sudo $PERF report -i "$RESULT_DIR/perf.data" --stdio > "$RESULT_DIR/report_full.txt"
    sudo $PERF report -i "$RESULT_DIR/perf.data" --sort=symbol --stdio | head -100 > "$RESULT_DIR/report_symbols.txt"
    echo "Profile saved successfully"
else
    echo "Warning: perf.data is empty or not created"
fi

# 5. Wait Test
echo ""
echo "[Test 5/5] Wait Performance (process lifecycle)"
sudo $PERF stat -o "$RESULT_DIR/stat_wait.txt" -e cycles,instructions,context-switches -r 10 stress-ng --wait 4 --timeout 20s --metrics-brief 2>&1 | tee "$RESULT_DIR/wait_output.txt"

echo ""
echo "=========================================="
echo "Tests completed!"
echo "Results saved to: $RESULT_DIR"
echo "=========================================="

# 요약 생성
cat > "$RESULT_DIR/summary.txt" << EOF
PID Performance Test Summary
============================
Kernel: $KERNEL_VERSION
Date: $(date)

Test Results:
- Process Creation: $RESULT_DIR/stat_fork.txt
- Clone Test: $RESULT_DIR/stat_clone.txt
- Context Switch: $RESULT_DIR/stat_switch.txt
- Detailed Profile: $RESULT_DIR/report_full.txt
- Wait Test: $RESULT_DIR/wait_output.txt

Key Metrics to Compare:
1. cycles (lower is better)
2. instructions (observe efficiency)
3. context-switches (observe pattern)
4. task-clock (lower is better)
5. instructions per cycle (higher is better)

To compare with other kernels:
1. Boot into different kernel (original/skiplist/rb-skiplist)
2. Run this script again
3. Compare results in perf_results_* directories

Example comparison:
$ grep "seconds time elapsed" perf_results_*/stat_fork.txt
EOF

cat "$RESULT_DIR/summary.txt"

# 주요 결과 요약 출력
echo ""
echo "=========================================="
echo "Quick Summary"
echo "=========================================="
echo "Fork Test:"
grep -E "seconds time elapsed|context-switches" "$RESULT_DIR/stat_fork.txt" 2>/dev/null | head -3
echo ""
echo "Clone Test:"
grep -E "seconds time elapsed|context-switches" "$RESULT_DIR/stat_clone.txt" 2>/dev/null | head -3
echo ""
echo "Context Switch Test:"
grep -E "seconds time elapsed|context-switches" "$RESULT_DIR/stat_switch.txt" 2>/dev/null | head -3
echo "=========================================="
