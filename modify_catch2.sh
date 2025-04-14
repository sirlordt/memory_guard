#!/bin/bash

# Script to modify Catch2's signal handling to disable all signal handling
# This allows MemoryGuard to handle signals instead

# Find the catch_fatal_condition_handler.cpp file
CATCH_FILE=$(find _deps/catch2-src -name "catch_fatal_condition_handler.cpp")

if [ -z "$CATCH_FILE" ]; then
    echo "Error: catch_fatal_condition_handler.cpp not found!"
    exit 1
fi

echo "Found Catch2 file: $CATCH_FILE"

# Comment out the entire signal handling implementation in POSIX section
# Find the start of the POSIX signals section
START_LINE=$(grep -n "#if defined( CATCH_CONFIG_POSIX_SIGNALS )" "$CATCH_FILE" | cut -d':' -f1)
# Find the end of the POSIX signals section
END_LINE=$(grep -n "#endif // CATCH_CONFIG_POSIX_SIGNALS" "$CATCH_FILE" | cut -d':' -f1)

if [ -z "$START_LINE" ] || [ -z "$END_LINE" ]; then
    echo "Error: Could not find POSIX signals section!"
    exit 1
fi

# Create a temporary file
TMP_FILE=$(mktemp)

# Copy the file content up to the start line
head -n $((START_LINE-1)) "$CATCH_FILE" > "$TMP_FILE"

# Add the #if defined line
echo "#if defined( CATCH_CONFIG_POSIX_SIGNALS )" >> "$TMP_FILE"
echo "" >> "$TMP_FILE"
echo "// Signal handling code commented out to avoid conflicts with MemoryGuard" >> "$TMP_FILE"
echo "// This allows MemoryGuard to handle signals instead of Catch2" >> "$TMP_FILE"
echo "" >> "$TMP_FILE"
echo "namespace Catch {" >> "$TMP_FILE"
echo "    // Provide empty implementations for signal handling functions" >> "$TMP_FILE"
echo "    void FatalConditionHandler::engage_platform() {}" >> "$TMP_FILE"
echo "    void FatalConditionHandler::disengage_platform() noexcept {}" >> "$TMP_FILE"
echo "    FatalConditionHandler::FatalConditionHandler() = default;" >> "$TMP_FILE"
echo "    FatalConditionHandler::~FatalConditionHandler() = default;" >> "$TMP_FILE"
echo "}" >> "$TMP_FILE"
echo "" >> "$TMP_FILE"

# Add the #endif line
echo "#endif // CATCH_CONFIG_POSIX_SIGNALS" >> "$TMP_FILE"

# Copy the rest of the file after the end line
tail -n +$((END_LINE+1)) "$CATCH_FILE" >> "$TMP_FILE"

# Replace the original file with the modified one
mv "$TMP_FILE" "$CATCH_FILE"

echo "Successfully commented out all signal handling code in Catch2"
