#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <curl/curl.h>
#include <time.h>

struct memory {
    char *response;
    size_t size;
};

/* Prototipos */
static size_t cb(char *data, size_t size, size_t nmemb, void *clientp);

int http_get(const char *url, char **out_response);

int json_get_max_update_id(const char *json, unsigned long long *max_id);
int json_get_last_chat_id(const char *json, unsigned long long *chat_id);
int json_get_last_first_name(const char *json, char *out, size_t out_sz);
int json_get_last_text(const char *json, char *out, size_t out_sz);
int json_get_last_date(const char *json, long long *unix_time);

int preparar_respuesta(const char *text, const char *nombre,
                       char *reply, size_t reply_sz);
int url_encode_spaces(const char *src, char *dst, size_t dst_sz);
int log_event(FILE *log, long long date, const char *name, const char *msg);

int contiene_subcadena_ci(const char *texto, const char *patron);

int leer_token_desde_archivo(const char *filename, char *token, size_t token_sz);

/* Callback de libcurl: acumula respuesta en memoria */
static size_t cb(char *data, size_t size, size_t nmemb, void *clientp) {
    size_t realsize = nmemb;
    struct memory *mem = (struct memory *)clientp;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (!ptr) {
        return 0; /* out of memory */
    }

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

/* Realiza un GET HTTP y devuelve el JSON en *out_response (malloc). */
int http_get(const char *url, char **out_response) {
    CURL *curl;
    CURLcode res;
    struct memory chunk = {0};

    if (url == NULL || out_response == NULL) {
        printf("Error: puntero nulo en http_get\n");
        return 1;
    }

    curl = curl_easy_init();
    if (!curl) {
        printf("Error: no se pudo inicializar CURL\n");
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        printf("Error: fallo la solicitud HTTP. Código: %d\n", res);
        curl_easy_cleanup(curl);
        free(chunk.response);
        return 1;
    }

    curl_easy_cleanup(curl);

    *out_response = chunk.response; /* el caller debe hacer free */
    return 0;
}

/* Obtiene el máximo update_id del JSON */
int json_get_max_update_id(const char *json, unsigned long long *max_id) {
    const char *p;
    unsigned long long tmp = 0;
    int found = 0;

    if (json == NULL || max_id == NULL) {
        printf("Error: puntero nulo en json_get_max_update_id\n");
        return 1;
    }

    p = json;
    while ((p = strstr(p, "\"update_id\":")) != NULL) {
        unsigned long long uid;

        p += strlen("\"update_id\":");
        uid = strtoull(p, NULL, 10);
        if (!found || uid > tmp) {
            tmp = uid;
            found = 1;
        }
    }

    if (!found) {
        /* No es un error grave: simplemente no hay mensajes nuevos */
        return 1;
    }

    *max_id = tmp;
    return 0;
}

/* Obtiene el chat.id del último mensaje */
int json_get_last_chat_id(const char *json, unsigned long long *chat_id) {
    const char *p;
    const char *last_chat = NULL;

    if (json == NULL || chat_id == NULL) {
        printf("Error: puntero nulo en json_get_last_chat_id\n");
        return 1;
    }

    p = json;
    while ((p = strstr(p, "\"chat\"")) != NULL) {
        last_chat = p;
        p += strlen("\"chat\"");
    }

    if (last_chat == NULL) {
        printf("Error: no se encontró 'chat' en JSON\n");
        return 1;
    }

    p = strstr(last_chat, "\"id\":");
    if (p == NULL) {
        printf("Error: no se encontró 'id' dentro de 'chat'\n");
        return 1;
    }

    p += strlen("\"id\":");
    *chat_id = strtoull(p, NULL, 10);

    return 0;
}

/* Obtiene chat.first_name del último mensaje */
int json_get_last_first_name(const char *json, char *out, size_t out_sz) {
    const char *p;
    const char *last_chat = NULL;
    size_t i = 0;

    if (json == NULL || out == NULL || out_sz == 0) {
        printf("Error: puntero nulo en json_get_last_first_name\n");
        return 1;
    }

    p = json;
    while ((p = strstr(p, "\"chat\"")) != NULL) {
        last_chat = p;
        p += strlen("\"chat\"");
    }

    if (last_chat == NULL) {
        printf("Error: no se encontró 'chat' en JSON\n");
        return 1;
    }

    p = strstr(last_chat, "\"first_name\":\"");
    if (p == NULL) {
        printf("Error: no se encontró 'first_name' dentro de 'chat'\n");
        return 1;
    }

    p += strlen("\"first_name\":\"");

    while (*p != '"' && *p != '\0' && i + 1 < out_sz) {
        out[i++] = *p;
        p++;
    }
    out[i] = '\0';

    if (i == 0) {
        printf("Error: 'first_name' vacío\n");
        return 1;
    }

    return 0;
}

/* Obtiene el texto del último mensaje */
int json_get_last_text(const char *json, char *out, size_t out_sz) {
    const char *p;
    const char *last_text = NULL;
    size_t i = 0;

    if (json == NULL || out == NULL || out_sz == 0) {
        printf("Error: puntero nulo en json_get_last_text\n");
        return 1;
    }

    p = json;
    while ((p = strstr(p, "\"text\"")) != NULL) {
        last_text = p;
        p += strlen("\"text\"");
    }

    if (last_text == NULL) {
        printf("Error: no se encontró 'text' en JSON\n");
        return 1;
    }

    p = strstr(last_text, "\"text\":\"");
    if (p == NULL) {
        printf("Error: formato de 'text' inválido\n");
        return 1;
    }

    p += strlen("\"text\":\"");

    while (*p != '"' && *p != '\0' && i + 1 < out_sz) {
        out[i++] = *p;
        p++;
    }
    out[i] = '\0';

    if (i == 0) {
        printf("Advertencia: texto vacío en mensaje\n");
    }

    return 0;
}

/* Obtiene el campo date (unix time) del último mensaje */
int json_get_last_date(const char *json, long long *unix_time) {
    const char *p;
    long long tmp = 0;
    int found = 0;

    if (json == NULL || unix_time == NULL) {
        printf("Error: puntero nulo en json_get_last_date\n");
        return 1;
    }

    p = json;
    while ((p = strstr(p, "\"date\":")) != NULL) {
        long long d;
        p += strlen("\"date\":");
        d = atoll(p);
        tmp = d;
        found = 1;
    }

    if (!found) {
        printf("Error: no se encontró 'date' en JSON\n");
        return 1;
    }

    *unix_time = tmp;
    return 0;
}

/* Busca si 'patron' aparece dentro de 'texto' sin distinguir mayúsculas/minúsculas */
int contiene_subcadena_ci(const char *texto, const char *patron) {
    size_t len_patron;
    const char *p;

    if (texto == NULL || patron == NULL) {
        return 0;
    }

    len_patron = strlen(patron);
    if (len_patron == 0) {
        return 0;
    }

    p = texto;
    while (*p != '\0') {
        if (strncasecmp(p, patron, len_patron) == 0) {
            return 1;
        }
        p++;
    }

    return 0;
}

/* Prepara el texto de respuesta según saludo recibido */
int preparar_respuesta(const char *text, const char *nombre,
                       char *reply, size_t reply_sz) {
    if (text == NULL || nombre == NULL || reply == NULL || reply_sz == 0) {
        printf("Error: puntero nulo en preparar_respuesta\n");
        return 1;
    }

    reply[0] = '\0';

    if (contiene_subcadena_ci(text, "hola")) {
        snprintf(reply, reply_sz, "Hola, %s", nombre);
    } else if (contiene_subcadena_ci(text, "chau")) {
        snprintf(reply, reply_sz, "Chau, %s", nombre);
    } else {
        /* No es un saludo reconocido: no respondemos */
        reply[0] = '\0';
    }

    return 0;
}

/* Reemplaza espacios por %20 (URL encode mínimo) */
int url_encode_spaces(const char *src, char *dst, size_t dst_sz) {
    size_t i = 0, j = 0;

    if (src == NULL || dst == NULL || dst_sz == 0) {
        printf("Error: puntero nulo en url_encode_spaces\n");
        return 1;
    }

    while (src[i] != '\0') {
        if (src[i] == ' ') {
            if (j + 3 >= dst_sz) {
                printf("Error: buffer insuficiente en url_encode_spaces\n");
                return 1;
            }
            dst[j++] = '%';
            dst[j++] = '2';
            dst[j++] = '0';
        } else {
            if (j + 1 >= dst_sz) {
                printf("Error: buffer insuficiente en url_encode_spaces\n");
                return 1;
            }
            dst[j++] = src[i];
        }
        i++;
    }

    if (j >= dst_sz) {
        printf("Error: buffer insuficiente al terminar url_encode_spaces\n");
        return 1;
    }

    dst[j] = '\0';
    return 0;
}

/* Registra mensaje en archivo de log */
int log_event(FILE *log, long long date, const char *name, const char *msg) {
    if (log == NULL || name == NULL || msg == NULL) {
        printf("Error: puntero nulo en log_event\n");
        return 1;
    }

    fprintf(log, "%lld;%s;%s\n", date, name, msg);
    fflush(log);

    return 0;
}

/* Lee el token desde un archivo (primera línea) */
int leer_token_desde_archivo(const char *filename, char *token, size_t token_sz) {
    FILE *fPtr;
    char *nl;

    if (filename == NULL || token == NULL || token_sz == 0) {
        printf("Error: puntero nulo en leer_token_desde_archivo\n");
        return 1;
    }

    fPtr = fopen(filename, "r");
    if (fPtr == NULL) {
        printf("Error: no se pudo abrir el archivo de token '%s'\n", filename);
        return 1;
    }

    if (fgets(token, (int)token_sz, fPtr) == NULL) {
        printf("Error: no se pudo leer el token desde '%s'\n", filename);
        fclose(fPtr);
        return 1;
    }

    fclose(fPtr);

    nl = strpbrk(token, "\r\n");
    if (nl != NULL) {
        *nl = '\0';
    }

    if (token[0] == '\0') {
        printf("Error: token vacío en archivo '%s'\n", filename);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    char token[256];
    unsigned long long offset = 0;
    FILE *log_file;

    if (argc != 2) {
        printf("Uso: %s token.txt\n", argv[0]);
        return 1;
    }

    if (leer_token_desde_archivo(argv[1], token, sizeof(token)) != 0) {
        return 1;
    }

    log_file = fopen("bot_log.txt", "a");
    if (log_file == NULL) {
        printf("Error: no se pudo abrir 'bot_log.txt' para escritura\n");
        return 1;
    }

    while (1) {
        char url[1024];
        char *json_resp = NULL;

        if (offset > 0) {
            snprintf(url, sizeof(url),
                     "https://api.telegram.org/bot%s/getUpdates?offset=%llu",
                     token, offset);
        } else {
            snprintf(url, sizeof(url),
                     "https://api.telegram.org/bot%s/getUpdates",
                     token);
        }

        if (http_get(url, &json_resp) != 0) {
            fclose(log_file);
            return 1;
        }

        if (strstr(json_resp, "\"ok\":false") != NULL) {
            printf("Error: respuesta de Telegram indica fallo:\n%s\n", json_resp);
            free(json_resp);
            fclose(log_file);
            return 1;
        }

        if (strstr(json_resp, "\"result\": []") != NULL ||
            strstr(json_resp, "\"result\":[]") != NULL ||
            strstr(json_resp, "\"update_id\":") == NULL) {
            free(json_resp);
            sleep(2);
            continue;
        }

        {
            unsigned long long max_update_id;
            unsigned long long chat_id;
            char first_name[128];
            char text[512];
            long long date_unix;
            char reply[512];
            char reply_encoded[1024];
            char url_send[1024];
            char *json_send = NULL;

            if (json_get_max_update_id(json_resp, &max_update_id) != 0) {
                free(json_resp);
                sleep(2);
                continue;
            }

            if (json_get_last_chat_id(json_resp, &chat_id) != 0) {
                free(json_resp);
                fclose(log_file);
                return 1;
            }

            if (json_get_last_first_name(json_resp, first_name,
                                         sizeof(first_name)) != 0) {
                free(json_resp);
                fclose(log_file);
                return 1;
            }

            if (json_get_last_text(json_resp, text, sizeof(text)) != 0) {
                free(json_resp);
                fclose(log_file);
                return 1;
            }

            if (json_get_last_date(json_resp, &date_unix) != 0) {
                free(json_resp);
                fclose(log_file);
                return 1;
            }

            if (log_event(log_file, date_unix, first_name, text) != 0) {
                free(json_resp);
                fclose(log_file);
                return 1;
            }

            if (preparar_respuesta(text, first_name,
                                   reply, sizeof(reply)) != 0) {
                free(json_resp);
                fclose(log_file);
                return 1;
            }

            if (reply[0] != '\0') {
                if (url_encode_spaces(reply, reply_encoded,
                                      sizeof(reply_encoded)) != 0) {
                    free(json_resp);
                    fclose(log_file);
                    return 1;
                }

                snprintf(url_send, sizeof(url_send),
                         "https://api.telegram.org/bot%s/sendMessage?chat_id=%llu&text=%s",
                         token, chat_id, reply_encoded);

                if (http_get(url_send, &json_send) != 0) {
                    free(json_resp);
                    fclose(log_file);
                    return 1;
                }

                {
                    long long now = (long long)time(NULL);
                    if (log_event(log_file, now, "BOT", reply) != 0) {
                        free(json_resp);
                        free(json_send);
                        fclose(log_file);
                        return 1;
                    }
                }

                free(json_send);
            }

            offset = max_update_id + 1ULL;
        }

        free(json_resp);
        sleep(2);
    }

    /* No se llega aquí normalmente */
    /* fclose(log_file); */
    /* return 0; */
}
