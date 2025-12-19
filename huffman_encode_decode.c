#include "huffman.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Кодирование файла ---
void encode_file(const char* input_filename, const char* output_filename) {
    // 1. Подсчитываем частоты символов
    uint32_t* freq = count_frequencies(input_filename);
    if (!freq) {
        printf("Error: cannot read input file %s\n", input_filename);
        return;
    }

    // 2. Открываем файлы
    FILE* in = fopen(input_filename, "rb");
    FILE* out = fopen(output_filename, "wb");
    if (!in || !out) {
        printf("Error: cannot open files for encoding\n");
        free(freq);
        return;
    }

    // 3. Записываем заголовок с частотами
    uint32_t symbol_count = 0;
    for (int i = 0; i < 256; i++) {
        if (freq[i]) symbol_count++;
    }
    fwrite(&symbol_count, sizeof(uint32_t), 1, out);

    for (int i = 0; i < 256; i++) {
        if (freq[i]) {
            fputc((unsigned char)i, out);
            fwrite(&freq[i], sizeof(uint32_t), 1, out);
        }
    }

    // 4. Строим коды Хаффмана
    char** codes = build_huffman_dictionary(freq);
    if (!codes) {
        printf("Error: failed to build Huffman codes\n");
        fclose(in);
        fclose(out);
        free(freq);
        return;
    }

    // 5. Кодируем данные файла
    unsigned char byte = 0;
    int bit_count = 0;
    long total_bits = 0;
    int c;

    while ((c = fgetc(in)) != EOF) {
        char* code = codes[c];
        if (!code) {
            printf("Error: no code for symbol %d\n", c);
            break;
        }

        for (int i = 0; code[i] != '\0'; i++) {
            if (code[i] == '1') {
                byte |= (1 << (7 - bit_count));
            }
            bit_count++;
            total_bits++;

            if (bit_count == 8) {
                fputc(byte, out);
                byte = 0;
                bit_count = 0;
            }
        }
    }

    // Записываем последний неполный байт
    if (bit_count > 0) {
        fputc(byte, out);
    }

    // 6. Закрываем файлы
    fclose(in);
    fclose(out);

    // 7. Вычисляем статистику
    FILE* tmp = fopen(input_filename, "rb");
    long input_size = 0;
    if (tmp) {
        fseek(tmp, 0, SEEK_END);
        input_size = ftell(tmp);
        fclose(tmp);
    }

    tmp = fopen(output_filename, "rb");
    long output_size = 0;
    if (tmp) {
        fseek(tmp, 0, SEEK_END);
        output_size = ftell(tmp);
        fclose(tmp);
    }

    // 8. Выводим результаты
    printf("\n=== Encoding Results ===\n");
    printf("Input file:  %s (%ld bytes)\n", input_filename, input_size);
    printf("Output file: %s (%ld bytes)\n", output_filename, output_size);
    printf("Total bits:  %ld\n", total_bits);

    if (input_size > 0) {
        double ratio = (double)output_size / input_size;
        printf("Compression: %.2f%%\n", (1.0 - ratio) * 100.0);
    }

    // 9. Освобождаем память
    for (int i = 0; i < 256; i++) {
        if (codes[i]) {
            free(codes[i]);
        }
    }
    free(codes);
    free(freq);

    printf("Encoding completed successfully!\n");
}

// --- Декодирование файла ---
void decode_file(const char* encoded_filename, const char* output_filename) {
    // 1. Читаем частоты из заголовка
    uint32_t freq[256];
    read_frequencies_from_huff(encoded_filename, freq);

    // 2. Проверяем количество уникальных символов
    int unique = 0;
    uint64_t total_symbols = 0;

    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            unique++;
            total_symbols += freq[i];
        }
    }

    printf("\n=== Decoding Information ===\n");
    printf("Unique symbols: %d\n", unique);
    printf("Total symbols to decode: %lu\n", (unsigned long)total_symbols);

    // 3. Случай: пустой файл
    if (unique == 0 || total_symbols == 0) {
        FILE* out = fopen(output_filename, "wb");
        if (out) fclose(out);
        printf("Decoding completed (empty file)\n");
        return;
    }

    // 4. Особый случай: только один символ
    if (unique == 1) {
        int symbol = -1;
        for (int i = 0; i < 256; i++) {
            if (freq[i] > 0) {
                symbol = i;
                break;
            }
        }

        FILE* out = fopen(output_filename, "wb");
        if (!out) {
            printf("Error: cannot create output file\n");
            return;
        }

        for (uint64_t i = 0; i < total_symbols; i++) {
            fputc(symbol, out);
        }

        fclose(out);
        printf("Decoding completed (single symbol file)\n");
        return;
    }

    // 5. Общий случай: строим дерево Хаффмана
    Node** nodes = (Node**)malloc(unique * sizeof(Node*));
    if (!nodes) {
        printf("Error: memory allocation failed\n");
        return;
    }

    int idx = 0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            nodes[idx++] = create_node((unsigned char)i, freq[i]);
        }
    }

    int node_count = unique;
    Node* root = build_huffman_tree(nodes, &node_count);

    // 6. Открываем файлы
    FILE* in = fopen(encoded_filename, "rb");
    FILE* out = fopen(output_filename, "wb");

    if (!in || !out) {
        printf("Error: cannot open files for decoding\n");
        free(nodes);
        return;
    }

    // 7. Пропускаем заголовок
    uint32_t symbol_count;
    fread(&symbol_count, sizeof(uint32_t), 1, in);
    for (uint32_t i = 0; i < symbol_count; i++) {
        fgetc(in); // символ
        uint32_t dummy;
        fread(&dummy, sizeof(uint32_t), 1, in); // частота
    }

    // 8. Декодируем данные
    Node* current = root;
    uint64_t decoded = 0;
    int byte;
    int bytes_read = 0;

    printf("Decoding progress: ");

    while (decoded < total_symbols && (byte = fgetc(in)) != EOF) {
        bytes_read++;

        // Показываем прогресс каждые 10KB
        if (bytes_read % 10240 == 0) {
            printf(".");
            fflush(stdout);
        }

        for (int i = 0; i < 8 && decoded < total_symbols; i++) {
            int bit = (byte >> (7 - i)) & 1;
            current = bit ? current->right : current->left;

            if (!current->left && !current->right) {
                fputc(current->symbol, out);
                decoded++;
                current = root;
            }
        }
    }

    printf("\n");

    // 9. Проверяем корректность декодирования
    if (decoded != total_symbols) {
        printf("Warning: expected %lu symbols, decoded %lu\n",
               (unsigned long)total_symbols, (unsigned long)decoded);
    }

    // 10. Закрываем файлы и освобождаем память
    fclose(in);
    fclose(out);
    free(nodes);

    printf("Decoding completed successfully!\n");
    printf("Decoded symbols: %lu\n", (unsigned long)decoded);
}