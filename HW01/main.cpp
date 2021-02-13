#include <stdio.h>
#include <stdlib.h>

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
 * @param simbols - результирующая таблица символов
 */
void character_table(const char *name_file, char simbols[66][7]) {
    FILE *fp;
    if ((fp = fopen(name_file, "r")) == NULL) {
        printf("Не удалось открыть файл %s\n", name_file);
        exit(1);
    }

    // Набор байт представляет кодировку символа
    static char cc[8];
    int iter = 0;
    while ((fgets(cc, 8, fp)) != nullptr) {
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
            simbols[iter][i] = cc[i];
        }
        simbols[iter][it] = '\0'; // Паследним символом задаем нуль-символ, чтобы обозначить конец строки
        iter++;
    }
    fclose(fp);
}

/**
 * Сравниваем строки.
 * @param str0 - строка сравнения
 * @param str1 - строка сравнения
 * @return 0|1 - 0 не одинаковые, 1 одинаковые
 */
int compare_strings(const char *str0, const char *str1) {
    for (int i = 0;; ++i) {
        if (str0[i] != str1[i]) {
            return 0;
        }

        // если строка закончилась там и там, то заканяиваем сравнение
        if (str0[i] == '\0' && str1[i] == '\0') {
            break;
        }

        // если где то строка закончилась, а где то нет, то возвращаем лож
        if (str0[i] == '\0' || str1[i] == '\0') {
            return 0;
        }
    }

    return 1;
}

/**
 * Получаем индекс символа в таблице символов.
 * @param simbol - символ для которого получаем индекс.
 * @param from - таблица символов в которой ищем символ.
 * @return index|-1 - индекс символа в таблице или -1 если символ в таблице не найден.
 */
int index_simbol(char simbol, char from[66][7]) {
    char str[2] = {simbol, '\0'};
    for (int i = 0; i < 66; ++i) {
        if (compare_strings(str, from[i])) {
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
    if (compare_strings(argv[3], utf8)
        || compare_strings(argv[3], cp125)
        || compare_strings(argv[3], koi8)
        || compare_strings(argv[3], iso_8859_5)) {
        printf("ERROR: Попытка переписать файл кодировки.\n");
        exit(1);
    }

    /** проверяем корректность аргумента кодировки файла который декодируем в utf8 */
    if (!compare_strings(argv[2], code_cp1251)
        && !compare_strings(argv[2], code_koi8)
        && !compare_strings(argv[2], code_iso_8859_5)) {
        printf("ERROR: Допустимые кодировки cp125|koi8|iso-8859-5.\n");
        exit(1);
    }

    /**  создаем таблицы символов */
    static char simbols_from[66][7]; // таблица символов кодировки исходного файла (индекс символа соответствует индексу символа в таблице кодировки simbols_utf8)
    static char simbols_utf8[66][7]; // таблица символов кодировки utf8 (индекс символа соответствует индексу символа в таблице кодировки simbols_from)
    character_table(utf8, simbols_utf8);
    if (compare_strings(argv[2], code_cp1251)) {
        character_table(cp125, simbols_from);
    }
    if (compare_strings(argv[2], code_koi8)) {
        character_table(koi8, simbols_from);
    }
    if (compare_strings(argv[2], code_iso_8859_5)) {
        character_table(iso_8859_5, simbols_from);
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
        index = index_simbol(cc, simbols_from);
        if (index == -1) {
            fputc(in, f_to);
            continue;
        }

        for (int i = 0; i < 2; ++i) {
            fputc((int)simbols_utf8[index][i], f_to);
        }
    };

    fclose(f_from);
    fclose(f_to);

    return 0;
}