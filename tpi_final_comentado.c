//cd "/c/Users/Usuario/Desktop/TP Integrador"
//gcc tpI_final.c -o tpI_final -lcurl

// tpI_final.c
// Repo: https://github.com/ValentinoVittore/Tp-UTN/blob/main/tpi_final_comentado-c
//
// Este programa implementa un bot de Telegram en C.
// La idea general es:
// 1) Leer el token del bot desde un archivo (argv[1]).
// 2) Armar la URL base de la API de Telegram con ese token.
// 3) Usar libcurl para hacer pedidos HTTP (getUpdates y sendMessage).
// 4) Quedar en un loop infinito leyendo mensajes nuevos y respondiendo
//    en función del texto ("hola", "chau").
// 5) Registrar en un archivo de log cada mensaje recibido (IN) y enviado (OUT).

#include <stdio.h>      // printf, fprintf, fopen, fclose, fgets, etc.
#include <stdlib.h>     // malloc, free, realloc, strtol, EXIT_FAILURE, etc.
#include <string.h>     // strlen, strcpy, strncpy, strstr, strchr, memcpy, strcmp, strpbrk, snprintf
#include <unistd.h>     // sleep (para pausar 2 segundos entre consultas)
#include <curl/curl.h>  // libcurl: funciones para hacer pedidos HTTP (curl_easy_*)
#include <ctype.h>      // tolower, para convertir caracteres a minúsculas

// Nombre del archivo donde se guarda el log de eventos del bot.
// Se guardan líneas con: dirección(IN/OUT);fecha_unix;nombre;texto
#define LOG_FILE "bot_log.txt"

// Esta estructura se usa para ir acumulando en memoria
// la respuesta completa de un pedido HTTP hecho con libcurl.
// - response: puntero dinámico a un buffer donde se guarda el texto (JSON).
// - size: cantidad de bytes útiles guardados en 'response'.
struct memory {
    char *response;
    size_t size;
};

/* Prototipos de funciones auxiliares */

// Callback que usa libcurl para escribir los datos recibidos en memoria.
static size_t cb(char *data, size_t size, size_t nmemb, void *clientp);

// Inicializa la estructura 'memory' dejándola en un estado "vacío".
void init_chunk(struct memory *chunk);

// Realiza un GET HTTP usando un CURL* ya inicializado, y llena 'chunk'.
int http_get(CURL *curl, struct memory *chunk, const char *url);

// Lee el token del bot desde un archivo de texto (primera línea).
int leer_token_desde_archivo(const char *filename, char *token, size_t token_sz);

// Busca un campo string dentro del JSON (por ejemplo "first_name": "Valentino")
// y lo copia en 'out'.
int parse_string_field(const char *json, const char *field_name, char *out, size_t maxlen);

// Busca un campo numérico entero dentro del JSON (por ejemplo "date": 1762883458).
int parse_long_field(const char *json, const char *field_name, long *value);

// Convierte un string a minúsculas (se usa para buscar "hola"/"chau" sin distinguir mayúsculas).
void to_lower_str(char *s);

// Reemplaza espacios por %20 para poder mandar textos en una URL (URL encoding mínimo).
void url_encode_spaces(const char *msg, char *out, size_t maxlen);

// Escribe en el archivo de log un evento de entrada o salida (IN/OUT).
int log_event(const char *direction, long unix_time, const char *name, const char *text);


/* Callback de libcurl: acumula respuesta en memoria
 *
 * Esta función es llamada automáticamente por libcurl cada vez que
 * llegan datos nuevos desde el servidor HTTP.
 * - data: puntero a los datos recién recibidos.
 * - size * nmemb: cantidad de bytes que llegaron.
 * - clientp: puntero genérico que pasamos a libcurl, en este caso
 *            lo usamos como (struct memory *) para ir guardando todo.
 */
static size_t cb(char *data, size_t size, size_t nmemb, void *clientp)
{
    size_t realsize = nmemb; /* igual que el código base: cantidad de bytes recibidos */
    struct memory *mem = (struct memory *)clientp; // casteo del puntero genérico a nuestro struct

    // Reservamos (o agrandamos) el buffer para acumular la respuesta completa.
    // realsize bytes nuevos + 1 para el '\0' final de string.
    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (!ptr) {
        // Si realloc falla, devolvemos 0 para que libcurl sepa que hubo error de memoria
        return 0; /* out of memory */
    }

    mem->response = ptr; // guardamos el nuevo puntero (ya agrandado)
    // Copiamos los nuevos datos al final del buffer existente.
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;                 // actualizamos el tamaño total
    mem->response[mem->size] = 0;          // agregamos '\0' para tratarlo como string C

    return realsize; // indicamos a libcurl que consumimos todos los bytes
}

// inicialización a cero de todos los campos
// struct memory chunk = {0};
// Esta función deja la estructura 'chunk' en un estado consistente para ser usada
// por http_get: sin memoria reservada y tamaño 0.
void init_chunk(struct memory *chunk)
{
    chunk->response = NULL; // aún no hay memoria asignada
    chunk->size = 0;        // tamaño del contenido es 0
}

/* GET HTTP usando CURL ya inicializado
 *
 * Esta función encapsula los pasos necesarios para realizar un pedido HTTP GET
 * con libcurl, usando un CURL* ya inicializado en el main.
 * - curl: manejador de libcurl (inicializado con curl_easy_init).
 * - chunk: estructura donde se acumulará la respuesta.
 * - url: URL completa del servicio (por ejemplo getUpdates o sendMessage).
 */
int http_get(CURL *curl, struct memory *chunk, const char *url)
{
    CURLcode res;

    // Validación básica de punteros nulos para evitar fallos.
    if (curl == NULL || chunk == NULL || url == NULL) {
        printf("Error: puntero nulo en http_get\n");
        return 1;
    }

    // Inicializamos el chunk antes de usarlo en este pedido.
    init_chunk(chunk);

    // Configuramos en libcurl la URL objetivo.
    curl_easy_setopt(curl, CURLOPT_URL, url);
    // Indicamos la función callback que libcurl llamará para escribir datos.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
    // Le pasamos un puntero a nuestra estructura 'chunk' como datos de usuario.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);

    // Ejecutamos la transferencia HTTP (realiza la conexión, envía el GET, recibe la respuesta).
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        // Si algo falla, mostramos el código de error.
        printf("Error: fallo la solicitud HTTP. Código: %d\n", res);
        // Si se llegó a reservar memoria para la respuesta, la liberamos.
        if (chunk->response != NULL) {
            free(chunk->response);
            chunk->response = NULL;
        }
        return 1;
    }

    // Si por alguna razón no se recibió nada, también lo consideramos error.
    if (chunk->response == NULL) {
        printf("Error: respuesta vacía desde el servidor\n");
        return 1;
    }

    return 0; // 0 = OK
}

/* Lee el token desde un archivo (primera línea)
 *
 * El token del bot NO debe estar hardcodeado en el código.
 * Esta función:
 * 1) Abre el archivo cuyo nombre está en 'filename'.
 * 2) Lee la primera línea (hasta salto de línea o fin de archivo).
 * 3) Elimina el '\n' final si existe.
 * 4) Verifica que no haya quedado vacío.
 */
int leer_token_desde_archivo(const char *filename, char *token, size_t token_sz)
{
    FILE *fPtr;
    char *nl;

    // Validación de punteros y tamaño.
    if (filename == NULL || token == NULL || token_sz == 0) {
        printf("Error: puntero nulo en leer_token_desde_archivo\n");
        return 1;
    }

    // Abrimos el archivo de token en modo lectura de texto.
    fPtr = fopen(filename, "r");
    if (fPtr == NULL) {
        printf("Error: no se pudo abrir el archivo de token '%s'\n", filename);
        return 1;
    }

    // Leemos una línea (hasta token_sz-1 caracteres como máximo).
    if (fgets(token, (int)token_sz, fPtr) == NULL) {
        printf("Error: no se pudo leer el token desde '%s'\n", filename);
        fclose(fPtr);
        return 1;
    }

    fclose(fPtr);

    // Buscamos el primer carácter de fin de línea (\r o \n) y lo reemplazamos por '\0'.
    nl = strpbrk(token, "\r\n");
    if (nl != NULL) {
        *nl = '\0';
    }

    // Si el token quedó vacío, también es un error.
    if (token[0] == '\0') {
        printf("Error: token vacío en archivo '%s'\n", filename);
        return 1;
    }

    return 0;
}

/* Parsea un campo string del JSON: "campo": "valor"
 *
 * Esta función busca dentro del texto JSON una ocurrencia de:
 *   "field_name": "algún texto"
 * y copia ese "algún texto" dentro de 'out'.
 *
 * json:      JSON completo como string.
 * field_name: nombre del campo a buscar (ej: "first_name", "text").
 * out:       buffer de salida donde se copiará el valor.
 * maxlen:    largo máximo de 'out'.
 */
int parse_string_field(const char *json, const char *field_name, char *out, size_t maxlen)
{
    char pattern[128];
    const char *p;
    size_t i = 0;

    // Validación básica de parámetros.
    if (json == NULL || field_name == NULL || out == NULL || maxlen == 0) {
        printf("Error: puntero nulo en parse_string_field\n");
        return 1;
    }

    /* Siempre dejamos algo coherente en out */
    out[0] = '\0';

    // Armamos el patrón de búsqueda, por ejemplo: "first_name"
    snprintf(pattern, sizeof(pattern), "\"%s\"", field_name);

    // Buscamos ese patrón dentro del JSON.
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

    // Desde donde encontramos "campo", buscamos el ':' que indica el valor.
    p = strchr(p, ':');
    if (!p) {
        printf("Error: formato inválido para campo '%s'\n", field_name);
        return 1;
    }
    p++;
    // Saltamos espacios o tabulaciones después de los dos puntos.
    while (*p == ' ' || *p == '\t') p++;

    // El valor para un string debe comenzar con comillas.
    if (*p != '\"') {
        printf("Error: campo '%s' no es string\n", field_name);
        return 1;
    }
    p++; /* saltar la comilla inicial */

    // Copiamos carácter por carácter hasta encontrar la comilla de cierre
    // o hasta completar el tamaño máximo del buffer.
    while (*p && *p != '\"' && i < maxlen - 1) {
        // Si encontramos un backslash (\), saltamos el carácter especial siguiente
        // (manejo mínimo de escapes).
        if (*p == '\\' && p[1] != '\0') {
            p++; /* saltear el backslash */
        }
        out[i++] = *p++;
    }
    out[i] = '\0';

    // Si el campo existe pero está vacío, lo advertimos (no es error fatal).
    if (i == 0) {
        printf("Advertencia: campo string '%s' vacío\n", field_name);
    }

    return 0;
}


/* Parsea un campo numérico entero: "campo": 12345
 *
 * Similar a parse_string_field, pero busca un valor numérico entero
 * y lo convierte con strtol.
 */
int parse_long_field(const char *json, const char *field_name, long *value)
{
    char pattern[128];
    const char *p;

    // Validación básica de parámetros.
    if (json == NULL || field_name == NULL || value == NULL) {
        printf("Error: puntero nulo en parse_long_field\n");
        return 1;
    }

    // Construimos el patrón "campo"
    snprintf(pattern, sizeof(pattern), "\"%s\"", field_name);

    // Buscamos dentro del JSON
    p = strstr(json, pattern);
    if (!p) {
        printf("Error: no se encontró campo '%s' en JSON\n", field_name);
        return 1;
    }

    // Buscamos el ':' que separa el nombre del valor.
    p = strchr(p, ':');
    if (!p) {
        printf("Error: formato inválido para campo '%s'\n", field_name);
        return 1;
    }
    p++;
    // Saltamos espacios o tabs.
    while (*p == ' ' || *p == '\t') p++;

    // Convertimos desde 'p' hasta que strtol encuentre algo no numérico.
    *value = strtol(p, NULL, 10);
    return 0;
}

/* Convierte string a minúsculas in-place
 *
 * Recorre el string y aplica tolower a cada carácter.
 * Se usa para que las comparaciones ("hola", "chau") sean
 * insensibles a mayúsculas/minúsculas.
 */
void to_lower_str(char *s)
{
    while (*s) {
        *s = (char)tolower((unsigned char)*s);
        s++;
    }
}

/* Codifica espacios como %20 (suficiente para el TP; trunca si no entra)
 *
 * Esta función recorre el mensaje 'msg' y copia en 'out':
 * - caracteres normales tal cual.
 * - espacios reemplazados por la secuencia '%20'.
 * Se usa para armar la parte 'text=' de la URL del sendMessage.
 */
void url_encode_spaces(const char *msg, char *out, size_t maxlen)
{
    size_t j = 0;
    size_t i;

    // Validación básica de punteros y tamaño de salida.
    if (msg == NULL || out == NULL || maxlen == 0) {
        return;
    }

    // Recorremos el mensaje original carácter por carácter.
    for (i = 0; msg[i] != '\0' && j < maxlen - 1; i++) {
        if (msg[i] == ' ') {
            // Si encontramos un espacio, intentamos escribir "%20".
            if (j + 3 >= maxlen) {
                break; /* no hay lugar, se trunca */
            }
            out[j++] = '%';
            out[j++] = '2';
            out[j++] = '0';
        } else {
            // Para cualquier otro carácter, copiamos tal cual.
            out[j++] = msg[i];
        }
    }
    // Terminamos el string resultante en '\0'.
    out[j] = '\0';
}

/* Loguea mensajes IN/OUT en archivo de texto
 *
 * direction: "IN" para mensajes recibidos, "OUT" para mensajes enviados.
 * unix_time: marca de tiempo (segundos desde 1970).
 * name:      nombre de usuario o "BOT".
 * text:      contenido del mensaje.
 *
 * El archivo LOG_FILE se abre en modo 'append' ("a"), de modo que
 * los nuevos eventos se agregan al final sin borrar lo previo.
 */
int log_event(const char *direction, long unix_time, const char *name, const char *text)
{
    FILE *f;

    // Si alguno de los strings viene como NULL, los reemplazamos por "-"
    // para evitar problemas al imprimir.
    if (direction == NULL) direction = "-";
    if (name == NULL) name = "-";
    if (text == NULL) text = "-";

    // Abrimos el archivo de log en modo append.
    f = fopen(LOG_FILE, "a");
    if (f == NULL) {
        printf("Error: no se pudo abrir '%s' para log\n", LOG_FILE);
        return 1;
    }

    // Escribimos una línea en formato: direction;unix_time;name;text
    fprintf(f, "%s;%ld;%s;%s\n", direction, unix_time, name, text);
    fclose(f);

    return 0;
}

int main(int argc, char *argv[])
{
    char token[256];           // buffer para guardar el token leído desde archivo
    char base_url[512];        // URL base de la API: "https://api.telegram.org/bot<TOKEN>"
    CURL *curl;                // manejador principal de libcurl
    struct memory chunk;       // estructura para guardar respuestas de getUpdates
    long offset_update_id = -1; /* -1: sin offset aún; luego será último update_id + 1 */

    // Verificamos que se haya pasado exactamente un argumento: el archivo de token.
    if (argc != 2) {
        printf("Uso: %s token.txt\n", argv[0]);
        return 1;
    }

    // Leemos el token desde el archivo indicado en argv[1].
    if (leer_token_desde_archivo(argv[1], token, sizeof(token)) != 0) {
        // Si falla, terminamos el programa con error.
        return 1;
    }

    // Armamos la URL base con el token:
    // base_url = "https://api.telegram.org/bot" + token
    snprintf(base_url, sizeof(base_url), "https://api.telegram.org/bot%s", token);

    // Inicializamos libcurl. Si esto falla, no podemos seguir.
    curl = curl_easy_init();
    if (!curl) {
        printf("Error: no se pudo inicializar CURL\n");
        return 1;
    }

    printf("Bot iniciado. Esperando mensajes...\n");

    // Loop infinito: el bot queda corriendo permanentemente.
    while (1) {
        char url_get[1024];  // buffer para construir la URL de getUpdates

        // Si ya tenemos un offset_update_id válido (>= 0),
        // lo agregamos como parámetro offset para no repetir mensajes.
        if (offset_update_id >= 0) {
            snprintf(url_get, sizeof(url_get),
                     "%s/getUpdates?offset=%ld", base_url, offset_update_id);
        } else {
            // Primer pedido: sin offset, para obtener los mensajes pendientes.
            snprintf(url_get, sizeof(url_get),
                     "%s/getUpdates", base_url);
        }

        // Hacemos el pedido HTTP GET a la URL construida.
        if (http_get(curl, &chunk, url_get) != 0) {
            printf("Fallo en getUpdates\n");
            // Si falló, esperamos 2 segundos y volvemos a intentar.
            sleep(2);
            continue;
        }

        // Verificamos si la API devolvió "ok": false (error general).
        if (strstr(chunk.response, "\"ok\":false") != NULL) {
            printf("Error: la API de Telegram devolvió fallo:\n%s\n", chunk.response);
            free(chunk.response);
            chunk.response = NULL;
            sleep(2);
            continue;
        }

        // Verificamos si el campo "result" está vacío: no hay mensajes nuevos.
        if (strstr(chunk.response, "\"result\": []") != NULL ||
            strstr(chunk.response, "\"result\":[]") != NULL) {
            // Liberamos la memoria de la respuesta y esperamos 2 segundos.
            free(chunk.response);
            chunk.response = NULL;
            sleep(2);
            continue;
        }

        /* Procesamos el primer mensaje de result (estilo Código 1)
         *
         * El JSON de getUpdates contiene un array "result" con uno o más
         * objetos de actualización. Aquí, para simplificar, procesamos solo
         * el primero.
         */

        /* update_id */
        long update_id = 0;
        // Extraemos el campo numérico "update_id" (primer aparición en el JSON).
        if (parse_long_field(chunk.response, "update_id", &update_id) != 0) {
            free(chunk.response);
            chunk.response = NULL;
            sleep(2);
            continue;
        }

        /* Sección "message" */
        // Buscamos la sección "message" dentro del JSON, que contiene
        // los datos principales: chat, text, date, etc.
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

        long chat_id = 0;        // ID del chat para poder responder
        long msg_date = 0;       // fecha del mensaje en tiempo Unix
        char first_name[128] = ""; // nombre del usuario que envió el mensaje
        char text[512] = "";       // texto del mensaje

        /* Dentro de message, buscamos chat */
        char *p_chat = strstr(p_message, "\"chat\"");
        if (p_chat) {
            /* chat.id */
            // Extraemos el ID de chat (puede ser un chat privado, grupo, etc.)
            if (parse_long_field(p_chat, "id", &chat_id) != 0) {
                chat_id = 0;
            }
            /* chat.first_name */
            // Extraemos el nombre del usuario (first_name).
            if (parse_string_field(p_chat, "first_name", first_name, sizeof(first_name)) != 0) {
                first_name[0] = '\0';
            }
        }

        /* message.date */
        // Obtenemos la marca de tiempo del mensaje (segundos desde 1970).
        if (parse_long_field(p_message, "date", &msg_date) != 0) {
            msg_date = 0;
        }

        /* message.text */
        // Extraemos el texto del mensaje. Si es un mensaje sin texto
        // (ej. foto, audio), text puede quedar vacío.
        if (parse_string_field(p_message, "text", text, sizeof(text)) != 0) {
            text[0] = '\0';
        }

        /* Log IN: mensaje recibido
         *
         * Registramos en archivo que se recibió un mensaje (IN),
         * con la fecha, el nombre del usuario y el texto.
         */
        log_event("IN", msg_date, first_name, text);

        /* Decidir respuesta (hola/chau) */
        char respuesta[512] = "";
        if (text[0] != '\0') {
            char texto_lower[512];
            // Copiamos el texto en otro buffer para pasarlo a minúsculas
            // (así no alteramos 'text' original).
            strncpy(texto_lower, text, sizeof(texto_lower) - 1);
            texto_lower[sizeof(texto_lower) - 1] = '\0';
            to_lower_str(texto_lower);

            // Si el texto contiene "hola" (en cualquier parte, cualquier combinación
            // de mayúsculas/minúsculas), respondemos con un saludo personalizado.
            if (strstr(texto_lower, "hola") != NULL) {
                snprintf(respuesta, sizeof(respuesta),
                         "Hola, %s", first_name[0] ? first_name : "usuario");
            } else if (strstr(texto_lower, "chau") != NULL) {
                // Si contiene "chau", respondemos despidiéndonos.
                snprintf(respuesta, sizeof(respuesta),
                         "Chau %s, que tengas buen día!",
                         first_name[0] ? first_name : "");
            }
        }

        // Solo intentamos responder si tenemos algo en 'respuesta' y un chat_id válido.
        if (respuesta[0] != '\0' && chat_id != 0) {
            char respuesta_codificada[1024]; // texto con espacios reemplazados por %20
            char url_send[2048];             // URL completa para sendMessage
            struct memory chunk_send;        // estructura para la respuesta de sendMessage

            // Codificamos los espacios del mensaje de respuesta.
            url_encode_spaces(respuesta, respuesta_codificada,
                              sizeof(respuesta_codificada));

            // Construimos la URL de sendMessage con chat_id y texto.
            snprintf(url_send, sizeof(url_send),
                     "%s/sendMessage?chat_id=%ld&text=%s",
                     base_url, chat_id, respuesta_codificada);

            // Realizamos el pedido HTTP GET para enviar el mensaje.
            if (http_get(curl, &chunk_send, url_send) == 0) {
                /* Log OUT: usamos la misma fecha del mensaje (como Código 1)
                 *
                 * Registramos en el log que enviamos una respuesta (OUT),
                 * indicando el mismo tiempo del mensaje original.
                 */
                log_event("OUT", msg_date, "BOT", respuesta);
                free(chunk_send.response);
            } else {
                printf("Error: no se pudo enviar la respuesta\n");
            }
        }

        /* Actualizamos offset: marcamos este mensaje como leído
         *
         * Para que en el próximo getUpdates no se repita el mismo
         * mensaje, avanzamos el offset al siguiente update_id.
         */
        offset_update_id = update_id + 1;

        // Liberamos el buffer de respuesta usado en getUpdates.
        if (chunk.response != NULL) {
            free(chunk.response);
            chunk.response = NULL;
        }

        // Esperamos 2 segundos antes de hacer un nuevo getUpdates
        // (polling cada 2 segundos).
        sleep(2);
    }

    /* En la práctica no se llega aquí porque el while(1) no termina,
     * pero por prolijidad se incluye la limpieza de libcurl.
     */
    curl_easy_cleanup(curl);
    return 0;
}
