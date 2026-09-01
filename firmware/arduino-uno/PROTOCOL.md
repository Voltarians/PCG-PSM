# PCG-PSM Rev A.2 Serial Protocol

Transport: **115200 baud, 8N1, newline-terminated ASCII**.

The Uno emits one compact JSON status object at `telemetry_interval_ms` (default 1000 ms). Commands are plain text and case-sensitive.

## Status object

Example:

```json
{"type":"status","fw":"A.2.0","ms":123456,"state":"RUN","cause":"NONE","acc":1,"service":0,"heartbeat":1,"vehicle_mv":13842,"main5_mv":5098,"temp_c_x10":417,"current_raw":104,"current_ma":null,"restart_attempts":0,"lv_protect":0,"thermal_protect":0}
```

`temp_c_x10` is tenths of a degree C. `current_ma` is `null` until BTS50060 current calibration is configured.

## Event objects

`EVENTS` prints oldest-to-newest valid records:

```json
{"type":"event","seq":42,"uptime_s":301,"code":"HEARTBEAT_LOST","detail":0,"value":0}
```

The ring holds 32 records.

## Configuration

`CONFIG` prints the current RAM configuration. `SET` updates RAM; `SAVE` stores the CRC-protected configuration in EEPROM.

Supported keys include:

```text
boot_timeout_ms
heartbeat_timeout_ms
ignition_off_delay_ms
shutdown_timeout_ms
power_off_settle_ms
service_timeout_ms
restart_delay_ms
restart_cooldown_ms
stable_run_ms
max_restart_attempts
telemetry_interval_ms
adc_ref_mv
low_voltage_enable
low_warn_mv
low_shutdown_mv
low_recover_mv
low_hold_ms
thermal_enable
thermal_shutdown_c_x10
thermal_recover_c_x10
thermal_hold_ms
current_zero_adc
current_ma_per_count_x1000
```

## Safety policy

`low_voltage_enable=0` and `thermal_enable=0` are the defaults. The firmware must not enforce guessed vehicle thresholds.
