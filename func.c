#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct Node {
    unsigned char symbol;
    uint32_t freq;
    struct Node* left;
    struct Node* right;
} Node;

// --- Подсчёт частот ---
void count_frequencies(const char* filename, uint32_t freq[256]) {
    for (int i = 0; i < 256; i++) freq[i] = 0;

    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: cannot open file %s\n", filename);
        exit(1);
    }

    int c;
    while ((c = fgetc(file)) != EOF) {
        freq[c]++;
    }
    fclose(file);
}

// --- Создание узла ---
Node* create_node(unsigned char symbol, uint32_t freq) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->symbol = symbol;
    node->freq = freq;
    node->left = node->right = NULL;
    return node;
}

// --- Сортировка узлов по частоте (пузырьком) ---
void sort_nodes(Node** nodes, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (nodes[j]->freq > nodes[j + 1]->freq) {
                Node* tmp = nodes[j];
                nodes[j] = nodes[j + 1];
                nodes[j + 1] = tmp;
            }
        }
    }
}

// --- Построение дерева Хаффмана ---
Node* build_huffman_tree(Node** nodes, int* node_count) {
    int n = *node_count;
    if (n == 0) return NULL;
    if (n == 1) {
        // Особый случай: один символ — делаем фиктивный корень
        Node* root = create_node(0, nodes[0]->freq);
        root->left = nodes[0];
        *node_count = 1;
        return root;
    }

    while (n > 1) {
        sort_nodes(nodes, n);
        Node* left = nodes[0];
        Node* right = nodes[1];
        Node* parent = create_node(0, left->freq + right->freq);
        parent->left = left;
        parent->right = right;

        // Объединяем: удаляем два первых, добавляем parent на позицию 0
        nodes[0] = parent;
        for (int i = 1; i < n - 1; i++) {
            nodes[i] = nodes[i + 1];
        }
        n--;
    }
    *node_count = n;
    return nodes[0];
}

// --- Генерация кодов рекурсивно ---
void generate_codes(Node* node, char* buffer, int depth, char** codes) {
    if (!node) return;
    if (!node->left && !node->right) {
        buffer[depth] = '\0';
        codes[node->symbol] = (char*)malloc((depth + 1) * sizeof(char));
        strcpy(codes[node->symbol], buffer);
        return;
    }
    buffer[depth] = '0';
    generate_codes(node->left, buffer, depth + 1, codes);
    buffer[depth] = '1';
    generate_codes(node->right, buffer, depth + 1, codes);
}

// --- Построение словаря кодов ---
char** build_huffman_dictionary(uint32_t freq[256]) {
    char** codes = (char**)calloc(256, sizeof(char*));
    int unique = 0;
    for (int i = 0; i < 256; i++) if (freq[i]) unique++;

    if (unique == 0) return codes;

    Node** nodes = (Node**)malloc(unique * sizeof(Node*));
    int idx = 0;
    for (int i = 0; i < 256; i++) {
        if (freq[i]) {
            nodes[idx++] = create_node((unsigned char)i, freq[i]);
        }
    }

    int node_count = unique;
    Node* root = build_huffman_tree(nodes, &node_count);

    char buffer[257];
    generate_codes(root, buffer, 0  , codes);

    free(nodes);
    // Дерево не освобождаем для простоты
    return codes;

}

// --- Печать словаря ---
void print_dictionary(char** codes, uint32_t freq[256]) {//ненужная функция
    printf("\n=== Translation Dictionary ===\n");
    for (int i = 0; i < 256; i++) {
        if (codes[i] && freq[i] > 0) {
            if (i >= 32 && i <= 126) {
                printf("'%c' (code %d): %s (freq: %u)\n", i, i, codes[i], freq[i]);
            } else {
                printf("code %d: %s (freq: %u)\n", i, codes[i], freq[i]);
            }
        }
    }
}

// --- Кодирование и запись в .huff файл ---
void encode_file(const char* input_filename, const char* output_filename, uint32_t freq[256]) {
    FILE* in = fopen(input_filename, "rb");
    FILE* out = fopen(output_filename, "wb");
    if (!in || !out) {
        printf("Error: cannot open files for encoding\n");
        exit(1);
    }

    // 1. Запись частот в заголовок
    uint32_t symbol_count = 0;
    for (int i = 0; i < 256; i++) if (freq[i]) symbol_count++;
    fwrite(&symbol_count, sizeof(uint32_t), 1, out);

    for (int i = 0; i < 256; i++) {
        if (freq[i]) {
            fputc((unsigned char)i, out);
            fwrite(&freq[i], sizeof(uint32_t), 1, out);
        }
    }

    // 2. Построение кодов
    char** codes = build_huffman_dictionary(freq);
    if (!codes) {
        printf("Error: failed to build Huffman codes\n");
        exit(1);
    }

    // 3. Кодирование
    unsigned char byte = 0;
    int bit_count = 0;
    long total_bits = 0;
    int c;
    while ((c = fgetc(in)) != EOF) {
        char* code = codes[c];
        for (int i = 0; code[i]; i++) {
            if (code[i] == '1')
                byte |= (1 << (7 - bit_count));
            bit_count++;
            total_bits++;
            if (bit_count == 8) {
                fputc(byte, out);
                byte = 0;
                bit_count = 0;
            }
        }
    }
    if (bit_count > 0) {
        fputc(byte, out);
    }

    fclose(in);
    fclose(out);

    // Очистка
    for (int i = 0; i < 256; i++) free(codes[i]);
    free(codes);

    long input_size = 0;
    FILE* tmp = fopen(input_filename, "rb");
    if (tmp) {
        fseek(tmp, 0, SEEK_END);
        input_size = ftell(tmp);
        fclose(tmp);
    }
    long output_size = ftell(out); // но out закрыт — лучше переоткрыть
    FILE* tmp2 = fopen(output_filename, "rb");
    if (tmp2) {
        fseek(tmp2, 0, SEEK_END);
        output_size = ftell(tmp2);
        fclose(tmp2);
    }

    double ratio = (input_size > 0) ? (double)output_size / input_size : 0.0;
    printf("\nEncoding completed!\n");
    printf("Input file: %s (%ld bytes)\n", input_filename, input_size);
    printf("Output file: %s (%ld bytes)\n", output_filename, output_size);
    printf("Total bits encoded: %ld\n", total_bits);
    printf("Compression ratio: %.2f%%\n", ratio * 100);
}

// --- Чтение частот из .huff файла ---
void read_frequencies_from_huff(const char* filename, uint32_t freq[256]) {
    for (int i = 0; i < 256; i++) freq[i] = 0;

    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: cannot open %s\n", filename);
        exit(1);
    }

    uint32_t symbol_count;
    fread(&symbol_count, sizeof(uint32_t), 1, file);

    for (uint32_t i = 0; i < symbol_count; i++) {
        int c = fgetc(file);
        uint32_t f;
        fread(&f, sizeof(uint32_t), 1, file);
        freq[c] = f;
    }

    fclose(file);
}

// --- Декодирование с использованием дерева ---
void decode_with_tree(Node* root, FILE* in, FILE* out, long file_size) {
    Node* current = root;
    int byte;
    long bytes_read = 0;
    while ((byte = fgetc(in)) != EOF && bytes_read < file_size) {
        for (int i = 0; i < 8; i++) {
            int bit = (byte >> (7 - i)) & 1;
            current = bit ? current->right : current->left;
            if (!current->left && !current->right) {
                fputc(current->symbol, out);
                current = root;
            }
        }
        bytes_read++;
    }
}

// --- Декодирование файла ---
void decode_file(const char* encoded_filename, const char* output_filename) {
    uint32_t freq[256];
    read_frequencies_from_huff(encoded_filename, freq);

    // Построить дерево
    int unique = 0;
    for (int i = 0; i < 256; i++) if (freq[i]) unique++;
    if (unique == 0) {
        printf("Error: no frequency data\n");
        return;
    }

    Node** nodes = (Node**)malloc(unique * sizeof(Node*));
    int idx = 0;
    for (int i = 0; i < 256; i++) {
        if (freq[i]) {
            nodes[idx++] = create_node((unsigned char)i, freq[i]);
        }
    }

    int node_count = unique;
    Node* root = build_huffman_tree(nodes, &node_count);

    // Открыть файлы
    FILE* in = fopen(encoded_filename, "rb");
    FILE* out = fopen(output_filename, "wb");
    if (!in || !out) {
        printf("Error opening files for decoding\n");
        exit(1);
    }

    // Пропустить заголовок
    uint32_t symbol_count;
    fread(&symbol_count, sizeof(uint32_t), 1, in);
    fseek(in, symbol_count * (1 + sizeof(uint32_t)), SEEK_CUR);

    // Получаем размер данных после заголовка
    fseek(in, 0, SEEK_END);
    long total_size = ftell(in);
    fseek(in, sizeof(uint32_t) + symbol_count * (1 + sizeof(uint32_t)), SEEK_SET);
    long data_size = total_size - (sizeof(uint32_t) + symbol_count * (1 + sizeof(uint32_t)));

    decode_with_tree(root, in, out, data_size);

    fclose(in);
    fclose(out);
    free(nodes);

    printf("\nDecoding completed!\n");
    printf("Encoded file: %s\n", encoded_filename);
    printf("Output file: %s\n", output_filename);
}

// --- Сравнение двух файлов ---
int files_equal(const char* f1, const char* f2) {
    FILE* a = fopen(f1, "rb");
    FILE* b = fopen(f2, "rb");
    if (!a || !b) return 0;

    int c1, c2;
    while (1) {
        c1 = fgetc(a);
        c2 = fgetc(b);
        if (c1 != c2) {
            fclose(a); fclose(b);
            return 0;
        }
        if (c1 == EOF) break;
    }
    fclose(a); fclose(b);
    return 1;
}

// --- MAIN ---
int main() {
    char mode[10];
    char input_filename[256];
    char encoded_filename[256];
    char decoded_filename[256];

    printf("=== HUFFMAN ENCODING PROGRAM ===\n");
    printf("Available modes: encode, decode\n");
    printf("Enter mode: ");
    scanf("%9s", mode);

    if (strcmp(mode, "encode") == 0) {
        printf("Enter source filename to encode: ");
        scanf("%255s", input_filename);

        strcpy(encoded_filename, input_filename);
        strcat(encoded_filename, ".huff");

        printf("\n--- Encoding ---\n");
        printf("Input: %s\n", input_filename);
        printf("Output: %s\n", encoded_filename);

        uint32_t freq[256];
        count_frequencies(input_filename, freq);

        // Доп. вывод: можно раскомментировать, если нужно
        // char** codes = build_huffman_dictionary(freq);
        // print_dictionary(codes, freq);
        // for (int i = 0; i < 256; i++) free(codes[i]); free(codes);

        encode_file(input_filename, encoded_filename, freq);
        printf("\n Encoding finished successfully!\n");

    } else if (strcmp(mode, "decode") == 0) {
        printf("Enter encoded filename (.huff): ");
        scanf("%255s", encoded_filename);

        // Формируем имя декодированного файла
        if (strstr(encoded_filename, ".huff") != NULL) {
            strcpy(decoded_filename, encoded_filename);
            char* dot = strstr(decoded_filename, ".huff");
            *dot = '\0';
            strcat(decoded_filename, "_decoded.txt");
        } else {
            strcpy(decoded_filename, encoded_filename);
            strcat(decoded_filename, "_decoded.txt");
        }

        printf("\n--- Decoding ---\n");
        printf("Encoded file: %s\n", encoded_filename);
        printf("Decoded file: %s\n", decoded_filename);

        decode_file(encoded_filename, decoded_filename);
        printf("\n Decoding finished successfully!\n");

        // Попытка найти исходный файл и сравнить
        char original[256];
        strcpy(original, encoded_filename);
        char* dot = strstr(original, ".huff");
        if (dot) *dot = '\0';

        if (files_equal(original, decoded_filename)) {
            printf("\n SUCCESS: Decoded file matches original '%s'!\n", original);
        } else {
            printf("\n  WARNING: Files do NOT match. Check for errors.\n");
        }

    } else {
        printf("Error: unknown mode '%s'\n", mode);
        printf("Use 'encode' or 'decode'\n");
        return 1;
    }
    return 0;
}