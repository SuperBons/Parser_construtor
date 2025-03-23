#!/bin/bash

# Set paths
TEST_DIR="./test/all_tests"
PARSER_BIN="./C++/bin/parser"
RESULTS_FILE="./RESULTS.txt"

# Check if files exist
if [ ! -d "$TEST_DIR" ]; then
    echo "Error: Test directory not found: $TEST_DIR"
    exit 1
fi

if [ ! -f "$PARSER_BIN" ]; then
    echo "Error: Parser binary not found: $PARSER_BIN"
    exit 1
fi

if [ ! -f "$RESULTS_FILE" ]; then
    echo "Error: Results file not found: $RESULTS_FILE"
    exit 1
fi

# Function to get expected output for a file
get_expected_output() {
    local file=$1
    local filename=$(basename "$file")
    grep -E "^$filename:" "$RESULTS_FILE" | sed "s/^$filename: //"
}

# Initialize counters
total=0
passed=0

# Test each .c file
for file in "$TEST_DIR"/*.c; do
    filename=$(basename "$file")
    expected=$(get_expected_output "$filename")
    
    echo "Testing: $filename"
    echo "Expected: $expected"
    
    # Run the parser
    output=$("$PARSER_BIN" "$file" 2>&1)
    
    echo "Got: $output"
    
    # Compare output
    if [[ "$output" == *"$expected"* ]]; then
        echo "✅ PASSED"
        ((passed++))
    else
        echo "❌ FAILED"
    fi
    
    echo "----------------------------------------"
    ((total++))
done

# Print summary
echo "Summary: $passed/$total tests passed"