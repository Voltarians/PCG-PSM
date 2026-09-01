# PCG-PSM Raspberry Pi Service — Rev A.2

The Raspberry Pi side deliberately splits responsibilities between **kernel device-tree overlays** and a small userspace daemon.

## Safety-critical GPIO ownership

The kernel owns the two shutdown handshake lines:

- **GPIO17** — shutdown request from the Uno. `gpio-shutdown` treats the optocoupler's LOW as a normal power-key event and starts Linux shutdown even if the PCG-PSM daemon has crashed.
- **GPIO22** — safe-to-cut-power acknowledgement to the Uno. `gpio-poweroff` drives the pin HIGH only after Linux reaches the final poweroff state; the optocoupler then pulls Uno D4 LOW.

The userspace daemon **does not claim GPIO17 or GPIO22**.

## Userspace daemon

`pcg_psm.py` owns:

- BCM **GPIO27** heartbeat generation (1 Hz by default)
- optional 115200-baud Uno USB telemetry from `/dev/ttyACM0`
- latest parsed status at `/run/pcg-psm/status.json`

If USB serial is absent, the heartbeat continues. This keeps power supervision independent from telemetry.

## Files

- `pcg_psm.py` — heartbeat / telemetry daemon
- `pcg-psm.service` — systemd unit
- `pcg-psm.conf` — runtime configuration
- `config.txt.example` — required Pi device-tree overlays
- `install.sh` — Raspberry Pi OS installer

## Install

```bash
cd pi-service
sudo bash install.sh
sudo reboot
```

After reboot:

```bash
systemctl status pcg-psm
cat /run/pcg-psm/status.json
```

## Expected handshake

```text
RUN
  Pi daemon toggles GPIO27 heartbeat

ACC OFF
  Uno waits ignition-off delay
  Uno D3 HIGH -> optocoupler -> GPIO17 LOW
  kernel gpio-shutdown starts Linux shutdown
  normal systemd shutdown stops PCG-Core services and flushes storage
  kernel reaches final poweroff
  GPIO22 HIGH -> optocoupler -> Uno D4 LOW
  Uno cuts main 12 V feed to DDR-60G-5
  Uno releases its own latch when ACC is absent
```

If Linux never reaches GPIO22, the Uno's hard timeout eventually removes main power and records `SHUTDOWN_HARD_TIMEOUT`.

## GPIO overlays

```text
dtoverlay=gpio-shutdown,gpio_pin=17,active_low=1,gpio_pull=2,debounce=100
dtoverlay=gpio-poweroff,gpiopin=22,active_low=0
```

`install.sh` makes a timestamped backup of `config.txt` before adding them.

## OBD Atlas / PCG-Core services

Normal systemd shutdown will stop services and sync filesystems. Services that require explicit ordering should add their own shutdown dependencies rather than putting application-specific control inside the safety heartbeat daemon.
