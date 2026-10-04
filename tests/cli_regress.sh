#!/bin/sh
# ==============================================================================
# MediSave Edge - CLI Regression Test Suite
# Tests EOF handling, graceful shutdown, input validation, and menu robustness
# ==============================================================================

set -e

BIN="./bin/medisave"
if [ ! -f "$BIN" ] && [ -f "./bin/medisave.exe" ]; then
    BIN="./bin/medisave.exe"
fi

if [ ! -f "$BIN" ]; then
    echo "[FAIL] Binary $BIN not found. Compile first with 'make'."
    exit 1
fi

echo "=========================================="
echo " Running MediSave Edge CLI Regression Tests"
echo "=========================================="

PASSED=0
TOTAL=0

run_test() {
    TEST_NAME="$1"
    INPUT="$2"
    EXPECTED_EXIT="$3"
    TOTAL=$((TOTAL + 1))

    printf "[TEST] %s ... " "$TEST_NAME"

    OUTPUT=$(printf "%s" "$INPUT" | "$BIN" 2>&1)
    STATUS=$?

    if [ "$STATUS" -ne "$EXPECTED_EXIT" ]; then
        echo "FAILED (exit code $STATUS, expected $EXPECTED_EXIT)"
        echo "$OUTPUT"
        exit 1
    fi

    echo "PASSED"
    PASSED=$((PASSED + 1))
}

# Test 1: Immediate EOF /dev/null should trigger graceful shutdown with exit code 0
run_test "Immediate EOF (/dev/null)" "" 0

# Test 2: Standard exit option (18)
run_test "Clean Menu Exit (Option 18)" "18
" 0

# Test 3: Invalid menu choice followed by exit
run_test "Invalid menu choice followed by exit" "99
18
" 0

# Test 4: Name with pipe delimiter '|' is rejected
OUTPUT=$(printf "1\nMED-CLI-01\nBad|Name\n18\n" | "$BIN" 2>&1)
TOTAL=$((TOTAL + 1))
if echo "$OUTPUT" | grep -iq "cannot.*contain" || echo "$OUTPUT" | grep -iq "invalid"; then
    echo "[TEST] Pipe delimiter rejection in Name ... PASSED"
    PASSED=$((PASSED + 1))
else
    echo "[TEST] Pipe delimiter rejection in Name ... FAILED"
    echo "$OUTPUT"
    exit 1
fi

# Test 5: Batch with pipe delimiter '|' is rejected
OUTPUT=$(printf "1\nMED-CLI-02\nGoodName\nBad|Batch\n18\n" | "$BIN" 2>&1)
TOTAL=$((TOTAL + 1))
if echo "$OUTPUT" | grep -iq "cannot.*contain" || echo "$OUTPUT" | grep -iq "invalid"; then
    echo "[TEST] Pipe delimiter rejection in Batch ... PASSED"
    PASSED=$((PASSED + 1))
else
    echo "[TEST] Pipe delimiter rejection in Batch ... FAILED"
    echo "$OUTPUT"
    exit 1
fi

echo "=========================================="
echo " CLI Regression Tests Passed: $PASSED / $TOTAL"
echo "=========================================="
exit 0
