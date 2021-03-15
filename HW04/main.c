/**
 * Сборка проекта происходит в файле make.sh
 * Библтотека cJSON собрана статически (lib/cJSON/libcJSON.a)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <curl/curl.h>
#include "lib/cJSON/cJSON.h"

#define API_URL "https://www.metaweather.com/api"
#define API_URN_SEARCH "/location/search/?query="
#define API_URN_LOCATION "/api/location/"
#define MAX_BUF 65536

char wr_buf[MAX_BUF + 1]; // буфер полученных данных
int wr_index;

/**
 * Структура описания температуры в локации
 */
typedef struct {
    char *applicable_date; // время создания записи о погоде
    char *weather_description; // описание погоды
    char *wind_speed; // скорость ветра
    char *wind_direction; // направление ветра
    char *min_temp; // минимальная температура
    char *max_temp; // максимальная температура
} meteo;

/**
 * парсим/получаем woeid из json строки
 * @param json - строка json которую разбираем
 * @return 0|<int> - 0 не удалось определить woeid | woeid
 */
int woeid_from_json(const char *const json) {
    cJSON *parsed_json = cJSON_Parse(json);
    if (parsed_json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return 0;
    }

    const cJSON *citys = NULL;
    cJSON_ArrayForEach (citys, parsed_json) {
        cJSON *woeid = cJSON_GetObjectItemCaseSensitive(citys, "woeid");
        if (cJSON_IsNumber(woeid)) {
            return woeid->valueint;
        }
    }

    cJSON_Delete(parsed_json);

    return 0;
}

/**
 * Получаем все данные по погоде в выбраной локации
 * @param json - строка json из которой получаем даныне
 * @return GSList* meteo - однонапрвленный список с заполненной структурой meteo (данные по погоде)
 */
GSList* weather_from_json(const char *const json) {
    GSList *list_weather = NULL;

    cJSON *parsed_json = cJSON_Parse(json);
    if (parsed_json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return list_weather;
    }

    cJSON *consolidated_weather = cJSON_GetObjectItemCaseSensitive(parsed_json, "consolidated_weather");
    const cJSON *weather = NULL;
    cJSON_ArrayForEach(weather, consolidated_weather) {
        meteo* mt;
        mt = g_new(meteo, 1);

        cJSON *weather_state_name = cJSON_GetObjectItemCaseSensitive(weather, "weather_state_name");
        if (cJSON_IsString(weather_state_name) && (weather_state_name->valuestring != NULL)) {
            mt->weather_description = cJSON_Print(weather_state_name);
        }

        cJSON *applicable_date = cJSON_GetObjectItemCaseSensitive(weather, "applicable_date");
        if (cJSON_IsString(applicable_date) && (applicable_date->valuestring != NULL)) {
            mt->applicable_date = cJSON_Print(applicable_date);
        }

        cJSON *wind_speed = cJSON_GetObjectItemCaseSensitive(weather, "wind_speed");
        if (cJSON_IsNumber(wind_speed)) {
            mt->wind_speed = cJSON_Print(wind_speed);
        }

        cJSON *wind_direction = cJSON_GetObjectItemCaseSensitive(weather, "wind_direction");
        if (cJSON_IsNumber(wind_direction)) {
            mt->wind_direction = cJSON_Print(wind_direction);
        }

        cJSON *min_temp = cJSON_GetObjectItemCaseSensitive(weather, "min_temp");
        if (cJSON_IsNumber(min_temp)) {
            mt->min_temp = cJSON_Print(min_temp);
        }

        cJSON *max_temp = cJSON_GetObjectItemCaseSensitive(weather, "max_temp");
        if (cJSON_IsNumber(max_temp)) {
            mt->max_temp = cJSON_Print(max_temp);
        }

        list_weather = g_slist_append(list_weather, mt);
    }

    cJSON_Delete(parsed_json);

    return list_weather;
}

/**
 * Метод инициализации глобального буфера данных wr_buf данными полученными запросом curl
 * @param buffer - буфер данных который записываем в глобальную переменную wr_buf
 * @param size
 * @param nmemb
 * @param userp
 * @return 0|<int> - 0 не удалось произвести действия | <int> размер прочитанного в байтах
 */
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    /** Определяем размер полученных данных */
    int segsize = size * nmemb;

    /** Проверяем чтобы размер ответа не превышал буфер результата */
    if (wr_index + segsize > MAX_BUF) {
        *(int *) userp = 1;
        perror("ERROR: Размер данных превышает размер буфера\n");
        return 0;
    }

    /** Записываем полученные данные в буфер */
    memcpy((void *) &wr_buf[wr_index], buffer, (size_t) segsize);

    /* Update the write index */
    wr_index += segsize;

    /* Последним в конец строки символ конца строки */
    wr_buf[wr_index] = '\0';

    /* Возвращаем кол-во полученных байт */
    return segsize;
}

/**
 * Выполнение запроса curl
 * @param url - url которому обращаемся
 * @return 0|<code> - 0 запрос прошел успешно | <code> код ошибки
 */
int curl(const char *url) {
    CURL *curl = NULL;
    CURLcode res;
    wr_index = 0;

    /** Инициализируем curl */
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        perror("ERROR: Не удалось инициализировать curl.\n");
        exit(1);
    }

    /** устанавливаем опции curl */
    curl_easy_setopt(curl, CURLOPT_URL, url); // url обращения
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // не проверяем ssl
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // Указываем в какую функцию передать результат

    /** непосредственно запрос curl */
    res = curl_easy_perform(curl);

    /* Проверка на ошибки */
    if (res != CURLE_OK) {
        fprintf(stderr, "ERROR code=(%d): curl_easy_perform() failed: %s\n", res, curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return res;
    }

    /* останавливаем curl */
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return 0;
}

/**
 * Вывод данных о погоде
 * @param meteo* met - структура с данными о погоде
 */
void print_meteo(meteo *met) {
    printf("Дата: %s\n", met->applicable_date);
    printf("Описание погоды: %s\n", met->weather_description);
    printf("Скорость ветра: %s\n", met->wind_speed);
    printf("Направление ветра (градусы): %s\n", met->wind_direction);
    printf("Минимальная температура: %s\n", met->min_temp);
    printf("Максимальная температура: %s\n", met->max_temp);
    printf("\n");
}

int main(int argc, char *argv[]) {
    /** Проверяем переданы ли все аргументы */
    if (argc < 2) {
        perror("ERROR: Переданы не все аргументы.\n");
        exit(1);
    }

    printf("Поиск производим в локации: %s\n", argv[1]);

    /** поиск локации (woeid) */
    char url_c[1024] = API_URL;
    char urn_search[1024] = API_URN_SEARCH;
    strcat(urn_search, argv[1]); // получаем urn для поиска локации
    strcat(url_c, urn_search); // получаем url поиска локации
    if (curl(url_c)) {
        fprintf(stderr, "ERROR: Не удалось определить woeid локации\n");
        exit(1);
    }

    int woeid = woeid_from_json(wr_buf);
    if(!woeid) {
        fprintf(stderr, "ERROR: Передана не корректная локация\n");
        exit(1);
    }
    char str_woeid[12];
    sprintf(str_woeid, "%d", woeid);

    /** получаем данные локации */
    char url_d[1024] = API_URL;
    char urn_location[1024] = API_URN_LOCATION;
    strcat(url_d, urn_location);
    strcat(url_d, str_woeid);
    strcat(url_d, "/");
    if (curl(url_d)) {
        fprintf(stderr, "ERROR: Не удалось получить данные по локации\n");
        exit(1);
    }

    GSList *list_weather = weather_from_json(wr_buf);

    printf("\nПогода на сегодня\n");
    print_meteo(g_slist_nth_data(list_weather, 0));

    printf("\nПогода на несколько дней\n");
    g_slist_foreach(list_weather, (GFunc)print_meteo, NULL);

    g_slist_free(list_weather); // не знаю, правильно ли освобождаю память, возможно нужно написать функцию и передать ее в g_slist_free_full ()

    return 0;
}
