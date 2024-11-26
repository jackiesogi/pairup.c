#!/bin/bash

INPUT='./relations.dot'
OUTPUT='./relations.png'

# 檢查輸入檔案是否存在
if [[ ! -f "$INPUT" ]]; then
    echo "Error: File '$INPUT' does not exist."
    exit 1
fi

# 嘗試生成 PNG
dot -Tpng "$INPUT" -o "$OUTPUT"
if [[ $? -ne 0 ]]; then
    echo "Error: Failed to generate PNG file. Check the DOT file for syntax errors."
    exit 1
fi

# 提示成功
echo "PNG file generated successfully: $OUTPUT"

xdg-open "$OUTPUT"  # Linux

