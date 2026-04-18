import flet as ft
import asyncio
from bleak import BleakScanner
import time

TARGET_SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b".lower()

# Zones Gradient Colors using Hex Strings instead of ft.colors to avoid version errors
COLD_GRADIENT = ["#311B92", "#0D47A1", "#37474F"] # Deep Purple 900, Blue 900, Blue Grey 800
WARM_GRADIENT = ["#BF360C", "#FF9800"] # Deep Orange 900, Orange 500
HOT_GRADIENT = ["#B71C1C", "#FF1744"] # Red 900, Red Accent 400
SEARCHING_GRADIENT = ["#263238", "#000000"] # Blue Grey 900, Black

async def main(page: ft.Page):
    page.title = "BLE Hot or Cold Tracker"
    page.padding = 0
    # Set window dimensions similar to mobile aspect ratio
    page.window.width = 450
    page.window.height = 800
    page.window.resizable = False

    # Status text element
    status_text = ft.Text("SEARCHING FOR DEVICE...", size=16, weight=ft.FontWeight.BOLD, color="#B3FFFFFF") # WHITE70
    
    # RSSI Circle elements
    rssi_text = ft.Text("---", size=90, weight=ft.FontWeight.W_900, color="#FFFFFF")
    rssi_label = ft.Text("dBm", size=24, color="#B3FFFFFF")
    
    # Zone classification text
    zone_text = ft.Text("Scanning", size=36, weight=ft.FontWeight.BOLD, color="#FFFFFF")

    # Core UI wrapper containing the fluid animating gradient
    gradient_container = ft.Container(
        expand=True,
        alignment=ft.Alignment(0, 0),
        # Set dynamic animated gradient
        gradient=ft.LinearGradient(
            begin=ft.Alignment(-1, -1),
            end=ft.Alignment(1, 1),
            colors=SEARCHING_GRADIENT
        ),
        # Here we instruct Flet to automatically animate our gradient properties!
        animate=ft.Animation(800, ft.AnimationCurve.EASE_IN_OUT),
        
        content=ft.Column(
            horizontal_alignment=ft.CrossAxisAlignment.CENTER,
            alignment=ft.MainAxisAlignment.SPACE_EVENLY,
            controls=[
                status_text,
                ft.Container(
                    width=280,
                    height=280,
                    border_radius=140,
                    border=ft.border.all(3, "#3DFFFFFF"), # WHITE24
                    bgcolor="#1AFFFFFF", # WHITE 10% opacity
                    shadow=ft.BoxShadow(spread_radius=1, blur_radius=15, color="#42000000"), # BLACK26
                    alignment=ft.Alignment(0, 0),
                    content=ft.Column(
                        horizontal_alignment=ft.CrossAxisAlignment.CENTER,
                        alignment=ft.MainAxisAlignment.CENTER,
                        spacing=0,
                        controls=[rssi_text, rssi_label]
                    )
                ),
                zone_text
            ]
        )
    )

    page.add(gradient_container)

    last_seen = time.time()

    def update_ui(rssi, is_active):
        if not is_active:
            status_text.value = "DEVICE LOST OR NOT FOUND"
            rssi_text.value = "---"
            zone_text.value = "Scanning"
            gradient_container.gradient.colors = SEARCHING_GRADIENT
        else:
            status_text.value = "TRACKING DEVICE"
            rssi_text.value = str(rssi)
            
            # Distance approximation mapping
            if rssi < -80:
                zone_text.value = "Freezing"
                gradient_container.gradient.colors = COLD_GRADIENT
            elif -80 <= rssi <= -60:
                zone_text.value = "Getting Warmer"
                gradient_container.gradient.colors = WARM_GRADIENT
            else:
                zone_text.value = "Burning Hot!"
                gradient_container.gradient.colors = HOT_GRADIENT
                
        page.update()

    def detection_callback(device, advertisement_data):
        nonlocal last_seen
        # Check against emitted service UUIDs
        uuids = advertisement_data.service_uuids
        if TARGET_SERVICE_UUID in [u.lower() for u in uuids]:
            last_seen = time.time()
            # Push safely to Flet's UI thread
            page.run_thread(update_ui, advertisement_data.rssi, True)
            
    # Background watchdog to clear data if device vanishes
    async def watchdog():
        while True:
            if time.time() - last_seen > 3.0:
                page.run_thread(update_ui, None, False)
            await asyncio.sleep(1)

    asyncio.create_task(watchdog())

    scanner = BleakScanner(detection_callback)
    await scanner.start()
    
    # Keeps async execution open while bleeding events to Bleak and Flet
    while True:
        await asyncio.sleep(1)

if __name__ == "__main__":
    ft.run(main)
