#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t keep_running = 1;

typedef enum {
    SOURCE_NONE = 0,
    SOURCE_DEVICE,
    SOURCE_MEM
} source_type_t;

typedef enum {
    DEVICE_TEXT = 0,
    DEVICE_BINARY_U32
} device_format_t;

typedef struct {
    source_type_t source;
    const char *device_path;
    const char *mem_device;
    uint64_t mem_address;
    size_t offset_a;
    size_t offset_b;
    unsigned width_bits;
    unsigned interval_ms;
    uint64_t max_samples;
    int pin_a;
    int pin_b;
    device_format_t device_format;
} config_t;

static void handle_signal(int signo) {
    (void)signo;
    keep_running = 0;
}

static uint64_t now_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return 0;
    }

    return ((uint64_t)ts.tv_sec * 1000ULL) + ((uint64_t)ts.tv_nsec / 1000000ULL);
}

static void sleep_ms(unsigned ms) {
    if (ms == 0) {
        return;
    }

    struct timespec req;
    req.tv_sec = (time_t)(ms / 1000U);
    req.tv_nsec = (long)(ms % 1000U) * 1000000L;

    while (keep_running && nanosleep(&req, &req) == -1 && errno == EINTR) {
    }
}

static bool parse_u64(const char *text, uint64_t *out) {
    char *end = NULL;
    errno = 0;
    unsigned long long value = strtoull(text, &end, 0);

    if (errno != 0 || end == text || *end != '\0') {
        return false;
    }

    *out = (uint64_t)value;
    return true;
}

static bool parse_size(const char *text, size_t *out) {
    uint64_t value = 0;
    if (!parse_u64(text, &value)) {
        return false;
    }

    *out = (size_t)value;
    return (uint64_t)(*out) == value;
}

static bool parse_uint(const char *text, unsigned *out) {
    uint64_t value = 0;
    if (!parse_u64(text, &value) || value > UINT32_MAX) {
        return false;
    }

    *out = (unsigned)value;
    return true;
}

static bool parse_int_arg(const char *text, int *out) {
    char *end = NULL;
    errno = 0;
    long value = strtol(text, &end, 0);

    if (errno != 0 || end == text || *end != '\0' || value < -1 || value > INT32_MAX) {
        return false;
    }

    *out = (int)value;
    return true;
}

static void print_usage(const char *program) {
    fprintf(stderr,
            "Usage:\n"
            "  %s --device PATH [options]\n"
            "  %s --mem-address ADDR [options]\n"
            "\n"
            "Sources:\n"
            "  --device PATH              Read samples from a char/proc/sysfs device.\n"
            "  --device-format FORMAT     text or binary-u32. Default: text.\n"
            "  --mem-address ADDR         Physical base address to map through /dev/mem.\n"
            "  --mem-device PATH          Memory device to use. Default: /dev/mem.\n"
            "\n"
            "Memory layout options:\n"
            "  --offset-a BYTES           Offset for value_a from ADDR. Default: 0.\n"
            "  --offset-b BYTES           Offset for value_b from ADDR. Default: 4.\n"
            "  --width BITS               Register width: 8, 16 or 32. Default: 32.\n"
            "\n"
            "Runtime options:\n"
            "  --interval-ms MS           Delay between samples. Default: 500.\n"
            "  --max-samples N            Stop after N samples. Default: 0 (infinite).\n"
            "  --pin-a N                  GPIO label for value_a. Default: null.\n"
            "  --pin-b N                  GPIO label for value_b. Default: null.\n"
            "  --help                     Show this help.\n"
            "\n"
            "Output: one JSON object per line on stdout.\n",
            program,
            program);
}

static void print_json_sample(uint64_t seq,
                              uint64_t timestamp_ms,
                              const char *source,
                              int64_t value_a,
                              int64_t value_b,
                              int pin_a,
                              int pin_b) {
    const int gpio_a_value = value_a != 0 ? 1 : 0;
    const int gpio_b_value = value_b != 0 ? 1 : 0;
    const int binary_value = (gpio_a_value << 1) | gpio_b_value;
    const double normalized_value = (double)binary_value / 3.0;

    printf("{\"timestamp_ms\":%" PRIu64
           ",\"seq\":%" PRIu64
           ",\"source\":\"%s\""
           ",\"value_a\":%" PRId64
           ",\"value_b\":%" PRId64
           ",\"gpio_a_value\":%d"
           ",\"gpio_b_value\":%d"
           ",\"binary_code\":\"%d%d\""
           ",\"binary_value\":%d"
           ",\"normalized_value\":%.6f"
           ",\"pin_a\":",
           timestamp_ms,
           seq,
           source,
           value_a,
           value_b,
           gpio_a_value,
           gpio_b_value,
           gpio_a_value,
           gpio_b_value,
           binary_value,
           normalized_value);

    if (pin_a >= 0) {
        printf("%d", pin_a);
    } else {
        printf("null");
    }

    printf(",\"pin_b\":");

    if (pin_b >= 0) {
        printf("%d", pin_b);
    } else {
        printf("null");
    }

    printf("}\n");
    fflush(stdout);
}

static bool parse_two_text_values(const char *line, int64_t *value_a, int64_t *value_b) {
    const char *cursor = line;
    char *end = NULL;
    int64_t values[2] = {0, 0};
    size_t found = 0;

    while (*cursor != '\0' && found < 2) {
        while (*cursor != '\0' &&
               !((*cursor >= '0' && *cursor <= '9') || *cursor == '-' || *cursor == '+')) {
            cursor++;
        }

        if (*cursor == '\0') {
            break;
        }

        errno = 0;
        long long parsed = strtoll(cursor, &end, 0);
        if (errno == 0 && end != cursor) {
            values[found++] = (int64_t)parsed;
            cursor = end;
        } else {
            cursor++;
        }
    }

    if (found < 2) {
        return false;
    }

    *value_a = values[0];
    *value_b = values[1];
    return true;
}

static bool parse_value_after_key(const char *text, const char *key, int *out) {
    const char *position = strstr(text, key);
    if (position == NULL) {
        return false;
    }

    position += strlen(key);

    char *end = NULL;
    errno = 0;
    long value = strtol(position, &end, 0);
    if (errno != 0 || end == position || value < INT32_MIN || value > INT32_MAX) {
        return false;
    }

    *out = (int)value;
    return true;
}

static bool parse_gpio_snapshot(const char *content,
                                int *value_a,
                                int *value_b,
                                int *detected_pin_a,
                                int *detected_pin_b) {
    bool found_a = false;
    bool found_b = false;
    bool saw_gpio_label = false;
    const char *line_start = content;

    while (*line_start != '\0') {
        const char *line_end = strchr(line_start, '\n');
        size_t length = line_end == NULL ? strlen(line_start) : (size_t)(line_end - line_start);

        char line[256];
        if (length >= sizeof(line)) {
            length = sizeof(line) - 1U;
        }
        memcpy(line, line_start, length);
        line[length] = '\0';

        if (strstr(line, "gpio_a") != NULL) {
            int parsed_value = 0;
            int parsed_pin = -1;
            saw_gpio_label = true;

            if (parse_value_after_key(line, "value=", &parsed_value)) {
                *value_a = parsed_value != 0 ? 1 : 0;
                found_a = true;
            }
            if (parse_value_after_key(line, "gpio_a=", &parsed_pin)) {
                *detected_pin_a = parsed_pin;
            }
        }

        if (strstr(line, "gpio_b") != NULL) {
            int parsed_value = 0;
            int parsed_pin = -1;
            saw_gpio_label = true;

            if (parse_value_after_key(line, "value=", &parsed_value)) {
                *value_b = parsed_value != 0 ? 1 : 0;
                found_b = true;
            }
            if (parse_value_after_key(line, "gpio_b=", &parsed_pin)) {
                *detected_pin_b = parsed_pin;
            }
        }

        if (line_end == NULL) {
            break;
        }
        line_start = line_end + 1;
    }

    if (found_a && found_b) {
        return true;
    }

    if (!saw_gpio_label) {
        int64_t fallback_a = 0;
        int64_t fallback_b = 0;
        if (parse_two_text_values(content, &fallback_a, &fallback_b)) {
            *value_a = fallback_a != 0 ? 1 : 0;
            *value_b = fallback_b != 0 ? 1 : 0;
            return true;
        }
    }

    return false;
}

static int read_device_text_snapshot(const char *path, char *buffer, size_t buffer_size) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    size_t used = 0;
    while (used + 1U < buffer_size) {
        ssize_t count = read(fd, buffer + used, buffer_size - used - 1U);
        if (count > 0) {
            used += (size_t)count;
            continue;
        }

        if (count == 0) {
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }

    buffer[used] = '\0';
    close(fd);
    return used > 0 ? 1 : 0;
}

static int read_exact(int fd, void *buffer, size_t length) {
    uint8_t *cursor = (uint8_t *)buffer;
    size_t remaining = length;

    while (remaining > 0) {
        ssize_t count = read(fd, cursor, remaining);
        if (count > 0) {
            cursor += count;
            remaining -= (size_t)count;
            continue;
        }

        if (count == 0) {
            return 0;
        }

        if (errno == EINTR) {
            continue;
        }

        return -1;
    }

    return 1;
}

static int run_device_binary_u32(const config_t *config) {
    int fd = open(config->device_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "error: cannot open %s: %s\n", config->device_path, strerror(errno));
        return 1;
    }

    uint64_t seq = 0;
    while (keep_running && (config->max_samples == 0 || seq < config->max_samples)) {
        uint32_t values[2] = {0, 0};
        int status = read_exact(fd, values, sizeof(values));

        if (status < 0) {
            fprintf(stderr, "error: cannot read %s: %s\n", config->device_path, strerror(errno));
            close(fd);
            return 1;
        }

        if (status == 0) {
            sleep_ms(config->interval_ms);
            continue;
        }

        print_json_sample(++seq,
                          now_ms(),
                          "device",
                          (int64_t)values[0],
                          (int64_t)values[1],
                          config->pin_a,
                          config->pin_b);
    }

    close(fd);
    return 0;
}

static int run_device_text(const config_t *config) {
    uint64_t seq = 0;

    while (keep_running && (config->max_samples == 0 || seq < config->max_samples)) {
        char buffer[1024];
        int read_status = read_device_text_snapshot(config->device_path, buffer, sizeof(buffer));
        if (read_status < 0) {
            fprintf(stderr, "error: cannot read %s: %s\n", config->device_path, strerror(errno));
            return 1;
        }

        if (read_status == 0) {
            sleep_ms(config->interval_ms);
            continue;
        }

        int value_a = 0;
        int value_b = 0;
        int detected_pin_a = -1;
        int detected_pin_b = -1;

        if (!parse_gpio_snapshot(buffer, &value_a, &value_b, &detected_pin_a, &detected_pin_b)) {
            fprintf(stderr, "warning: skipped invalid snapshot from %s: %s\n", config->device_path, buffer);
            sleep_ms(config->interval_ms);
            continue;
        }

        print_json_sample(++seq,
                          now_ms(),
                          "device",
                          (int64_t)value_a,
                          (int64_t)value_b,
                          config->pin_a >= 0 ? config->pin_a : detected_pin_a,
                          config->pin_b >= 0 ? config->pin_b : detected_pin_b);

        sleep_ms(config->interval_ms);
    }

    return 0;
}

static int run_device_source(const config_t *config) {
    if (config->device_format == DEVICE_BINARY_U32) {
        return run_device_binary_u32(config);
    }

    return run_device_text(config);
}

static size_t width_bytes(unsigned width_bits) {
    switch (width_bits) {
        case 8:
            return 1;
        case 16:
            return 2;
        case 32:
            return 4;
        default:
            return 0;
    }
}

static uint32_t read_register_value(volatile uint8_t *base, size_t offset, unsigned width_bits) {
    switch (width_bits) {
        case 8:
            return *(volatile uint8_t *)(base + offset);
        case 16:
            return *(volatile uint16_t *)(base + offset);
        case 32:
            return *(volatile uint32_t *)(base + offset);
        default:
            return 0;
    }
}

static int run_mem_source(const config_t *config) {
    const long page_size_long = sysconf(_SC_PAGESIZE);
    if (page_size_long <= 0) {
        fprintf(stderr, "error: cannot determine system page size\n");
        return 1;
    }

    const size_t page_size = (size_t)page_size_long;
    const size_t bytes = width_bytes(config->width_bits);
    const size_t max_offset = config->offset_a > config->offset_b ? config->offset_a : config->offset_b;
    const uint64_t aligned_addr = config->mem_address & ~((uint64_t)page_size - 1ULL);
    const size_t page_offset = (size_t)(config->mem_address - aligned_addr);
    const size_t required_len = page_offset + max_offset + bytes;
    const size_t map_len = ((required_len + page_size - 1U) / page_size) * page_size;

    int fd = open(config->mem_device, O_RDONLY | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "error: cannot open %s: %s\n", config->mem_device, strerror(errno));
        return 1;
    }

    void *mapped = mmap(NULL, map_len, PROT_READ, MAP_SHARED, fd, (off_t)aligned_addr);
    if (mapped == MAP_FAILED) {
        fprintf(stderr,
                "error: cannot mmap 0x%" PRIx64 " from %s: %s\n",
                config->mem_address,
                config->mem_device,
                strerror(errno));
        close(fd);
        return 1;
    }

    volatile uint8_t *base = (volatile uint8_t *)mapped + page_offset;
    uint64_t seq = 0;

    while (keep_running && (config->max_samples == 0 || seq < config->max_samples)) {
        uint32_t value_a = read_register_value(base, config->offset_a, config->width_bits);
        uint32_t value_b = read_register_value(base, config->offset_b, config->width_bits);

        print_json_sample(++seq,
                          now_ms(),
                          "mem",
                          (int64_t)value_a,
                          (int64_t)value_b,
                          config->pin_a,
                          config->pin_b);

        sleep_ms(config->interval_ms);
    }

    munmap(mapped, map_len);
    close(fd);
    return 0;
}

static int parse_args(int argc, char **argv, config_t *config) {
    static const struct option options[] = {
        {"device", required_argument, NULL, 'd'},
        {"device-format", required_argument, NULL, 'f'},
        {"mem-address", required_argument, NULL, 'a'},
        {"mem-device", required_argument, NULL, 'm'},
        {"offset-a", required_argument, NULL, 'A'},
        {"offset-b", required_argument, NULL, 'B'},
        {"width", required_argument, NULL, 'w'},
        {"interval-ms", required_argument, NULL, 'i'},
        {"max-samples", required_argument, NULL, 'n'},
        {"pin-a", required_argument, NULL, 'p'},
        {"pin-b", required_argument, NULL, 'q'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    int opt = 0;
    while ((opt = getopt_long(argc, argv, "d:f:a:m:A:B:w:i:n:p:q:h", options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                config->source = SOURCE_DEVICE;
                config->device_path = optarg;
                break;
            case 'f':
                if (strcmp(optarg, "text") == 0) {
                    config->device_format = DEVICE_TEXT;
                } else if (strcmp(optarg, "binary-u32") == 0) {
                    config->device_format = DEVICE_BINARY_U32;
                } else {
                    fprintf(stderr, "error: invalid --device-format. Use text or binary-u32\n");
                    return 1;
                }
                break;
            case 'a':
                config->source = SOURCE_MEM;
                if (!parse_u64(optarg, &config->mem_address)) {
                    fprintf(stderr, "error: invalid --mem-address value: %s\n", optarg);
                    return 1;
                }
                break;
            case 'm':
                config->mem_device = optarg;
                break;
            case 'A':
                if (!parse_size(optarg, &config->offset_a)) {
                    fprintf(stderr, "error: invalid --offset-a value: %s\n", optarg);
                    return 1;
                }
                break;
            case 'B':
                if (!parse_size(optarg, &config->offset_b)) {
                    fprintf(stderr, "error: invalid --offset-b value: %s\n", optarg);
                    return 1;
                }
                break;
            case 'w':
                if (!parse_uint(optarg, &config->width_bits) ||
                    (config->width_bits != 8 && config->width_bits != 16 && config->width_bits != 32)) {
                    fprintf(stderr, "error: invalid --width. Use 8, 16 or 32\n");
                    return 1;
                }
                break;
            case 'i':
                if (!parse_uint(optarg, &config->interval_ms)) {
                    fprintf(stderr, "error: invalid --interval-ms value: %s\n", optarg);
                    return 1;
                }
                break;
            case 'n':
                if (!parse_u64(optarg, &config->max_samples)) {
                    fprintf(stderr, "error: invalid --max-samples value: %s\n", optarg);
                    return 1;
                }
                break;
            case 'p':
                if (!parse_int_arg(optarg, &config->pin_a)) {
                    fprintf(stderr, "error: invalid --pin-a value: %s\n", optarg);
                    return 1;
                }
                break;
            case 'q':
                if (!parse_int_arg(optarg, &config->pin_b)) {
                    fprintf(stderr, "error: invalid --pin-b value: %s\n", optarg);
                    return 1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 2;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (config->source == SOURCE_NONE) {
        fprintf(stderr, "error: select one source: --device PATH or --mem-address ADDR\n");
        print_usage(argv[0]);
        return 1;
    }

    if (config->source == SOURCE_MEM && config->mem_address == 0) {
        fprintf(stderr, "error: --mem-address cannot be zero\n");
        return 1;
    }

    return 0;
}

int main(int argc, char **argv) {
    config_t config = {
        .source = SOURCE_NONE,
        .device_path = NULL,
        .mem_device = "/dev/mem",
        .mem_address = 0,
        .offset_a = 0,
        .offset_b = 4,
        .width_bits = 32,
        .interval_ms = 500,
        .max_samples = 0,
        .pin_a = -1,
        .pin_b = -1,
        .device_format = DEVICE_TEXT
    };

    int parse_status = parse_args(argc, argv, &config);
    if (parse_status == 2) {
        return 0;
    }
    if (parse_status != 0) {
        return parse_status;
    }

    setvbuf(stdout, NULL, _IOLBF, 0);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (config.source == SOURCE_MEM) {
        return run_mem_source(&config);
    }

    return run_device_source(&config);
}
