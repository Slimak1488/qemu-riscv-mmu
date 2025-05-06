#include <stdarg.h>

extern void putc(char c);

static void print_str(const char* s) {
    while (*s) {
        putc(*s++);
    }
}

static void print_uint(unsigned int num) {
    char buf[10];
    char* p = buf + sizeof(buf) - 1;
    *p = '\0';
    
    do {
        *--p = '0' + (num % 10);
        num /= 10;
    } while (num != 0);
    
    print_str(p);
}

static void print_hex(unsigned int num) {
    const char hex_digits[] = "0123456789abcdef";
    char buf[9];
    char* p = buf + sizeof(buf) - 1;
    *p = '\0';
    
    for (int i = 0; i < 8; i++) {
        *--p = hex_digits[num & 0xF];
        num >>= 4;
    }
    
    print_str(p);
}

static void print_hex64(unsigned long long num) {
    const char hex_digits[] = "0123456789abcdef";
    char buf[17];
    char* p = buf + sizeof(buf) - 1;
    *p = '\0';
    
    for (int i = 0; i < 16; i++) {
        *--p = hex_digits[num & 0xF];
        num >>= 4;
    }
    
    print_str(p);
}

void printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 's': {
                    char* s = va_arg(args, char*);
                    print_str(s);
                    break;
                }
                case 'd': {
                    int num = va_arg(args, int);
                    if (num < 0) {
                        putc('-');
                        num = -num;
                    }
                    print_uint((unsigned int)num);
                    break;
                }
                case 'u': {
                    unsigned int num = va_arg(args, unsigned int);
                    print_uint(num);
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    print_hex(num);
                    break;
                }
                case 'l': {
                    // Обработка long (64-бит)
                    fmt++;
                    if (*fmt == 'x') {
                        unsigned long long num = va_arg(args, unsigned long long);
                        print_hex64(num);
                    } else {
                        // Неподдерживаемый спецификатор
                        putc('%');
                        putc('l');
                        putc(*fmt);
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    putc(c);
                    break;
                }
                case '%': {
                    putc('%');
                    break;
                }
                default: {
                    putc('%');
                    putc(*fmt);
                    break;
                }
            }
        } else {
            putc(*fmt);
        }
        fmt++;
    }
    
    va_end(args);
}