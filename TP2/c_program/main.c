#define _POSIX_C_SOURCE 200112L

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define API_HOST "api.worldbank.org"
#define API_PORT "443"
#define API_PATH_TEMPLATE "/v2/en/country/all/indicator/SI.POV.GINI?format=json&date=%d:%d&page=1&per_page=32500"
#define MAX_POINTS 64
#define REQUEST_BUFFER_SIZE 512
#define PATH_BUFFER_SIZE 256
#define COUNTRY_BUFFER_SIZE 128
#define RESPONSE_CHUNK_SIZE 4096
#define INITIAL_RESPONSE_CAPACITY 65536

typedef struct {
    int year;
    float original_value;
    int converted_value;
    int adjusted_value;
} IndicatorPoint;

typedef struct {
    int socket_fd;
    SSL_CTX *ssl_ctx;
    SSL *ssl;
} TlsConnection;

static void print_usage(const char *program_name) {
    fprintf(
        stderr,
        "Uso: %s [pais] [anio_inicio] [anio_fin]\n"
        "Ejemplo: %s Argentina 2011 2020\n",
        program_name,
        program_name
    );
}

static int connect_tcp_socket(const char *host, const char *port) {
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct addrinfo *entry = NULL;
    int socket_fd = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &result) != 0) {
        return -1;
    }

    for (entry = result; entry != NULL; entry = entry->ai_next) {
        socket_fd = socket(entry->ai_family, entry->ai_socktype, entry->ai_protocol);
        if (socket_fd == -1) {
            continue;
        }

        if (connect(socket_fd, entry->ai_addr, entry->ai_addrlen) == 0) {
            break;
        }

        close(socket_fd);
        socket_fd = -1;
    }

    freeaddrinfo(result);
    return socket_fd;
}

static void cleanup_tls_connection(TlsConnection *connection) {
    if (connection->ssl != NULL) {
        SSL_shutdown(connection->ssl);
        SSL_free(connection->ssl);
        connection->ssl = NULL;
    }
    if (connection->ssl_ctx != NULL) {
        SSL_CTX_free(connection->ssl_ctx);
        connection->ssl_ctx = NULL;
    }
    if (connection->socket_fd >= 0) {
        close(connection->socket_fd);
        connection->socket_fd = -1;
    }
}

static int open_tls_connection(TlsConnection *connection) {
    connection->socket_fd = -1;
    connection->ssl_ctx = NULL;
    connection->ssl = NULL;

    SSL_load_error_strings();
    OPENSSL_init_ssl(0, NULL);

    connection->socket_fd = connect_tcp_socket(API_HOST, API_PORT);
    if (connection->socket_fd == -1) {
        perror("No se pudo abrir el socket TCP");
        return -1;
    }

    connection->ssl_ctx = SSL_CTX_new(TLS_client_method());
    if (connection->ssl_ctx == NULL) {
        fprintf(stderr, "No se pudo crear el contexto TLS\n");
        cleanup_tls_connection(connection);
        return -1;
    }

    SSL_CTX_set_default_verify_paths(connection->ssl_ctx);
    SSL_CTX_set_verify(connection->ssl_ctx, SSL_VERIFY_PEER, NULL);

    connection->ssl = SSL_new(connection->ssl_ctx);
    if (connection->ssl == NULL) {
        fprintf(stderr, "No se pudo crear el objeto SSL\n");
        cleanup_tls_connection(connection);
        return -1;
    }

    SSL_set_fd(connection->ssl, connection->socket_fd);
    SSL_set_tlsext_host_name(connection->ssl, API_HOST);

    if (SSL_connect(connection->ssl) <= 0) {
        fprintf(stderr, "No se pudo establecer la conexion TLS\n");
        ERR_print_errors_fp(stderr);
        cleanup_tls_connection(connection);
        return -1;
    }

    if (SSL_get_verify_result(connection->ssl) != X509_V_OK) {
        fprintf(stderr, "La verificacion del certificado TLS fallo\n");
        cleanup_tls_connection(connection);
        return -1;
    }

    return 0;
}

static int build_world_bank_request_path(
    int start_year,
    int end_year,
    char *request_path,
    size_t request_path_size
) {
    int written = snprintf(
        request_path,
        request_path_size,
        API_PATH_TEMPLATE,
        start_year,
        end_year
    );

    if (written < 0 || (size_t) written >= request_path_size) {
        fprintf(stderr, "No se pudo construir la ruta HTTP\n");
        return -1;
    }

    return 0;
}

static int fetch_https_response(
    const char *request_path,
    char **response_buffer,
    size_t *response_size
) {
    TlsConnection connection;
    char request_buffer[REQUEST_BUFFER_SIZE];
    char chunk_buffer[RESPONSE_CHUNK_SIZE];
    char *buffer = NULL;
    size_t capacity = INITIAL_RESPONSE_CAPACITY;
    size_t used = 0;

    *response_buffer = NULL;
    *response_size = 0;

    snprintf(
        request_buffer,
        sizeof(request_buffer),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: gini-client-c/1.0\r\n"
        "Accept: application/json\r\n"
        "Connection: close\r\n"
        "\r\n",
        request_path,
        API_HOST
    );

    if (open_tls_connection(&connection) != 0) {
        return -1;
    }

    buffer = malloc(capacity);
    if (buffer == NULL) {
        fprintf(stderr, "No se pudo reservar memoria para la respuesta HTTP\n");
        cleanup_tls_connection(&connection);
        return -1;
    }

    if (SSL_write(connection.ssl, request_buffer, (int) strlen(request_buffer)) <= 0) {
        fprintf(stderr, "No se pudo enviar la peticion HTTPS\n");
        ERR_print_errors_fp(stderr);
        free(buffer);
        cleanup_tls_connection(&connection);
        return -1;
    }

    for (;;) {
        int received = SSL_read(connection.ssl, chunk_buffer, sizeof(chunk_buffer));
        if (received == 0) {
            break;
        }
        if (received < 0) {
            fprintf(stderr, "No se pudo leer la respuesta HTTPS\n");
            ERR_print_errors_fp(stderr);
            free(buffer);
            cleanup_tls_connection(&connection);
            return -1;
        }

        if (used + (size_t) received + 1 > capacity) {
            size_t new_capacity = capacity * 2;
            while (used + (size_t) received + 1 > new_capacity) {
                new_capacity *= 2;
            }

            char *grown_buffer = realloc(buffer, new_capacity);
            if (grown_buffer == NULL) {
                fprintf(stderr, "No se pudo ampliar el buffer de respuesta\n");
                free(buffer);
                cleanup_tls_connection(&connection);
                return -1;
            }

            buffer = grown_buffer;
            capacity = new_capacity;
        }

        memcpy(buffer + used, chunk_buffer, (size_t) received);
        used += (size_t) received;
    }

    buffer[used] = '\0';
    cleanup_tls_connection(&connection);

    *response_buffer = buffer;
    *response_size = used;
    return 0;
}

static int extract_http_body(char *http_response, char **body_start) {
    char *separator = strstr(http_response, "\r\n\r\n");

    if (separator == NULL) {
        fprintf(stderr, "Respuesta HTTP invalida\n");
        return -1;
    }

    if (strncmp(http_response, "HTTP/1.1 200", 12) != 0 && strncmp(http_response, "HTTP/1.0 200", 12) != 0) {
        fprintf(stderr, "La API devolvio un estado no exitoso: %.32s\n", http_response);
        return -1;
    }

    *body_start = separator + 4;
    return 0;
}

static int parse_world_bank_json(
    const char *json_body,
    const char *country_name,
    IndicatorPoint *points,
    int max_points,
    char *resolved_country,
    size_t resolved_country_size
) {
    const char *cursor = json_body;
    int count = 0;

    if (resolved_country_size > 0) {
        resolved_country[0] = '\0';
    }

    while ((cursor = strstr(cursor, "\"country\":{\"id\":\"")) != NULL) {
        const char *country_id_end = NULL;
        const char *country_value = NULL;
        const char *date_field = NULL;
        const char *numeric_value = NULL;
        const char *decimal_field = NULL;
        char country_buffer[COUNTRY_BUFFER_SIZE];
        char *country_end = NULL;
        char *date_end = NULL;
        char *value_end = NULL;
        long parsed_year = 0;
        float parsed_value = 0.0f;

        country_id_end = strchr(cursor + strlen("\"country\":{\"id\":\""), '"');
        if (country_id_end == NULL) {
            break;
        }

        country_value = strstr(country_id_end, "\"value\":\"");
        if (country_value == NULL) {
            break;
        }
        country_value += strlen("\"value\":\"");
        country_end = strchr(country_value, '"');
        if (country_end == NULL) {
            break;
        }

        date_field = strstr(country_end, "\"date\":\"");
        if (date_field == NULL) {
            break;
        }
        date_field += strlen("\"date\":\"");

        numeric_value = strstr(date_field, "\"value\":");
        if (numeric_value == NULL) {
            break;
        }
        numeric_value += strlen("\"value\":");

        decimal_field = strstr(numeric_value, "\"decimal\":");
        if (decimal_field == NULL) {
            break;
        }

        size_t country_length = (size_t) (country_end - country_value);
        if (country_length >= sizeof(country_buffer)) {
            country_length = sizeof(country_buffer) - 1;
        }
        memcpy(country_buffer, country_value, country_length);
        country_buffer[country_length] = '\0';

        cursor = decimal_field;

        if (strcasecmp(country_buffer, country_name) != 0) {
            continue;
        }

        errno = 0;
        parsed_year = strtol(date_field, &date_end, 10);
        if (errno != 0 || date_end == date_field) {
            return -1;
        }

        if (strncmp(numeric_value, "null", 4) == 0) {
            continue;
        }

        errno = 0;
        parsed_value = strtof(numeric_value, &value_end);
        if (errno != 0 || value_end == numeric_value) {
            return -1;
        }

        if (count >= max_points) {
            fprintf(stderr, "Se alcanzo el limite de puntos soportados\n");
            return -1;
        }

        points[count].year = (int) parsed_year;
        points[count].original_value = parsed_value;
        points[count].converted_value = 0;
        points[count].adjusted_value = 0;
        ++count;

        if (resolved_country[0] == '\0') {
            snprintf(resolved_country, resolved_country_size, "%s", country_buffer);
        }
    }

    return count;
}

static void sort_points_by_year(IndicatorPoint *points, int count) {
    for (int i = 0; i < count - 1; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (points[j].year < points[i].year) {
                IndicatorPoint temp = points[i];
                points[i] = points[j];
                points[j] = temp;
            }
        }
    }
}

static void convert_points_to_int(IndicatorPoint *points, int count) {
    int reference_value = 0;
    for (int index = 0; index < count; ++index) {
        points[index].converted_value = (int) points[index].original_value;
        reference_value = points[index].converted_value + 0.5;

        if (points[index].original_value - reference_value >= 0.5f) {
            points[index].converted_value += 1;
        }
    }
}

static void increment_points(IndicatorPoint *points, int count, int delta) {
    for (int index = 0; index < count; ++index) {
        points[index].adjusted_value = points[index].converted_value + delta;
    }
}

static void print_table(const IndicatorPoint *points, int count) {
    printf("Anio | Valor API | Entero | Ajustado\n");
    printf("-----+-----------+--------+---------\n");
    for (int index = 0; index < count; ++index) {
        printf(
            "%4d | %9.2f | %6d | %7d\n",
            points[index].year,
            points[index].original_value,
            points[index].converted_value,
            points[index].adjusted_value
        );
    }
}

static void plot_points_console(const IndicatorPoint *points, int count) {
    printf("\nGrafico ASCII (valor ajustado)\n");
    for (int index = 0; index < count; ++index) {
        int bar_length = points[index].adjusted_value;
        if (bar_length < 0) {
            bar_length = 0;
        }
        if (bar_length > 60) {
            bar_length = 60;
        }

        printf("%4d | ", points[index].year);
        for (int cursor = 0; cursor < bar_length; ++cursor) {
            putchar('#');
        }
        printf(" (%d)\n", points[index].adjusted_value);
    }
}

int main(int argc, char **argv) {
    const char *country = "Argentina";
    int start_year = 2011;
    int end_year = 2020;
    char request_path[PATH_BUFFER_SIZE];
    char resolved_country[COUNTRY_BUFFER_SIZE];
    char *http_response = NULL;
    char *response_body = NULL;
    size_t response_size = 0;
    IndicatorPoint points[MAX_POINTS];
    int point_count = 0;

    if (argc >= 2) {
        country = argv[1];
    }
    if (argc >= 3) {
        start_year = atoi(argv[2]);
    }
    if (argc >= 4) {
        end_year = atoi(argv[3]);
    }
    if (argc > 4) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (start_year > end_year) {
        fprintf(stderr, "El anio inicial no puede ser mayor al anio final\n");
        return EXIT_FAILURE;
    }

    memset(points, 0, sizeof(points));
    memset(request_path, 0, sizeof(request_path));
    memset(resolved_country, 0, sizeof(resolved_country));

    if (build_world_bank_request_path(start_year, end_year, request_path, sizeof(request_path)) != 0) {
        return EXIT_FAILURE;
    }

    if (fetch_https_response(request_path, &http_response, &response_size) != 0) {
        return EXIT_FAILURE;
    }

    if (extract_http_body(http_response, &response_body) != 0) {
        free(http_response);
        return EXIT_FAILURE;
    }

    point_count = parse_world_bank_json(
        response_body,
        country,
        points,
        MAX_POINTS,
        resolved_country,
        sizeof(resolved_country)
    );
    if (point_count < 0) {
        fprintf(stderr, "No se pudo parsear el JSON recibido\n");
        free(http_response);
        return EXIT_FAILURE;
    }
    if (point_count == 0) {
        fprintf(stderr, "La API no devolvio datos para el pais solicitado\n");
        free(http_response);
        return EXIT_FAILURE;
    }

    sort_points_by_year(points, point_count);
    convert_points_to_int(points, point_count);
    increment_points(points, point_count, 1);

    printf("Pais: %s\n", resolved_country[0] != '\0' ? resolved_country : country);
    printf("Cantidad de puntos: %d\n", point_count);
    printf("Bytes HTTP recibidos: %zu\n\n", response_size);
    print_table(points, point_count);
    plot_points_console(points, point_count);

    free(http_response);
    return EXIT_SUCCESS;
}
