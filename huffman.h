
#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdint.h>

// Структура узла дерева Хаффмана
typedef struct Node {
    unsigned char symbol;     // Символ (0-255), есть только у листьев
    uint32_t freq;            // Частота символа
    struct Node* left;        // Левый потомок (бит 0)
    struct Node* right;       // Правый потомок (бит 1)
} Node;

// --- Основные функции кодирования/декодирования ---
void encode_file(const char* input_filename, const char* output_filename);
void decode_file(const char* encoded_filename, const char* output_filename);

// --- Вспомогательные функции (могут быть полезны для тестирования) ---
uint32_t* count_frequencies(const char* filename);
char** build_huffman_dictionary(const uint32_t* freq);
void print_dictionary(const char** codes, const uint32_t* freq);
int files_equal(const char* f1, const char* f2);

#endif // HUFFMAN_H
