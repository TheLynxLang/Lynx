#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t* buf;
    size_t size;
    size_t capacity;
} CodeBuffer;

void emit_byte(CodeBuffer* cb, uint8_t byte) {
    if (cb->size >= cb->capacity) {
        cb->capacity = cb->capacity ? cb->capacity * 2 : 4096;
        cb->buf = realloc(cb->buf, cb->capacity);
    }
    cb->buf[cb->size++] = byte;
}

void emit_mov_rax_imm64(CodeBuffer* cb, uint64_t value) {
    emit_byte(cb, 0x48);
    emit_byte(cb, 0xB8);
    for (int i = 0; i < 8; i++) emit_byte(cb, (value >> (i * 8)) & 0xFF);
}

void emit_ret(CodeBuffer* cb) {
    emit_byte(cb, 0xC3);
}

uint8_t* compile_to_machine(const char* source, size_t* out_size) {
    CodeBuffer cb = {0};
    cb.capacity = 4096;
    cb.buf = malloc(cb.capacity);

    // mov rax, 42
    emit_mov_rax_imm64(&cb, 42);
    emit_ret(&cb);

    *out_size = cb.size;
    return cb.buf;
}