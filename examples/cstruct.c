// cstruct.c — example for cstruct
//
// Various struct layouts demonstrating padding and alignment.

struct packet {
    char type;
    void *data;
    short len;
    char flags;
};

struct optimized {
    void *data;
    short len;
    char type;
    char flags;
};

struct mixed {
    char a;
    int b;
    char c;
    double d;
    char e;
};

struct bitfields {
    unsigned a : 3;
    unsigned b : 5;
    unsigned c : 8;
    unsigned d : 16;
};
