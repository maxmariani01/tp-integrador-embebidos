/*
 * Test de host para la Etapa 1: framing y checksum.
 * Compila SOLO protocol.c (sin FreeRTOS ni hardware) y verifica que las
 * funciones generen exactamente las tramas de la tabla del TP.
 */
#include <stdio.h>
#include <string.h>

#include "protocol.h"

static int g_pass = 0;
static int g_fail = 0;

/* Imprime una trama mostrando el \n como texto "\n" para poder verla. */
static void print_visible(const char *s, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '\n') {
            printf("\\n");
        } else {
            putchar(s[i]);
        }
    }
}

/* Verifica que encode(type,payload) == esperado (con \n al final). */
static void check_encode(protocol_type_t type, const char *payload, const char *expected)
{
    protocol_message_t msg;
    char frame[PROTOCOL_MAX_FRAME_LENGTH + 1U];
    size_t frame_len = 0U;

    protocol_message_set(&msg, type, payload);
    bool ok = protocol_encode_frame(&msg, frame, sizeof(frame), &frame_len);
    frame[frame_len] = '\0';

    bool match = ok && (strcmp(frame, expected) == 0);
    printf("  encode(%-3s, %-16s) -> ", protocol_type_to_text(type), payload);
    print_visible(frame, frame_len);
    if (match) {
        printf("   [OK]\n");
        g_pass++;
    } else {
        printf("   [FALLA] esperaba ");
        print_visible(expected, strlen(expected));
        printf("\n");
        g_fail++;
    }
}

/* Verifica el checksum XOR de una cadena. */
static void check_checksum(const char *data, uint8_t expected)
{
    uint8_t cs = protocol_compute_checksum(data, strlen(data));
    bool match = (cs == expected);
    printf("  checksum(\"%s\") = 0x%02X (esperaba 0x%02X)   [%s]\n",
           data, cs, expected, match ? "OK" : "FALLA");
    if (match) { g_pass++; } else { g_fail++; }
}

/* Verifica que decode("TTT:PAYLOAD") extraiga tipo y payload correctos. */
static void check_decode(const char *body, protocol_type_t exp_type, const char *exp_payload)
{
    protocol_message_t msg;
    memset(&msg, 0, sizeof(msg));
    bool ok = protocol_decode_body(body, (uint8_t) strlen(body), &msg);
    bool match = ok && (msg.type == exp_type) && (strcmp(msg.payload, exp_payload) == 0);
    printf("  decode(\"%s\") -> type=%s payload=\"%s\"   [%s]\n",
           body, ok ? protocol_type_to_text(msg.type) : "(falla)", ok ? msg.payload : "",
           match ? "OK" : "FALLA");
    if (match) { g_pass++; } else { g_fail++; }
}

/* Verifica que decode rechace un body invalido. */
static void check_decode_reject(const char *body)
{
    protocol_message_t msg;
    bool ok = protocol_decode_body(body, (uint8_t) strlen(body), &msg);
    printf("  decode(\"%s\") -> %s   [%s]\n",
           body, ok ? "ACEPTA" : "RECHAZA", (!ok) ? "OK" : "FALLA");
    if (!ok) { g_pass++; } else { g_fail++; }
}

int main(void)
{
    printf("=== Checksum XOR ===\n");
    check_checksum("08:CMD:ping", 0x52);
    check_checksum("0E:CMD:led=toggle", 0x7D);
    check_checksum("0A:ACK:cmd=ok", 0x6B);
    check_checksum("14:ERR:code=unknown_cmd", 0x2D);

    printf("\n=== Encode (tabla de la consigna) ===\n");
    check_encode(PROTOCOL_TYPE_CMD, "ping",             "@08:CMD:ping:52\n");
    check_encode(PROTOCOL_TYPE_CMD, "led=on",           "@0A:CMD:led=on:6A\n");
    check_encode(PROTOCOL_TYPE_CMD, "led=off",          "@0B:CMD:led=off:07\n");
    check_encode(PROTOCOL_TYPE_CMD, "led=toggle",       "@0E:CMD:led=toggle:7D\n");
    check_encode(PROTOCOL_TYPE_CMD, "status?",          "@0B:CMD:status?:13\n");
    check_encode(PROTOCOL_TYPE_ACK, "pong=1",           "@0A:ACK:pong=1:22\n");
    check_encode(PROTOCOL_TYPE_ACK, "cmd=ok",           "@0A:ACK:cmd=ok:6B\n");
    check_encode(PROTOCOL_TYPE_ERR, "code=unknown_cmd", "@14:ERR:code=unknown_cmd:2D\n");

    printf("\n=== Decode (abrir el sobre) ===\n");
    check_decode("CMD:ping",       PROTOCOL_TYPE_CMD, "ping");
    check_decode("ACK:pong=1",     PROTOCOL_TYPE_ACK, "pong=1");
    check_decode("CMD:led=toggle", PROTOCOL_TYPE_CMD, "led=toggle");

    printf("\n=== Decode: casos que deben rechazarse ===\n");
    check_decode_reject("XYZ:ping");   /* tipo desconocido */
    check_decode_reject("CMDxping");   /* falta el ':' en la posicion 3 */
    check_decode_reject("CM");         /* demasiado corto */

    printf("\n=== Resultado: %d OK, %d FALLA ===\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
