#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"

/*
 * Parser incremental (FSM byte a byte).
 *
 * Consume un byte por vez y recorre 9 estados:
 * WAIT_START -> READ_LEN_HI -> READ_LEN_LO -> EXPECT_LEN_SEPARATOR ->
 * READ_BODY -> EXPECT_CHECK_SEPARATOR -> READ_CHECK_HI -> READ_CHECK_LO ->
 * EXPECT_END -> (mensaje listo)
 */

/* Convierte un caracter hexadecimal MAYUSCULA a su valor (0-15).
 * Devuelve -1 si el caracter no es un digito hex valido.
 * Acepta solo 0-9 y A-F: el protocolo rechaza minusculas. */
static int hex_char_to_nibble(char c)
{
    if ((c >= '0') && (c <= '9')) {
        return c - '0';
    }
    if ((c >= 'A') && (c <= 'F')) {
        return (c - 'A') + 10;
    }
    return -1;
}

/* Maneja un error de parsing: descarta la trama parcial y vuelve a WAIT_START.
 * Optimizacion clave (resincronizacion): si el byte que causo el error es '@',
 * se reutiliza como inicio de una trama nueva, para no perder la siguiente
 * trama valida cuando hay ruido en la linea. */
static parser_result_t parser_fail(parser_t *parser, uint8_t byte)
{
    parser_reset(parser);
    if (byte == PROTOCOL_START_CHAR) {
        parser->state = PARSER_STATE_READ_LEN_HI;
    }
    return PARSER_RESULT_ERROR;
}

/* Validacion final (en EXPECT_END, despues de recibir '\n'):
 * 1. recalcula el checksum XOR sobre "LL:TTT:PAYLOAD" y lo compara con CC,
 * 2. si coincide, decodifica el cuerpo TTT:PAYLOAD dentro de message.
 * Devuelve true solo si todo es valido. */
static bool parser_validate_and_build(const parser_t *parser, protocol_message_t *message)
{
    char checksum_input[PROTOCOL_MAX_FRAME_LENGTH + 1U];
    int checksum_input_length;
    uint8_t checksum_computed;
    uint8_t checksum_received;
    int hi;
    int lo;

    /* Reconstruir "LL:TTT:PAYLOAD" para recalcular el checksum. */
    checksum_input_length = snprintf(checksum_input, sizeof(checksum_input), "%s%c%s",
                                     parser->length_field, PROTOCOL_SEPARATOR_CHAR, parser->body);
    if ((checksum_input_length < 0) || ((size_t) checksum_input_length >= sizeof(checksum_input))) {
        return false;
    }
    checksum_computed = protocol_compute_checksum(checksum_input, (size_t) checksum_input_length);

    /* Convertir el checksum recibido (2 digitos hex) a un byte. */
    hi = hex_char_to_nibble(parser->checksum_field[0]);
    lo = hex_char_to_nibble(parser->checksum_field[1]);
    if ((hi < 0) || (lo < 0)) {
        return false;
    }
    checksum_received = (uint8_t) ((hi << 4) | lo);

    if (checksum_computed != checksum_received) {
        return false;
    }

    /* Checksum OK: abrir el sobre (valida tipo y copia payload). */
    return protocol_decode_body(parser->body, parser->expected_body_length, message);
}

void parser_reset(parser_t *parser)
{
    if (parser == NULL) {
        return;
    }

    parser->state = PARSER_STATE_WAIT_START;
    parser->length_field[0] = '\0';
    parser->length_field[1] = '\0';
    parser->length_field[2] = '\0';
    parser->checksum_field[0] = '\0';
    parser->checksum_field[1] = '\0';
    parser->checksum_field[2] = '\0';
    parser->body[0] = '\0';
    parser->expected_body_length = 0U;
    parser->body_index = 0U;
}

void parser_init(parser_t *parser)
{
    parser_reset(parser);
}

parser_result_t parser_consume_byte(parser_t *parser, uint8_t byte, protocol_message_t *message)
{
    int hi;
    int lo;

    if ((parser == NULL) || (message == NULL)) {
        return PARSER_RESULT_ERROR;
    }

    /* '\r' (0x0D) se ignora en todo momento: no avanza de estado ni rompe.
     * Permite aceptar tanto "...:52\n" como "...:52\r\n". */
    if (byte == '\r') {
        return PARSER_RESULT_IN_PROGRESS;
    }

    switch (parser->state) {
    case PARSER_STATE_WAIT_START:
        /* Ignora todo hasta encontrar el inicio de trama '@'. */
        if (byte == PROTOCOL_START_CHAR) {
            parser->state = PARSER_STATE_READ_LEN_HI;
        }
        return PARSER_RESULT_IN_PROGRESS;

    case PARSER_STATE_READ_LEN_HI:
        if (hex_char_to_nibble((char) byte) < 0) {
            return parser_fail(parser, byte);
        }
        parser->length_field[0] = (char) byte;
        parser->state = PARSER_STATE_READ_LEN_LO;
        return PARSER_RESULT_IN_PROGRESS;

    case PARSER_STATE_READ_LEN_LO:
        if (hex_char_to_nibble((char) byte) < 0) {
            return parser_fail(parser, byte);
        }
        parser->length_field[1] = (char) byte;
        parser->length_field[2] = '\0';

        /* Convertir los dos digitos hex a la longitud del cuerpo. */
        hi = hex_char_to_nibble(parser->length_field[0]);
        lo = hex_char_to_nibble(parser->length_field[1]);
        parser->expected_body_length = (uint8_t) ((hi << 4) | lo);

        /* La longitud declarada tiene que tener sentido y entrar en el buffer. */
        if ((parser->expected_body_length == 0U) ||
            (parser->expected_body_length > PROTOCOL_MAX_BODY_SIZE)) {
            return parser_fail(parser, byte);
        }
        parser->state = PARSER_STATE_EXPECT_LEN_SEPARATOR;
        return PARSER_RESULT_IN_PROGRESS;

    case PARSER_STATE_EXPECT_LEN_SEPARATOR:
        if (byte != PROTOCOL_SEPARATOR_CHAR) {
            return parser_fail(parser, byte);
        }
        parser->state = PARSER_STATE_READ_BODY;
        return PARSER_RESULT_IN_PROGRESS;

    case PARSER_STATE_READ_BODY:
        /* Acumula el cuerpo TTT:PAYLOAD byte a byte. */
        parser->body[parser->body_index] = (char) byte;
        parser->body_index++;
        if (parser->body_index >= parser->expected_body_length) {
            parser->body[parser->body_index] = '\0';
            parser->state = PARSER_STATE_EXPECT_CHECK_SEPARATOR;
        }
        return PARSER_RESULT_IN_PROGRESS;

    case PARSER_STATE_EXPECT_CHECK_SEPARATOR:
        if (byte != PROTOCOL_SEPARATOR_CHAR) {
            return parser_fail(parser, byte);
        }
        parser->state = PARSER_STATE_READ_CHECK_HI;
        return PARSER_RESULT_IN_PROGRESS;

    case PARSER_STATE_READ_CHECK_HI:
        if (hex_char_to_nibble((char) byte) < 0) {
            return parser_fail(parser, byte);
        }
        parser->checksum_field[0] = (char) byte;
        parser->state = PARSER_STATE_READ_CHECK_LO;
        return PARSER_RESULT_IN_PROGRESS;

    case PARSER_STATE_READ_CHECK_LO:
        if (hex_char_to_nibble((char) byte) < 0) {
            return parser_fail(parser, byte);
        }
        parser->checksum_field[1] = (char) byte;
        parser->checksum_field[2] = '\0';
        parser->state = PARSER_STATE_EXPECT_END;
        return PARSER_RESULT_IN_PROGRESS;

    case PARSER_STATE_EXPECT_END:
        if (byte != PROTOCOL_END_CHAR) {
            return parser_fail(parser, byte);
        }
        /* Llego el '\n': validar checksum y cuerpo antes de entregar. */
        if (!parser_validate_and_build(parser, message)) {
            return parser_fail(parser, byte);
        }
        parser_reset(parser);
        return PARSER_RESULT_MESSAGE_READY;

    default:
        return parser_fail(parser, byte);
    }
}
