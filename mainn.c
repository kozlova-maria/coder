#include "huffman.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Функция для отображения меню ---
void print_menu() {
    printf("\n=====================================\n");
    printf("    HUFFMAN ENCODING PROGRAM\n");
    printf("=====================================\n");
    printf("1. Encode a file\n");
    printf("2. Decode a .huff file\n");
    printf("3. Test encoding/decoding\n");
    printf("4. Show dictionary for a file\n");
    printf("5. Compare two files\n");
    printf("6. Exit\n");
    printf("=====================================\n");
    printf("Enter your choice (1-6): ");
}

// --- Функция для тестирования ---
void test_encoding_decoding(const char* filename) {
    printf("\n=== Testing Huffman Encoding/Decoding ===\n");

    // Создаем имена файлов
    char encoded[256];
    char decoded[256];

    snprintf(encoded, sizeof(encoded), "%s.huff", filename);
    snprintf(decoded, sizeof(decoded), "%s_decoded.bin", filename);

    // 1. Кодируем файл
    printf("1. Encoding %s...\n", filename);
    encode_file(filename, encoded);

    // 2. Декодируем файл
    printf("\n2. Decoding %s...\n", encoded);
    decode_file(encoded, decoded);

    // 3. Сравниваем исходный и декодированный
    printf("\n3. Comparing files...\n");
    if (files_equal(filename, decoded)) {
        printf("SUCCESS: Original and decoded files are identical!\n");
    } else {
        printf("FAILURE: Files are different!\n");
    }

    // 4. Очистка временных файлов (опционально)
    printf("\n4. Cleaning up...\n");
    remove(encoded);
    remove(decoded);
    printf("Temporary files removed.\n");
}

// --- Главная функция ---
int main() {
    int choice;
    char filename[256];
    char encoded_filename[256];
    char decoded_filename[256];
    char filename2[256];

    printf("=== HUFFMAN ENCODING PROGRAM ===\n");
    printf("Efficient file compression using Huffman coding\n\n");

    while (1) {
        print_menu();

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n'); // Очищаем буфер ввода
            continue;
        }

        getchar(); // Убираем символ новой строки

        switch (choice) {
            case 1: // Кодирование файла
                printf("\n--- File Encoding ---\n");
                printf("Enter source filename: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strcspn(filename, "\n")] = '\0'; // Убираем \n

                snprintf(encoded_filename, sizeof(encoded_filename),
                        "%s.huff", filename);

                printf("Input:  %s\n", filename);
                printf("Output: %s\n", encoded_filename);

                encode_file(filename, encoded_filename);
                break;

            case 2: // Декодирование файла
                printf("\n--- File Decoding ---\n");
                printf("Enter .huff filename: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strcspn(filename, "\n")] = '\0';

                // Проверяем, есть ли .huff расширение
                if (strstr(filename, ".huff") == NULL) {
                    printf("Warning: file doesn't have .huff extension\n");
                }

                // Генерируем имя для декодированного файла
                strcpy(decoded_filename, filename);
                char* dot = strstr(decoded_filename, ".huff");
                if (dot) {
                    *dot = '\0';
                }
                strcat(decoded_filename, "_decoded.bin");

                printf("Input:  %s\n", filename);
                printf("Output: %s\n", decoded_filename);

                decode_file(filename, decoded_filename);
                break;

            case 3: // Тестирование
                printf("\n--- Test Mode ---\n");
                printf("Enter filename to test: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strcspn(filename, "\n")] = '\0';

                // Проверяем существование файла
                FILE* test = fopen(filename, "rb");
                if (!test) {
                    printf("Error: file '%s' not found\n", filename);
                } else {
                    fclose(test);
                    test_encoding_decoding(filename);
                }
                break;

            case 4: // Показать словарь
                printf("\n--- Show Dictionary ---\n");
                printf("Enter filename: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strcspn(filename, "\n")] = '\0';

                uint32_t* freq = count_frequencies(filename);
                if (freq) {
                    char** codes = build_huffman_dictionary(freq);
                    if (codes) {
                        print_dictionary(codes, freq);

                        // Освобождаем память
                        for (int i = 0; i < 256; i++) free(codes[i]);
                        free(codes);
                    }
                    free(freq);
                } else {
                    printf("Error: cannot read file or file is empty\n");
                }
                break;

            case 5: // Сравнить два файла
                printf("\n--- Compare Files ---\n");
                printf("Enter first filename: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strcspn(filename, "\n")] = '\0';

                printf("Enter second filename: ");
                fgets(filename2, sizeof(filename2), stdin);
                filename2[strcspn(filename2, "\n")] = '\0';

                if (files_equal(filename, filename2)) {
                    printf("Files are identical\n");
                } else {
                    printf("Files are different\n");
                }
                break;

            case 6: // Выход
                printf("\nThank you for using Huffman Encoder!\n");
                printf("Exiting program...\n");
                return 0;

            default:
                printf("Invalid choice. Please enter a number between 1 and 6.\n");
                break;
        }

        printf("\nPress Enter to continue...");
        getchar(); // Ждем нажатия Enter
    }

    return 0;
}