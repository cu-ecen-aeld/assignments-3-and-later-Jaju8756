#!/bin/sh
#Reference: https://tldp.org/LDP/abs/html/

# Assign arguments
filesdir="$1"
searchstr="$2"

if [ -z "$filesdir" ] || [ -z "$searchstr" ]; then
    echo "Error: Both filesdir and searchstr must be specified"
    exit 1
fi

if [ ! -d "$filesdir" ]; then
    echo "Error: filesdir does not represent a directory"
    exit 1
fi

# Count number of files
num_files=$(find "$filesdir" -type f | wc -l)

# Count number of matching lines
num_matching_lines=$(grep -r "$searchstr" "$filesdir" | wc -l)

echo "The number of files are $num_files and the number of matching lines are $num_matching_lines"

