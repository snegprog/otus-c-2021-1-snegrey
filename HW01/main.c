#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *utf8 = "alfavit_utf8.txt";
static const char *cp125 = "alfavit_cp1251.txt";
static const char *koi8 = "alfavit_koi8.txt";
static const char *iso_8859_5 = "alfavit_iso-8859-5.txt";

static const char *code_cp1251 = "cp1251";
static const char *code_koi8 = "koi8";
static const char *code_iso_8859_5 = "iso-8859-5";

/**
 * Получаем таблицу символов кодировки переданного файла
 * @param name_file - файл с набором символов выбранной кодировки
 * @param symbols - результирующая таблица символов
 */
void character_table(const char *name_file, char symbols[66][3]) {
    FILE *fp;
    if ((fp = fopen(name_file, "r")) == NULL) {
        printf("Не удалось открыть файл %s\n", name_file);
        exit(1);
    }

    // Набор байт представляет кодировку символа
    static char cc[8];
    int iter = 0;
    while ((fgets(cc, 8, fp)) != NULL) {
        // Узнаем сколько байт используется для указания символа
        int it = 0;
        for (int i = 0;; ++i) {
            if (cc[i] == 10) {
                break;
            }
            it++;
        }

        // инициализируем результирующий массив
        for (int i = 0; i < it; ++i) {
            symbols[iter][i] = cc[i];
        }
        symbols[iter][it] = '\0'; // Паследним символом задаем нуль-символ, чтобы обозначить конец строки
        iter++;
    }
    fclose(fp);
}

/**
 * Получаем индекс символа в таблице символов.
 * @param symbol - символ для которого получаем индекс.
 * @param from - таблица символов в которой ищем символ.
 * @return index|-1 - индекс символа в таблице или -1 если символ в таблице не найден.
 */
int index_symbol(char symbol, char from[][3]) {
    for (int i = 0; i < 66; ++i) {
        if (symbol == from[i][0]) {
            return i;
        }
    }

    return -1;
}

/**
 * Декодер файла из кодировки cp125|koi8|iso-8859-5 в utf8
 * @param argc - кол-во входящих аргументов
 * @param argv[1] - файл который требуется раскодировать
 * @param argv[2] - кодировка входного файла cp125|koi8|iso-8859-5
 * @param argv[3] - выходной файл
 * @return 0|exit(1)
 */
int main(int argc, char *argv[]) {
    /** Проверяем переданы ли все аргументы */
    if (argc < 4) {
        printf("ERROR: Переданы не все аргументы.\n");
        exit(1);
    }

    /** проверяем чтобы файл результата не был файлом с кодировками, во избижания затирания кодировки */
    if (!strcmp(argv[3], utf8)
        || !strcmp(argv[3], cp125)
        || !strcmp(argv[3], koi8)
        || !strcmp(argv[3], iso_8859_5)) {
        printf("ERROR: Попытка переписать файл кодировки.\n");
        exit(1);
    }

    /** проверяем корректность аргумента кодировки файла который декодируем в utf8 */
    if (strcmp(argv[2], code_cp1251)
        && strcmp(argv[2], code_koi8)
        && strcmp(argv[2], code_iso_8859_5)) {
        printf("ERROR: Допустимые кодировки cp125|koi8|iso-8859-5.\n");
        exit(1);
    }

    /**  создаем таблицы символов */
    static char symbols_from[66][3]; // таблица символов кодировки исходного файла (индекс символа соответствует индексу символа в таблице кодировки symbols_utf8)
    static char symbols_utf8[66][3]; // таблица символов кодировки utf8 (индекс символа соответствует индексу символа в таблице кодировки symbols_from)
    character_table(utf8, symbols_utf8);
    if (!strcmp(argv[2], code_cp1251)) {
        character_table(cp125, symbols_from);
    }
    if (!strcmp(argv[2], code_koi8)) {
        character_table(koi8, symbols_from);
    }
    if (!strcmp(argv[2], code_iso_8859_5)) {
        character_table(iso_8859_5, symbols_from);
    }

    FILE *f_to;
    if ((f_to = fopen(argv[3], "w")) == NULL) {
        printf("Не удалось открыть файл результата '%s'\n", argv[3]);
        exit(1);
    }
    FILE *f_from;
    if ((f_from = fopen(argv[1], "r")) == NULL) {
        printf("Не удалось открыть декодируемый файл '%s'\n", argv[1]);
        exit(1);
    }

    /** непосредственно декодирование */
    int index; // индекс символа в таблице символов
    int in; // значение возвращаемое fgetc
    char cc; // Сивол который декодируем
    while ((in = fgetc(f_from)) != EOF) {
        cc = (char) in;
        index = index_symbol(cc, symbols_from);
        if (index == -1) {
            fputc(in, f_to);
            continue;
        }

        for (int i = 0; i < 2; ++i) {
            fputc((int)symbols_utf8[index][i], f_to);
        }
    };

    fclose(f_from);
    fclose(f_to);

    return 0;
}