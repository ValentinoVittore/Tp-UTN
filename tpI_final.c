// tpI_final.c
// Repo: https://github.com/ValentinoVittore/Tp-UTN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <ctype.h>

#define LOG_FILE "bot_log.txt"

struct memory {
    char *response;
    size_t size;
};

/* Prototipos */
static size_t cb(char *data, size_t size, size_t nmemb, void *clientp);
void init_chunk(struct memory *chunk);

int http_get(CURL *curl, struct memory *chunk, const char *url);
int leer_token_desde_archivo(const char *filename, char *token, size_t token_sz);
int parse_string_field(const char *json, const char *field_name, char *out, size_t maxlen);
int parse_long_field(const char *json, const char *field_name, long *value);
void to_lower_str(char *s);
void url_encode_spaces(const char *msg, char *out, size_t maxlen);
int log_event(const char *direction, long unix_time, const char *name, const char *text);


/* Callback de libcurl: acumula respuesta en memoria*/
static size_t cb(char *data, size_t size, size_t nmemb, void *clientp)
{
    size_t realsize = nmemb; /* igual que el código base */
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

// inicialización a cero de todos los campos
// struct memory chunk = {0};
void init_chunk(struct memory *chunk)
{
    chunk->response = NULL;
    chunk->size = 0;
}

/* GET HTTP usando CURL ya inicializado */
int http_get(CURL *curl, struct memory *chunk, const char *url)
{
    CURLcode res;

    if (curl == NULL || chunk == NULL || url == NULL) {
        printf("Error: puntero nulo en http_get\n");
        return 1;
    }

    init_chunk(chunk);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        printf("Error: fallo la solicitud HTTP. Código: %d\n", res);
        if (chunk->response != NULL) {
            free(chunk->response);
            chunk->response = NULL;
        }
        return 1;
    }

    if (chunk->response == NULL) {
        printf("Error: respuesta vacía desde el servidor\n");
        return 1;
    }

    return 0;
}

/* Lee el token desde un archivo (primera línea) */
int leer_token_desde_archivo(const char *filename, char *token, size_t token_sz)
{
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

/* Parsea un campo string del JSON: "campo": "valor" */
int parse_string_field(const char *json, const char *field_name, char *out, size_t maxlen)
{
    char pattern[128];
    const char *p;
    size_t i = 0;

    if (json == NULL || field_name == NULL || out == NULL || maxlen == 0) {
        printf("Error: puntero nulo en parse_string_field\n");
        return 1;
    }

    /* Siempre dejamos algo coherente en out */
    out[0] = '\0';

    snprintf(pattern, sizeof(pattern), "\"%s\"", field_name);

    p = strstr(json, pattern);
    if (!p) {
        /* CASO ESPECIAL: si el campo es "text" y no existe,
           lo tomamos como "no hay texto" (foto/audio/etc.),*/
        if (strcmp(field_name, "text") == 0) {
            /* out ya quedó como "" */
            /* Info opcional, podés comentar este printf si molesta */
            printf("Info: mensaje sin campo 'text' (foto/audio/etc.), se ignora.\n");
            return 0;  /* OK, pero sin texto */
        }

        /* Para el resto de campos seguimos considerándolo error */
        printf("Error: no se encontró campo '%s' en JSON\n", field_name);
        return 1;
    }

    p = strchr(p, ':');
    if (!p) {
        printf("Error: formato inválido para campo '%s'\n", field_name);
        return 1;
    }
    p++;
    while (*p == ' ' || *p == '\t') p++;

    if (*p != '\"') {
        printf("Error: campo '%s' no es string\n", field_name);
        return 1;
    }
    p++; /* saltar la comilla inicial */

    while (*p && *p != '\"' && i < maxlen - 1) {
        if (*p == '\\' && p[1] != '\0') {
            p++; /* saltear el backslash */
        }
        out[i++] = *p++;
    }
    out[i] = '\0';

    if (i == 0) {
        printf("Advertencia: campo string '%s' vacío\n", field_name);
    }

    return 0;
}


/* Parsea un campo numérico entero: "campo": 12345 */
int parse_long_field(const char *json, const char *field_name, long *value)
{
    char pattern[128];
    const char *p;

    if (json == NULL || field_name == NULL || value == NULL) {
        printf("Error: puntero nulo en parse_long_field\n");
        return 1;
    }

    snprintf(pattern, sizeof(pattern), "\"%s\"", field_name);

    p = strstr(json, pattern);
    if (!p) {
        printf("Error: no se encontró campo '%s' en JSON\n", field_name);
        return 1;
    }

    p = strchr(p, ':');
    if (!p) {
        printf("Error: formato inválido para campo '%s'\n", field_name);
        return 1;
    }
    p++;
    while (*p == ' ' || *p == '\t') p++;

    *value = strtol(p, NULL, 10);
    return 0;
}

/* Convierte string a minúsculas in-place */
void to_lower_str(char *s)
{
    while (*s) {
        *s = (char)tolower((unsigned char)*s);
        s++;
    }
}

/* Codifica espacios como %20 (suficiente para el TP; trunca si no entra) */
void url_encode_spaces(const char *msg, char *out, size_t maxlen)
{
    size_t j = 0;
    size_t i;

    if (msg == NULL || out == NULL || maxlen == 0) {
        return;
    }

    for (i = 0; msg[i] != '\0' && j < maxlen - 1; i++) {
        if (msg[i] == ' ') {
            if (j + 3 >= maxlen) {
                break; /* no hay lugar, se trunca */
            }
            out[j++] = '%';
            out[j++] = '2';
            out[j++] = '0';
        } else {
            out[j++] = msg[i];
        }
    }
    out[j] = '\0';
}

/* Loguea mensajes IN/OUT en archivo de texto */
int log_event(const char *direction, long unix_time, const char *name, const char *text)
{
    FILE *f;

    if (direction == NULL) direction = "-";
    if (name == NULL) name = "-";
    if (text == NULL) text = "-";

    f = fopen(LOG_FILE, "a");
    if (f == NULL) {
        printf("Error: no se pudo abrir '%s' para log\n", LOG_FILE);
        return 1;
    }

    fprintf(f, "%s;%ld;%s;%s\n", direction, unix_time, name, text);
    fclose(f);

    return 0;
}

int main(int argc, char *argv[])
{
    char token[256];
    char base_url[512];
    CURL *curl;
    struct memory chunk;
    long offset_update_id = -1; /* -1: sin offset aún */

    if (argc != 2) {
        printf("Uso: %s token.txt\n", argv[0]);
        return 1;
    }

    if (leer_token_desde_archivo(argv[1], token, sizeof(token)) != 0) {
        return 1;
    }

    snprintf(base_url, sizeof(base_url), "https://api.telegram.org/bot%s", token);

    curl = curl_easy_init();
    if (!curl) {
        printf("Error: no se pudo inicializar CURL\n");
        return 1;
    }

    printf("Bot iniciado. Esperando mensajes...\n");

    while (1) {
        char url_get[1024];

        if (offset_update_id >= 0) {
            snprintf(url_get, sizeof(url_get),
                     "%s/getUpdates?offset=%ld", base_url, offset_update_id);
        } else {
            snprintf(url_get, sizeof(url_get),
                     "%s/getUpdates", base_url);
        }

        if (http_get(curl, &chunk, url_get) != 0) {
            printf("Fallo en getUpdates\n");
            sleep(2);
            continue;
        }

        /* Error general de la API */
        if (strstr(chunk.response, "\"ok\":false") != NULL) {
            printf("Error: la API de Telegram devolvió fallo:\n%s\n", chunk.response);
            free(chunk.response);
            chunk.response = NULL;
            sleep(2);
            continue;
        }

        /* Sin mensajes nuevos */
        if (strstr(chunk.response, "\"result\": []") != NULL ||
            strstr(chunk.response, "\"result\":[]") != NULL) {
            free(chunk.response);
            chunk.response = NULL;
            sleep(2);
            continue;
        }

        /* Procesamos el primer mensaje de result (estilo Código 1) */

        /* update_id */
        long update_id = 0;
        if (parse_long_field(chunk.response, "update_id", &update_id) != 0) {
            free(chunk.response);
            chunk.response = NULL;
            sleep(2);
            continue;
        }

        /* Sección "message" */
        char *p_message = strstr(chunk.response, "\"message\"");
        if (!p_message) {
            printf("Error: no se encontró 'message' en JSON\n");
            free(chunk.response);
            chunk.response = NULL;
            /* Marcamos igual como leído para no trabarnos */
            offset_update_id = update_id + 1;
            sleep(2);
            continue;
        }

        long chat_id = 0;
        long msg_date = 0;
        char first_name[128] = "";
        char text[512] = "";

        /* Dentro de message, buscamos chat */
        char *p_chat = strstr(p_message, "\"chat\"");
        if (p_chat) {
            /* chat.id */
            if (parse_long_field(p_chat, "id", &chat_id) != 0) {
                chat_id = 0;
            }
            /* chat.first_name */
            if (parse_string_field(p_chat, "first_name", first_name, sizeof(first_name)) != 0) {
                first_name[0] = '\0';
            }
        }

        /* message.date */
        if (parse_long_field(p_message, "date", &msg_date) != 0) {
            msg_date = 0;
        }

        /* message.text */
        if (parse_string_field(p_message, "text", text, sizeof(text)) != 0) {
            text[0] = '\0';
        }

        /* Log IN: mensaje recibido */
        log_event("IN", msg_date, first_name, text);

        /* Decidir respuesta (hola/chau) */
        char respuesta[512] = "";
        if (text[0] != '\0') {
            char texto_lower[512];
            strncpy(texto_lower, text, sizeof(texto_lower) - 1);
            texto_lower[sizeof(texto_lower) - 1] = '\0';
            to_lower_str(texto_lower);

            if (strstr(texto_lower, "hola") != NULL) {
                snprintf(respuesta, sizeof(respuesta),
                         "Hola, %s", first_name[0] ? first_name : "usuario");
            } else if (strstr(texto_lower, "chau") != NULL) {
                snprintf(respuesta, sizeof(respuesta),
                         "Chau %s, que tengas buen día!",
                         first_name[0] ? first_name : "");
            }
        }

        if (respuesta[0] != '\0' && chat_id != 0) {
            char respuesta_codificada[1024];
            char url_send[2048];
            struct memory chunk_send;

            url_encode_spaces(respuesta, respuesta_codificada,
                              sizeof(respuesta_codificada));

            snprintf(url_send, sizeof(url_send),
                     "%s/sendMessage?chat_id=%ld&text=%s",
                     base_url, chat_id, respuesta_codificada);

            if (http_get(curl, &chunk_send, url_send) == 0) {
                /* Log OUT: usamos la misma fecha del mensaje (como Código 1) */
                log_event("OUT", msg_date, "BOT", respuesta);
                free(chunk_send.response);
            } else {
                printf("Error: no se pudo enviar la respuesta\n");
            }
        }

        /* Actualizamos offset: marcamos este mensaje como leído */
        offset_update_id = update_id + 1;

        if (chunk.response != NULL) {
            free(chunk.response);
            chunk.response = NULL;
        }

        sleep(2);
    }

    /* En la práctica no se llega aquí */
    curl_easy_cleanup(curl);
    return 0;
}
