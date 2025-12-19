#include "huffman.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Локальные функции (используются только внутри этого файла) ---
static Node* create_node(unsigned char symbol, uint32_t freq);
static void sort_nodes(Node** nodes, int n);
static Node* build_huffman_tree(Node** nodes, int* node_count);
static void generate_codes(Node* node, char* buffer, int depth, char** codes);
static void read_frequencies_from_huff(const char* filename, uint32_t* freq);

// --- Создание узла ---
Node* create_node(unsigned char symbol, uint32_t freq) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->symbol = symbol;
    node->freq = freq;
    node->left = node->right = NULL;
    return node;
}

// --- Подсчёт частот ---
uint32_t* count_frequencies(const char* filename) {
    uint32_t* freq = (uint32_t*)calloc(256, sizeof(uint32_t));
    if (!freq) return NULL;

    FILE* file = fopen(filename, "rb");
    if (!file) {
        free(freq);
        return NULL;
    }

    int c;
    while ((c = fgetc(file)) != EOF) {
        freq[c]++;
    }

    fclose(file);
    return freq;
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
        if (codes[node->symbol]) {
            strcpy(codes[node->symbol], buffer);
        }
        return;
    }

    if (node->left) {
        buffer[depth] = '0';
        generate_codes(node->left, buffer, depth + 1, codes);
    }

    if (node->right) {
        buffer[depth] = '1';
        generate_codes(node->right, buffer, depth + 1, codes);
    }
}

// --- Построение словаря кодов ---
char** build_huffman_dictionary(const uint32_t* freq) {
    // Выделяем память для кодов
    char** codes = (char**)calloc(256, sizeof(char*));
    if (!codes) return NULL;

    // Считаем количество уникальных символов
    int unique = 0;
    for (int i = 0; i < 256; i++) {
        if (freq[i]) unique++;
    }

    if (unique == 0) {
        return codes;  // Пустой файл
    }

    // Особый случай: файл содержит только один тип символа
    if (unique == 1) {
        for (int i = 0; i < 256; i++) {
            if (freq[i] > 0) {
                // Единственному символу присваиваем код "0"
                codes[i] = (char*)malloc(2 * sizeof(char));
                if (codes[i]) {
                    strcpy(codes[i], "0");
                }
                break;
            }
        }
        return codes;
    }

    // Обычный случай: несколько символов
    Node** nodes = (Node**)malloc(unique * sizeof(Node*));
    if (!nodes) {
        free(codes);
        return NULL;
    }

    // Создаем узлы для каждого символа
    int idx = 0;
    for (int i = 0; i < 256; i++) {
        if (freq[i]) {
            nodes[idx++] = create_node((unsigned char)i, freq[i]);
        }
    }

    int node_count = unique;
    Node* root = build_huffman_tree(nodes, &node_count);

    // Генерируем коды
    char buffer[257];
    generate_codes(root, buffer, 0, codes);

    // Освобождаем память узлов (но не само дерево целиком)
    free(nodes);

    return codes;
}

// --- Печать словаря ---
void print_dictionary(const char** codes, const uint32_t* freq) {
    printf("\n=== Translation Dictionary ===\n");
    int printed = 0;

    for (int i = 0; i < 256; i++) {
        if (codes[i] && freq[i] > 0) {
            printed++;
            if (i >= 32 && i <= 126) {
                printf("'%c' (code %3d): %-20s (freq: %u)\n",
                       i, i, codes[i], freq[i]);
            } else {
                printf("code %3d: %-20s (freq: %u)\n",
                       i, codes[i], freq[i]);
            }
        }
    }

    if (printed == 0) {
        printf("(no symbols)\n");
    } else {
        printf("Total: %d unique symbols\n", printed);
    }
}

// --- Чтение частот из заголовка .huff файла ---
void read_frequencies_from_huff(const char* filename, uint32_t* freq) {
    for (int i = 0; i < 256; i++) freq[i] = 0;

    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: cannot open %s\n", filename);
        return;
    }

    uint32_t symbol_count;
    if (fread(&symbol_count, sizeof(uint32_t), 1, file) != 1) {
        fclose(file);
        return;
    }

    for (uint32_t i = 0; i < symbol_count; i++) {
        int c = fgetc(file);
        uint32_t f;
        if (fread(&f, sizeof(uint32_t), 1, file) != 1) {
            break;
        }
        freq[c] = f;
    }

    fclose(file);
}

// --- Сравнение двух файлов ---
int files_equal(const char* f1, const char* f2) {
    FILE* a = fopen(f1, "rb");
    FILE* b = fopen(f2, "rb");
    if (!a || !b) return 0;

    int c1, c2;
    int equal = 1;

    while (1) {
        c1 = fgetc(a);
        c2 = fgetc(b);

        if (c1 != c2) {
            equal = 0;
            break;
        }

        if (c1 == EOF) break;
    }

    fclose(a);
    fclose(b);
    return equal;
}