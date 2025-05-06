typedef unsigned char uint8_t;
typedef signed char int8_t;

enum PageBits {
    Empty = 0,
    Taken = 1 << 0,
    Last = 1 << 1,
};

struct Page {
   uint8_t flags;
};

