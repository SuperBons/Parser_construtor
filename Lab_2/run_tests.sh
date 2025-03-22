# Directory containing test files
TEST_DIR="./test/all_tests"

# Directory containing the parser executable
PARSER_EXEC="./bin/parser"

# Reference results file
REFERENCE_RESULTS="./refercence.txt"

# Run parser on each test file and compare output with reference results
while IFS= read -r line; do
  if [[ $line == *":"* ]]; then
    TEST_FILE=$(echo $line | cut -d':' -f1)
    EXPECTED_RESULT=$(echo $line | cut -d':' -f2-)
    
    echo "Running test: $TEST_FILE"
    OUTPUT=$($PARSER_EXEC < "$TEST_DIR/$TEST_FILE")
    
    if [[ $OUTPUT == *"$EXPECTED_RESULT"* ]]; then
      echo "Test passed: $TEST_FILE"
    else
      echo "Test failed: $TEST_FILE"
      echo "Expected: $EXPECTED_RESULT"
      echo "Got: $OUTPUT"
    fi
  fi
done < "$REFERENCE_RESULTS"