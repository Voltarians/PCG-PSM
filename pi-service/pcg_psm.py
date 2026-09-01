#!/usr/bin/env python3
"""PCG-PSM Raspberry Pi companion daemon.

Kernel overlays own the safety-critical shutdown handshake:
- GPIO17: gpio-shutdown input, active low from Uno shutdown request
- GPIO22: gpio-poweroff output, high only when Linux is safe for power removal

This daemon owns only:
- GPIO27 heartbeat generation
- optional USB serial telemetry ingestion from the Uno
- status publication under /run/pcg-psm/status.json
"""

from __future__ import annotations

import argparse
import configparser
import json
import logging
import os
import signal
import sys
import threading
import time
from pathlib import Path
from typing import Any

from gpiozero import OutputDevice

try:
    import serial  # type: ignore
except ImportError:
    serial = None

LOG = logging.getLogger("pcg-psm")
STOP = threading.Event()


class AtomicJsonFile:
    def __init__(self, path: Path) -> None:
        self.path = path
        self.path.parent.mkdir(parents=True, exist_ok=True)

    def write(self, payload: dict[str, Any]) -> None:
        tmp = self.path.with_suffix(self.path.suffix + ".tmp")
        tmp.write_text(json.dumps(payload, separators=(",", ":")) + "\n", encoding="utf-8")
        os.replace(tmp, self.path)


def load_config(path: Path) -> configparser.ConfigParser:
    cfg = configparser.ConfigParser()
    cfg.read_dict(
        {
            "gpio": {"heartbeat_pin": "27", "heartbeat_half_period_s": "0.5"},
            "serial": {"enabled": "true", "device": "/dev/ttyACM0", "baud": "115200", "reconnect_s": "5"},
            "status": {"path": "/run/pcg-psm/status.json"},
        }
    )
    cfg.read(path)
    return cfg


def serial_worker(cfg: configparser.ConfigParser, status_file: AtomicJsonFile) -> None:
    if not cfg.getboolean("serial", "enabled", fallback=True):
        LOG.info("Uno serial telemetry disabled")
        return
    if serial is None:
        LOG.warning("python3-serial is not installed; heartbeat remains active")
        return

    device = cfg.get("serial", "device", fallback="/dev/ttyACM0")
    baud = cfg.getint("serial", "baud", fallback=115200)
    reconnect_s = cfg.getfloat("serial", "reconnect_s", fallback=5.0)

    while not STOP.is_set():
        try:
            LOG.info("Opening Uno telemetry on %s at %d baud", device, baud)
            with serial.Serial(device, baudrate=baud, timeout=1.0) as port:
                port.reset_input_buffer()
                port.write(b"STATUS\n")
                while not STOP.is_set():
                    raw = port.readline()
                    if not raw:
                        continue
                    line = raw.decode("utf-8", errors="replace").strip()
                    if not line.startswith("{"):
                        LOG.debug("Uno: %s", line)
                        continue
                    try:
                        payload = json.loads(line)
                    except json.JSONDecodeError:
                        LOG.warning("Malformed Uno JSON: %s", line)
                        continue
                    if payload.get("type") == "status":
                        payload["host_epoch_s"] = int(time.time())
                        status_file.write(payload)
                    elif payload.get("type") == "event":
                        LOG.info("Uno event: %s", line)
        except (OSError, getattr(serial, "SerialException", OSError)) as exc:
            LOG.warning("Uno serial unavailable: %s", exc)
            STOP.wait(reconnect_s)
        except Exception:
            LOG.exception("Unexpected serial worker failure")
            STOP.wait(reconnect_s)


def install_signal_handlers() -> None:
    def _stop(signum: int, _frame: Any) -> None:
        LOG.info("Signal %d received; stopping heartbeat", signum)
        STOP.set()

    signal.signal(signal.SIGTERM, _stop)
    signal.signal(signal.SIGINT, _stop)


def main() -> int:
    parser = argparse.ArgumentParser(description="PCG-PSM Raspberry Pi heartbeat/telemetry daemon")
    parser.add_argument("--config", default="/etc/pcg-psm/pcg-psm.conf")
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO,
                        format="%(asctime)s %(levelname)s %(name)s: %(message)s")
    install_signal_handlers()

    cfg = load_config(Path(args.config))
    heartbeat_pin = cfg.getint("gpio", "heartbeat_pin", fallback=27)
    half_period = max(0.1, cfg.getfloat("gpio", "heartbeat_half_period_s", fallback=0.5))
    status_file = AtomicJsonFile(Path(cfg.get("status", "path", fallback="/run/pcg-psm/status.json")))

    telemetry_thread = threading.Thread(target=serial_worker, args=(cfg, status_file), name="pcg-psm-serial", daemon=True)
    telemetry_thread.start()

    # GPIO17 and GPIO22 MUST NOT be claimed here; the kernel overlays own them.
    heartbeat = OutputDevice(heartbeat_pin, active_high=True, initial_value=False)
    LOG.info("Heartbeat active on BCM GPIO%d (period %.2fs)", heartbeat_pin, half_period * 2.0)

    try:
        while not STOP.wait(half_period):
            heartbeat.toggle()
    finally:
        heartbeat.off()
        heartbeat.close()
        STOP.set()
        telemetry_thread.join(timeout=2.0)
        LOG.info("PCG-PSM daemon stopped")

    return 0


if __name__ == "__main__":
    sys.exit(main())
