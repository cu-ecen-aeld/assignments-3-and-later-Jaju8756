#!/bin/sh
# Reference: https://tldp.org/LDP/abs/html/

# Assign arguments
writefile="$1"
writestr="$2"

if [ -z "$writefile" ] || [ -z "$writestr" ]; then
    echo "Error: Both writefile and writestr must be specified"
    exit 1
fi

# Extract directory path
dirpath=$(dirname "$writefile")

# Create directory if it doesn't exist
mkdir -p "$dirpath" 2>/dev/null

# Write string to file, overwrite if exists
echo "$writestr" > "$writefile" 2>/dev/null

# Check if writing succeeded
if [ $? -ne 0 ]; then
    echo "Error: Could not create or write to file $writefile"
    exit 1
fi

