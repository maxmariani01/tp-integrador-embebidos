/*
 * Test de host para la Etapa 2: parser incremental (FSM).
 * Alimenta el parser byte a byte con distintas secuencias y verifica
 * cuantos mensajes validos entrega y cuantos errores produce.
 */
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "protocol.h"

static int g_pass = 0;
static int g_fail = 0;

/* Resultado de alimentar una secuencia completa al parser. */
typedef struct {
    int messages;                 /* cuantos MESSAGE_READY */
    int errors;                   /* cuantos ERROR */
    protocol_type_t last_type;    /* tipo del ultimo mensaje */
    char last_payload[64];        /* payload del ultimo mensaje */
    char first_payload[64];       /* payload del primer mensaje */
} feed_result_t;

/* Convierte una cadena C (con \n y \r reales) en bytes y la mete byte a byte. */
static feed_result_t feed(const char *stream, size_t len)
{
    parser_t parser;
    protocol_message_t msg;
    feed_result_t r = {0};

    parser_init(&parser);
    r.last_type = PROTOCOL_TYPE_INVALID;

    for (size_t i = 0; i < len; i++) {
        parser_result_t res = parser_consume_byte(&parser, (uint8_t) stream[i], &msg);
        if (res == PARSER_RESULT_MESSAGE_READY) {
            if (r.messages == 0) {
                strncpy(r.first_payload, msg.payload, sizeof(r.first_payload) - 1);
            }
            r.messages++;
            r.last_type = msg.type;
            strncpy(r.last_payload, msg.payload, sizeof(r.last_payload) - 1);
        } else if (res == PARSER_RESULT_ERROR) {
            r.errors++;
        }
    }
    return r;
}

static void report(const char *name, int cond)
{
    printf("  [%s] %s\n", cond ? "OK" : "FALLA", name);
    if (cond) { g_pass++; } else { g_fail++; }
}

/* Macro para mostrar el stream con \n y \r visibles. */
static void print_stream(const char *s, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '\n') { printf("\\n"); }
        else if (s[i] == '\r') { printf("\\r"); }
        else { putchar(s[i]); }
    }
}

#define FEED(s) feed((s), sizeof(s) - 1)

int main(void)
{
    feed_result_t r;

    printf("=== Trama valida simple ===\n");
    {
        const char in[] = "@08:CMD:ping:52\n";
        r = FEED(in);
        printf("  entrada: "); print_stream(in, sizeof(in)-1); printf("\n");
        report("entrega exactamente 1 mensaje", r.messages == 1);
        report("tipo CMD", r.last_type == PROTOCOL_TYPE_CMD);
        report("payload ping", strcmp(r.last_payload, "ping") == 0);
        report("sin errores", r.errors == 0);
    }

    printf("\n=== Basura antes de la trama (xyz@...) ===\n");
    {
        const char in[] = "xyz@08:CMD:ping:52\n";
        r = FEED(in);
        printf("  entrada: "); print_stream(in, sizeof(in)-1); printf("\n");
        report("entrega 1 mensaje pese a la basura", r.messages == 1);
        report("payload ping", strcmp(r.last_payload, "ping") == 0);
    }

    printf("\n=== Caso B del TP: ruido con @ en el medio (@0@...) ===\n");
    {
        const char in[] = "@0@08:CMD:ping:52\n";
        r = FEED(in);
        printf("  entrada: "); print_stream(in, sizeof(in)-1); printf("\n");
        report("reutiliza el @ y entrega 1 mensaje", r.messages == 1);
        report("payload ping", strcmp(r.last_payload, "ping") == 0);
        report("registra 1 error (el @ del medio)", r.errors == 1);
    }

    printf("\n=== Caso C del TP: checksum corrupto (:53) ===\n");
    {
        const char in[] = "@08:CMD:ping:53\n";
        r = FEED(in);
        printf("  entrada: "); print_stream(in, sizeof(in)-1); printf("\n");
        report("NO entrega mensaje", r.messages == 0);
        report("registra 1 error", r.errors == 1);
    }

    printf("\n=== Terminador \\r\\n (monitor serie) ===\n");
    {
        const char in[] = "@08:CMD:ping:52\r\n";
        r = FEED(in);
        printf("  entrada: "); print_stream(in, sizeof(in)-1); printf("\n");
        report("ignora \\r y entrega 1 mensaje", r.messages == 1);
        report("sin errores", r.errors == 0);
    }

    printf("\n=== Caso D del TP: dos tramas pegadas ===\n");
    {
        const char in[] = "@08:CMD:ping:52\n@0A:CMD:led=on:6A\n";
        r = FEED(in);
        printf("  entrada: "); print_stream(in, sizeof(in)-1); printf("\n");
        report("entrega 2 mensajes", r.messages == 2);
        report("primero = ping", strcmp(r.first_payload, "ping") == 0);
        report("segundo = led=on", strcmp(r.last_payload, "led=on") == 0);
    }

    printf("\n=== Pregunta de cierre: ###@... ===\n");
    {
        const char in[] = "###@08:CMD:ping:52\n";
        r = FEED(in);
        printf("  entrada: "); print_stream(in, sizeof(in)-1); printf("\n");
        report("entrega 1 mensaje, sin errores", r.messages == 1 && r.errors == 0);
    }

    printf("\n=== Longitud invalida menor que el cuerpo (@07:CMD:ping) ===\n");
    {
        const char in[] = "@07:CMD:ping:52\n";
        r = FEED(in);
        printf("  entrada: "); print_stream(in, sizeof(in)-1); printf("\n");
        report("NO entrega mensaje", r.messages == 0);
        report("registra al menos 1 error", r.errors >= 1);
    }

    printf("\n=== Tipo distinto de los conocidos (@08:XYZ:ping) ===\n");
    {
        /* checksum 43 = XOR correcto de "08:XYZ:ping": asi el rechazo viene
         * genuinamente del tipo desconocido, no de un checksum malo. */
        const char in[] = "@08:XYZ:ping:43\n";
        r = FEED(in);
        printf("  entrada: "); print_stream(in, sizeof(in)-1); printf("\n");
        report("NO entrega mensaje (tipo invalido)", r.messages == 0);
    }

    printf("\n=== Resultado: %d OK, %d FALLA ===\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
