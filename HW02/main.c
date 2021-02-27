#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define EOCDR_SIGNATURE 0x06054b50
#define CFH_SIGNATURE 0x02014b50

typedef struct {
    uint16_t disk_number;        /* Number of this disk. */
    uint16_t start_disk_number;   /* Nbr. of disk with start of the CD. */
    uint16_t number_central_directory_record; /* Nbr. of CD entries on this disk. */
    uint16_t total_central_directory_record;      /* Nbr. of Central Directory entries. */
    uint32_t size_of_central_directory;         /* Central Directory size in bytes. */
    uint32_t central_directory_offset;       /* Central Directory file offset. */
    uint16_t comment_length;     /* Archive comment length. */
} eocdr;

/**
 * Получаем размер файла в байтах
 * @param fp - указатель на файл размер которого узнаем
 * @return long int - размер файла в байтах
 */
unsigned long filesize(FILE *fp) {
    unsigned long save_pos, size_of_file;
    save_pos = ftell(fp); // сохраняем начальную позицию чтения файла для дальнейшего возврата к ней
    fseek(fp, 0L, SEEK_END); // смещаем указатель позиции чтения на конец файла
    size_of_file = ftell(fp); // получаем позицию после смещения в конец файла
    fseek(fp, save_pos, SEEK_SET); // Возвращаем позицию чтения файла на начало
    return (size_of_file); // возвращаем размер файла в байтах, т.е. номер позиции конца файла
}

/**
 * Поиск позиции eocdr
 * @param fp - указатель на поток файла в котором изем eocdr
 * @return 0|<unsigned long> - 0 eocdr не найден | <unsigned long> позиция eocdr от начала файла
 */
unsigned long find_eocdr(FILE *fp) {
    unsigned long fp_size = filesize(fp);
    unsigned long offset;
    // Перебираем по байтам, с конца файла минус размер структуры eocdr, в поисках сигнатуры блока eocdr
    for (offset = fp_size - sizeof(eocdr); offset != 0; --offset) {
        uint32_t signature = 0;

        // Непосредственно смещение позиции на заданный размер с конца файла
        if (fseek(fp, offset, SEEK_SET) != 0) {
            printf("ERROR: Не удалось сместить позицию в файле.\n");
            fclose(fp);
            exit(1);
        }

        // Читаем из файла 4 байта и проверяем равно ли значение в них сигнатуре eocdr zip
        fread(&signature, sizeof(uint32_t), 1, fp);
        if (signature == EOCDR_SIGNATURE) {
            break;
        }
    }

    return offset;
}

/**
 * Читаем блок eocdr
 * @param fp - указатель на поток файла
 * @param end_record - структура куда считываем eocdr
 * @return 0|<offset> - 0 если не нашли запись eocdr | <offset> позиция записи eocdr
 */
unsigned long read_eocdr(FILE *fp, eocdr *end_record) {
    if(find_eocdr(fp) == 0) {
        return 0;
    }

    // Смещаемся от конца файла на минимальный
    if (fseek(fp, find_eocdr(fp)+ sizeof(uint32_t), SEEK_SET) != 0) {
        printf("ERROR: Не удалось сместить позицию в файле (read_eocdr).\n");
        fclose(fp);
        exit(1);
    }
    fread(end_record, sizeof(eocdr), 1, fp);

    return find_eocdr(fp);
}

int main(int argc, char *argv[]) {
    /** Проверяем переданы ли все аргументы */
    if (argc < 2) {
        printf("ERROR: Переданы не все аргументы.\n");
        exit(1);
    }

    FILE *fp;
    if ((fp = fopen(argv[1], "r")) == NULL) {
        printf("Не удалось открыть файл результата '%s'\n", argv[1]);
        exit(1);
    }

    eocdr end_record;
    unsigned long offset = read_eocdr(fp, &end_record);
    if (offset == 0) {
        fclose(fp);
        printf("Файл %s не содержит zip архива.\n", argv[1]);
        return 0;
    }

    /**
     * Смещаемся на первую запись центрального каталога.
     * Т.к. мы не знаем размер картинки положение центрального каталога получаем не исходя из значения
     * смещения от начала файла указанного в блоке eocdr, а путем вычитания размера центрального каталога
     * от позиции начала блока eocdr
     */
    fseek(fp, offset - end_record.size_of_central_directory, SEEK_SET);
    for (uint16_t i = 0; i < end_record.total_central_directory_record; ++i) {
        uint32_t signature;
        fread(&signature, sizeof(uint32_t), 1, fp);
        if (signature != CFH_SIGNATURE) {
            printf("ERROR: не найдена сигнатура центрального каталога\n");
            break;
        }

        // Перемещаемся на позицию где начинаются указания на размеры переменных величин (длина название файла, дополнительное поле, длина комментария)
        fseek(fp, 24, SEEK_CUR);
        // Получаем размеры переменных велечин
        uint16_t name_len, extra_len, comment_len;
        fread(&name_len, sizeof(uint16_t), 1, fp);
        fread(&extra_len, sizeof(uint16_t), 1, fp);
        fread(&comment_len, sizeof(uint16_t), 1, fp);

        // Перемещаемся на позицию названия файла
        fseek(fp, 12, SEEK_CUR);
        char name[name_len]; // строка название файла длиною полученной раннее (name_len)
        fread(&name, name_len, 1, fp);
        // Вывод названия файла
        for (int j = 0; j < name_len; ++j) {
            printf("%c", name[j]);
        }
        printf("\n");

        // Смещаем текущую позицию чтения файла на размер непрочитанных блоков
        fseek(fp, extra_len+comment_len, SEEK_CUR);
    }

    fclose(fp);

    return 0;
}
