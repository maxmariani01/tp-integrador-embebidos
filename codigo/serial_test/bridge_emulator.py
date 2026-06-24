#!/usr/bin/env python3
"""
Emulador del bridge ROS 2 para probar la variante Deadman en banco (sin robot).

- Lee tramas @LL:TTT:PAYLOAD:CC\\n que emite la Blue Pill.
- Valida longitud y checksum XOR (igual que el bridge real).
- Muestra cada trama con el topico ROS 2 que publicaria el bridge.
- Imprime la reaccion SIMULADA del robot (para que la demo se entienda).
- Responde ACK:ok a cada EVT:deadman, cerrando el lazo de reenvio del firmware.

Uso:
    python3 bridge_emulator.py [puerto]        # default /dev/ttyUSB0
Requiere:
    pip install pyserial
"""
import sys
import time

try:
    import serial  # pyserial
except ImportError:
    sys.exit("Falta pyserial: instalalo con  pip install pyserial")

TOPIC = {
    "DAT": "bridge/data",
    "EVT": "bridge/event",
    "STS": "bridge/status",
    "ACK": "bridge/ack",
    "ERR": "bridge/error",
    "CMD": "bridge/cmd",
}


def xor(data: bytes) -> int:
    c = 0
    for b in data:
        c ^= b
    return c


def encode(ttt: str, payload: str) -> bytes:
    """Arma una trama valida @LL:TTT:PAYLOAD:CC\\n."""
    body = f"{ttt}:{payload}"
    ll = f"{len(body):02X}"
    cc = f"{xor(f'{ll}:{body}'.encode('ascii')):02X}"
    return f"@{ll}:{body}:{cc}\n".encode("ascii")


def parse(line: str):
    """Devuelve (ttt, payload, cc, ok) o None si no parsea."""
    if not line.startswith("@"):
        return None
    s = line[1:]
    try:
        ll, rest = s.split(":", 1)
        ttt, rest2 = rest.split(":", 1)
        payload, cc = rest2.rsplit(":", 1)
    except ValueError:
        return None
    body = f"{ttt}:{payload}"
    len_ok = (f"{len(body):02X}" == ll.upper())
    chk_ok = (f"{xor(f'{ll}:{body}'.encode('ascii')):02X}" == cc.upper())
    return ttt, payload, cc, (len_ok and chk_ok)


def robot_reaction(ttt: str, payload: str):
    if ttt == "EVT" and payload == "deadman_ok":
        return ">> ROBOT: operador presente -> habilitado a operar"
    if ttt == "EVT" and payload == "deadman":
        return ">> ROBOT: !PARADA INMEDIATA!"
    if ttt == "DAT" and payload.startswith("spd="):
        return f">> ROBOT: avanza a {payload[4:]} % de velocidad"
    if ttt == "STS" and payload == "ALIVE":
        return ">> ROBOT: heartbeat de operador presente"
    if ttt == "STS" and payload == "EMERGENCY":
        return ">> ROBOT: emergencia disparada por un sensor"
    return None


def main():
    # Forzar line-buffering: si no, al mandar la salida a un pipe (| tee) Python
    # acumula en un buffer y no se ve nada hasta que se llena.
    try:
        sys.stdout.reconfigure(line_buffering=True)
    except Exception:
        pass
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
    ser = serial.Serial(port, 115200, timeout=0.2)
    print(f"=== Bridge ROS 2 emulado en {port} (115200 8N1). Ctrl-C para salir. ===")
    buf = b""
    while True:
        data = ser.read(256)
        if data:
            buf += data
            while b"\n" in buf:
                raw, buf = buf.split(b"\n", 1)
                line = raw.decode("ascii", "replace").strip("\r")
                if not line:
                    continue
                r = parse(line)
                if r is None:
                    print(f"RX  (no-trama)  {line!r}")
                    continue
                ttt, payload, cc, ok = r
                topic = TOPIC.get(ttt, "bridge/rx_raw")
                flag = "OK " if ok else "BAD"
                print(f"RX [{flag}] {ttt}:{payload:<12} -> {topic}")
                rr = robot_reaction(ttt, payload)
                if rr:
                    print("           " + rr)
                if ok and ttt == "EVT" and payload == "deadman":
                    frame = encode("ACK", "ok")
                    ser.write(frame)
                    print(f"TX        ACK:ok          ({frame.decode().strip()})")
        time.sleep(0.005)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nfin")
