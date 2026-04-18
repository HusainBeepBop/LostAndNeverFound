"""
BLE Proximity Tracker — python_tracker.py
Requires: pip install flet bleak

HOW TO ADD / REMOVE TAGS:
  Edit KNOWN_TAGS below. The key must exactly match TAG_NAME
  in the ESP32 firmware. The value is the display label shown
  in the app.

ZONE CALIBRATION:
  Hold the tag at a known distance and note the dBm value.
  Adjust the thresholds in ZONES to match your environment.
  Thicker walls, metal surfaces, and body-blocking all lower RSSI.
"""

import flet as ft
import asyncio
from bleak import BleakScanner
import time

# ─────────────────────────── CONFIGURATION ───────────────────────────

# Must match TRACKER_SERVICE_UUID in ble_beacon.ino
TRACKER_SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b".lower()

# Known tags whitelist. Key = TAG_NAME in firmware, Value = display label.
# Only devices in this list will be shown. Others are silently ignored.
KNOWN_TAGS = {
    "Aastha-Bag":       "Main Bag",
    "Aastha-Backpack":  "Backpack",
    "Aastha-Case":      "Laptop Case",
    "Aastha-Wallet":    "Wallet",
}

# EMA smoothing: lower α → smoother (slower), higher → noisier (faster)
# 0.15 is a good balance for BLE which typically scans every ~1–2 s
EMA_ALPHA = 0.15

# Seconds of silence before a tag is marked "Signal Lost"
LOST_TIMEOUT = 4.0

# ─────────────────────────── ZONE DEFINITIONS ───────────────────────────
# Each entry: (min_rssi_threshold, display_label, [gradient_start, gradient_end])
# Evaluated top-to-bottom; first match wins.
ZONES = [
    (-52,  "Very Near",   ["#BF360C", "#FF6D00"]),   # Burnt orange → vivid orange
    (-63,  "Near",        ["#1B5E20", "#66BB6A"]),   # Forest → leaf green
    (-72,  "Fairly Near", ["#006064", "#00BCD4"]),   # Deep teal → cyan
    (-81,  "Far",         ["#0D47A1", "#42A5F5"]),   # Navy → sky blue
    (-999, "Very Far",    ["#1A237E", "#311B92"]),   # Deep indigo → deep purple
]

SEARCHING_GRADIENT = ["#102027", "#263238"]          # Near-black blue-grey

# ─────────────────────────── HELPERS ───────────────────────────

def get_zone(rssi: float) -> tuple[str, list[str]]:
    for threshold, label, colors in ZONES:
        if rssi > threshold:
            return label, colors
    return ZONES[-1][1], ZONES[-1][2]


# ─────────────────────────── APP ───────────────────────────

async def main(page: ft.Page):
    page.title = "BLE Tracker"
    page.padding = 0
    page.window.width = 420
    page.window.height = 820
    page.window.resizable = False
    page.bgcolor = "#000000"

    # ── Per-device runtime state ──
    # { tag_name: { "ema": float, "raw": int, "last_seen": float } }
    device_state: dict = {}
    selected = {"name": None}   # mutable box for selected tag name
    update_lock = {"busy": False}  # simple debounce flag

    # ── UI atoms ──
    status_text = ft.Text(
        "SCANNING FOR TAGS...",
        size=12,
        weight=ft.FontWeight.W_700,
        color="#60FFFFFF",
    )

    rssi_text = ft.Text(
        "---",
        size=88,
        weight=ft.FontWeight.W_900,
        color="#FFFFFF",
    )

    rssi_unit = ft.Text(
        "dBm",
        size=18,
        color="#60FFFFFF",
    )

    zone_text = ft.Text(
        "Searching",
        size=30,
        weight=ft.FontWeight.W_700,
        color="#FFFFFF",
    )

    pills_row = ft.Row(
        spacing=8,
        alignment=ft.MainAxisAlignment.CENTER,
        wrap=True,
    )

    gradient_bg = ft.Container(
        expand=True,
        alignment=ft.Alignment(0, 0),
        gradient=ft.LinearGradient(
            begin=ft.Alignment(-1, -1),
            end=ft.Alignment(1, 1),
            colors=SEARCHING_GRADIENT,
        ),
        animate=ft.Animation(900, ft.AnimationCurve.EASE_IN_OUT),
        content=ft.Column(
            horizontal_alignment=ft.CrossAxisAlignment.CENTER,
            alignment=ft.MainAxisAlignment.SPACE_EVENLY,
            controls=[
                ft.Container(height=4),
                status_text,

                # RSSI circle
                ft.Container(
                    width=250,
                    height=250,
                    border_radius=125,
                    border=ft.border.all(1.5, "#28FFFFFF"),
                    bgcolor="#0FFFFFFF",
                    shadow=ft.BoxShadow(
                        spread_radius=1,
                        blur_radius=40,
                        color="#44000000",
                    ),
                    alignment=ft.Alignment(0, 0),
                    content=ft.Column(
                        horizontal_alignment=ft.CrossAxisAlignment.CENTER,
                        alignment=ft.MainAxisAlignment.CENTER,
                        spacing=0,
                        controls=[rssi_text, rssi_unit],
                    ),
                ),

                zone_text,

                # Tag pills
                ft.Container(
                    content=pills_row,
                    padding=ft.padding.symmetric(horizontal=24, vertical=4),
                ),

                ft.Container(height=8),
            ],
        ),
    )

    page.add(gradient_bg)

    # ── UI helpers ──

    def make_pill(name: str, is_active: bool) -> ft.Container:
        label = KNOWN_TAGS.get(name, name)
        return ft.Container(
            content=ft.Text(
                label,
                size=11,
                weight=ft.FontWeight.W_600,
                color="#FFFFFF" if is_active else "#70FFFFFF",
            ),
            bgcolor="#38FFFFFF" if is_active else "#14FFFFFF",
            border=ft.border.all(1, "#55FFFFFF" if is_active else "#20FFFFFF"),
            border_radius=20,
            padding=ft.padding.symmetric(horizontal=14, vertical=7),
            animate=ft.Animation(300, ft.AnimationCurve.EASE_OUT),
            on_click=lambda e, n=name: _select(n),
        )

    def _rebuild_pills():
        pills_row.controls.clear()
        for name in device_state:
            pills_row.controls.append(
                make_pill(name, name == selected["name"])
            )

    def _refresh_display():
        name = selected["name"]

        if name is None or name not in device_state:
            status_text.value = "SCANNING FOR TAGS..."
            rssi_text.value = "---"
            zone_text.value = "Searching"
            gradient_bg.gradient.colors = SEARCHING_GRADIENT
            return

        state = device_state[name]
        lost = (time.time() - state["last_seen"]) > LOST_TIMEOUT
        display = KNOWN_TAGS.get(name, name)

        if lost:
            status_text.value = f"{display.upper()} · SIGNAL LOST"
            rssi_text.value = "---"
            zone_text.value = "Out of Range"
            gradient_bg.gradient.colors = SEARCHING_GRADIENT
        else:
            ema_rounded = int(round(state["ema"]))
            label, colors = get_zone(ema_rounded)
            status_text.value = f"TRACKING · {display.upper()}"
            rssi_text.value = str(ema_rounded)
            zone_text.value = label
            gradient_bg.gradient.colors = colors

    def _full_update():
        _rebuild_pills()
        _refresh_display()
        page.update()
        update_lock["busy"] = False

    def _select(name: str):
        selected["name"] = name
        _full_update()

    # ── BLE callback (runs in Bleak's thread) ──

    def on_device(device, adv_data):
        # Filter by shared service UUID
        uuids = [u.lower() for u in (adv_data.service_uuids or [])]
        if TRACKER_SERVICE_UUID not in uuids:
            return

        # Filter by whitelist
        name = device.name or ""
        if name not in KNOWN_TAGS:
            return

        raw = adv_data.rssi
        now = time.time()

        if name not in device_state:
            # First time seeing this tag — initialise EMA at first reading
            device_state[name] = {"ema": float(raw), "raw": raw, "last_seen": now}
            if selected["name"] is None:
                selected["name"] = name  # auto-select first found tag
        else:
            # Exponential moving average — smooths out RSSI noise
            old_ema = device_state[name]["ema"]
            device_state[name]["ema"] = EMA_ALPHA * raw + (1.0 - EMA_ALPHA) * old_ema
            device_state[name]["raw"] = raw
            device_state[name]["last_seen"] = now

        # Debounce: skip if an update is already queued
        if not update_lock["busy"]:
            update_lock["busy"] = True
            page.run_thread(_full_update)

    # ── Watchdog: catches lost devices even with no new packets ──

    async def watchdog():
        while True:
            await asyncio.sleep(1)
            if not update_lock["busy"]:
                update_lock["busy"] = True
                page.run_thread(_full_update)

    asyncio.create_task(watchdog())

    # ── Start scanner in active mode (needed to receive scan-response/name) ──
    scanner = BleakScanner(detection_callback=on_device, scanning_mode="active")
    await scanner.start()

    while True:
        await asyncio.sleep(1)


if __name__ == "__main__":
    ft.run(main)