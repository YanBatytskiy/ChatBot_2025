#!/bin/bash

# Папка, откуда будем собирать файлы
SOURCE_DIR="$HOME/SynologyDrive/CPP/Linux/GitProjects/Shark2_0/src"
OUTPUT_FILE="for_chat.txt"

# Обнуляем/создаем файл
> "$OUTPUT_FILE"

# Находим все .h и .cpp файлы и объединяем
find "$SOURCE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) | sort | while read file; do
  echo "// ===== $file =====" >> "$OUTPUT_FILE"
  cat "$file" >> "$OUTPUT_FILE"
  echo -e "\n\n" >> "$OUTPUT_FILE"
done

echo "✅ Готово: создан файл $OUTPUT_FILE со всеми исходниками."

cat "$OUTPUT_FILE" | xclip -selection clipboard
echo "📋 Код скопирован в буфер обмена!"
