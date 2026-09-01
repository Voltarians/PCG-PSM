#!/usr/bin/env bash
set -euo pipefail

if [[ ${EUID} -ne 0 ]]; then
  echo "Run as root: sudo ./install.sh" >&2
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_TXT="/boot/firmware/config.txt"
[[ -f "$CONFIG_TXT" ]] || CONFIG_TXT="/boot/config.txt"

if [[ ! -f "$CONFIG_TXT" ]]; then
  echo "Could not find Raspberry Pi config.txt" >&2
  exit 1
fi

missing=()
python3 -c 'import gpiozero' >/dev/null 2>&1 || missing+=(python3-gpiozero python3-lgpio)
python3 -c 'import serial' >/dev/null 2>&1 || missing+=(python3-serial)
if ((${#missing[@]})); then
  echo "Installing required Raspberry Pi OS packages: ${missing[*]}"
  apt-get update
  apt-get install -y "${missing[@]}"
fi

install -d -m 0755 /usr/local/lib/pcg-psm /etc/pcg-psm
install -m 0755 "$SCRIPT_DIR/pcg_psm.py" /usr/local/lib/pcg-psm/pcg_psm.py
install -m 0644 "$SCRIPT_DIR/pcg-psm.service" /etc/systemd/system/pcg-psm.service
if [[ ! -f /etc/pcg-psm/pcg-psm.conf ]]; then
  install -m 0644 "$SCRIPT_DIR/pcg-psm.conf" /etc/pcg-psm/pcg-psm.conf
else
  echo "Keeping existing /etc/pcg-psm/pcg-psm.conf"
fi

shutdown_line='dtoverlay=gpio-shutdown,gpio_pin=17,active_low=1,gpio_pull=2,debounce=100'
poweroff_line='dtoverlay=gpio-poweroff,gpiopin=22,active_low=0'

cp -a "$CONFIG_TXT" "${CONFIG_TXT}.pcg-psm.bak.$(date +%Y%m%d%H%M%S)"

if ! grep -Fqx "$shutdown_line" "$CONFIG_TXT"; then
  printf '\n# PCG-PSM shutdown request (Uno -> Pi)\n%s\n' "$shutdown_line" >> "$CONFIG_TXT"
fi
if ! grep -Fqx "$poweroff_line" "$CONFIG_TXT"; then
  printf '# PCG-PSM safe-to-cut acknowledgement (Pi -> Uno)\n%s\n' "$poweroff_line" >> "$CONFIG_TXT"
fi

systemctl daemon-reload
systemctl enable pcg-psm.service

echo
echo "PCG-PSM installed."
echo "A reboot is required before the gpio-shutdown/gpio-poweroff overlays take effect."
echo "After reboot run: systemctl status pcg-psm"
echo "Telemetry status: cat /run/pcg-psm/status.json"
