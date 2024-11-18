#!/bin/bash
URL="https://docs.google.com/spreadsheets/d/19s78tQZO6-g5ph2sOiKDf1whIAt3fITZpoo5QeRf62A/export?format=csv&id=19s78tQZO6-g5ph2sOiKDf1whIAt3fITZpoo5QeRf62A&gid=458839323"
OUTDIR=$(dirname $0)/data
OUTPUT="$OUTDIR/$(date +%Y-%m-%d).csv"

mkdir -p "$OUTDIR"
curl -s -L -o "$OUTPUT" "$URL" > /dev/null
cp -f "$OUTPUT" data.csv