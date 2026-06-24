#include <stdio.h>
#include <string.h>

#include "protocol.h"

/*
 * Este archivo fue dejado intencionalmente incompleto para la práctica.
 *
 * Objetivo:
 * - construir tramas del tipo @LL:TTT:PAYLOAD:CC\n
 * - calcular checksum XOR de LL:TTT:PAYLOAD
 * - decodificar el body TTT:PAYLOAD
 *
 * Documentación útil:
 * - docs/PROTOCOL.md
 * - docs/FRAME_GENERATION_AND_PARSING.md
 */

const char *protocol_type_to_text(protocol_type_t type)
{
    switch (type) {
    case PROTOCOL_TYPE_CMD:
        return "CMD";
    case PROTOCOL_TYPE_DAT:
        return "DAT";
    case PROTOCOL_TYPE_EVT:
        return "EVT";
    case PROTOCOL_TYPE_STS:
        return "STS";
    case PROTOCOL_TYPE_ACK:
        return "ACK";
    case PROTOCOL_TYPE_ERR:
        return "ERR";
    default:
        return "INV";
    }
}

protocol_type_t protocol_type_from_text(const char *text)
{
    if (text == NULL) {
        return PROTOCOL_TYPE_INVALID;
    }

    if (strncmp(text, "CMD", PROTOCOL_TYPE_LENGTH) == 0) {
        return PROTOCOL_TYPE_CMD;
    }
    if (strncmp(text, "DAT", PROTOCOL_TYPE_LENGTH) == 0) {
        return PROTOCOL_TYPE_DAT;
    }
    if (strncmp(text, "EVT", PROTOCOL_TYPE_LENGTH) == 0) {
        return PROTOCOL_TYPE_EVT;
    }
    if (strncmp(text, "STS", PROTOCOL_TYPE_LENGTH) == 0) {
        return PROTOCOL_TYPE_STS;
    }
    if (strncmp(text, "ACK", PROTOCOL_TYPE_LENGTH) == 0) {
        return PROTOCOL_TYPE_ACK;
    }
    if (strncmp(text, "ERR", PROTOCOL_TYPE_LENGTH) == 0) {
        return PROTOCOL_TYPE_ERR;
    }

    return PROTOCOL_TYPE_INVALID;
}

bool protocol_message_set(protocol_message_t *message, protocol_type_t type, const char *payload)
{
    size_t payload_length;

    if ((message == NULL) || (payload == NULL)) {
        return false;
    }

    payload_length = strlen(payload);
    if (payload_length > PROTOCOL_MAX_PAYLOAD_LENGTH) {
        return false;
    }

    message->type = type;
    message->payload_length = (uint8_t) payload_length;
    memcpy(message->payload, payload, payload_length + 1U);
    return true;
}

uint8_t protocol_compute_checksum(const char *data, size_t length)
{
    uint8_t checksum = 0U;
    size_t i;

    if (data == NULL) {
        return 0U;
    }

    /* XOR byte a byte. Si un solo byte cambia, el acumulado final cambia,
     * y por eso la validacion puede detectar la corrupcion. */
    for (i = 0U; i < length; i++) {
        checksum ^= (uint8_t) data[i];
    }

    return checksum;
}

bool protocol_encode_frame(const protocol_message_t *message, char *frame, size_t frame_size, size_t *frame_length)
{
    const char *type_text;
    char body[PROTOCOL_MAX_BODY_SIZE + 1U];
    char checksum_input[PROTOCOL_MAX_FRAME_LENGTH + 1U];
    int body_length;
    int checksum_input_length;
    int frame_written;
    uint8_t checksum;

    if ((message == NULL) || (frame == NULL) || (frame_length == NULL)) {
        return false;
    }

    /* El tipo tiene que ser uno conocido (CMD, DAT, EVT, STS, ACK, ERR). */
    if (message->type >= PROTOCOL_TYPE_INVALID) {
        return false;
    }
    if (message->payload_length > PROTOCOL_MAX_PAYLOAD_LENGTH) {
        return false;
    }

    type_text = protocol_type_to_text(message->type);

    /* 1. Cuerpo: "TTT:PAYLOAD". Esto es lo unico que mide LL. */
    body_length = snprintf(body, sizeof(body), "%s%c%s",
                           type_text, PROTOCOL_SEPARATOR_CHAR, message->payload);
    if ((body_length < 0) || ((size_t) body_length >= sizeof(body))) {
        return false;
    }

    /* 2-3. Entrada del checksum: "LL:TTT:PAYLOAD".
     * LL es la longitud del cuerpo en dos digitos hex MAYUSCULA (%02X). */
    checksum_input_length = snprintf(checksum_input, sizeof(checksum_input), "%02X%c%s",
                                     (unsigned int) body_length, PROTOCOL_SEPARATOR_CHAR, body);
    if ((checksum_input_length < 0) || ((size_t) checksum_input_length >= sizeof(checksum_input))) {
        return false;
    }

    /* 4. Checksum XOR sobre "LL:TTT:PAYLOAD" (sin '@' ni '\n'). */
    checksum = protocol_compute_checksum(checksum_input, (size_t) checksum_input_length);

    /* 5. Trama final: '@' + "LL:TTT:PAYLOAD" + ':' + CC + '\n'. */
    frame_written = snprintf(frame, frame_size, "%c%s%c%02X%c",
                             PROTOCOL_START_CHAR, checksum_input,
                             PROTOCOL_SEPARATOR_CHAR, checksum, PROTOCOL_END_CHAR);
    if ((frame_written < 0) || ((size_t) frame_written >= frame_size)) {
        return false;
    }

    *frame_length = (size_t) frame_written;
    return true;
}

bool protocol_decode_body(const char *body, uint8_t body_length, protocol_message_t *message)
{
    char type_text[PROTOCOL_TYPE_LENGTH + 1U];
    protocol_type_t type;
    uint8_t payload_length;

    if ((body == NULL) || (message == NULL)) {
        return false;
    }

    /* Forma minima: 3 letras de tipo + ':' + al menos 1 char de payload. */
    if (body_length < (PROTOCOL_TYPE_LENGTH + 2U)) {
        return false;
    }

    /* El separador interno tiene que estar justo despues del tipo (posicion 3). */
    if (body[PROTOCOL_TYPE_LENGTH] != PROTOCOL_SEPARATOR_CHAR) {
        return false;
    }

    /* Extraer las 3 letras de tipo y traducirlas a un tipo conocido. */
    memcpy(type_text, body, PROTOCOL_TYPE_LENGTH);
    type_text[PROTOCOL_TYPE_LENGTH] = '\0';
    type = protocol_type_from_text(type_text);
    if (type == PROTOCOL_TYPE_INVALID) {
        return false;
    }

    /* El payload es todo lo que sigue al separador. */
    payload_length = (uint8_t) (body_length - PROTOCOL_TYPE_LENGTH - 1U);
    if (payload_length > PROTOCOL_MAX_PAYLOAD_LENGTH) {
        return false;
    }

    message->type = type;
    message->payload_length = payload_length;
    memcpy(message->payload, body + PROTOCOL_TYPE_LENGTH + 1U, payload_length);
    message->payload[payload_length] = '\0';
    return true;
}
