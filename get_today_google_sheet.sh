#!/bin/bash
URL="https://docs.google.com/spreadsheets/d/19s78tQZO6-g5ph2sOiKDf1whIAt3fITZpoo5QeRf62A/export?format=csv&id=19s78tQZO6-g5ph2sOiKDf1whIAt3fITZpoo5QeRf62A&gid=458839323"  # Main
# URL="https://docs.google.com/spreadsheets/d/16XHLaGoY7cMcLTrdPsWv6YkhvQcU6_HNJklRzQOc16s/export?format=csv&id=16XHLaGoY7cMcLTrdPsWv6YkhvQcU6_HNJklRzQOc16s&gid=458839323"    # For testing
OUTDIR=$(dirname $0)/data
OUTPUT="$OUTDIR/$(date +%Y-%m-%d).csv"

mkdir -p "$OUTDIR"
curl -s -L -o "$OUTPUT" "$URL" > /dev/null
cp -f "$OUTPUT" data.csv