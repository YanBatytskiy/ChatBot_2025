#!/bin/zsh

SOURCE_DIR="./src"
OUTPUT_FILE="./for_chat.txt"

> "$OUTPUT_FILE"

find "$SOURCE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -print0 |
while IFS= read -r -d '' file; do
  echo "// ===== $file =====" >> "$OUTPUT_FILE"
  cat "$file" >> "$OUTPUT_FILE"
  printf "\n\n" >> "$OUTPUT_FILE"
done

echo "✅ Готово: $OUTPUT_FILE"