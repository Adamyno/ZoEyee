import urllib.request
from PIL import Image, ImageOps

# Trying alternative URL
url = "https://img.icons8.com/ios-filled/256/wifi-logo.png"
req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
with urllib.request.urlopen(req) as response:
    with open("wifi.png", "wb") as f:
        f.write(response.read())

img = Image.open("wifi.png").convert("RGBA")
bg = Image.new("RGBA", img.size, (255, 255, 255, 255))
bg.paste(img, mask=img)
img = bg.convert("L")

target_width = 32
aspect = img.height / img.width
target_height = int(target_width * aspect)

img = img.resize((target_width, target_height), Image.Resampling.LANCZOS)
# It's a black icon on white background. To draw white on black, we need the icon bits to be 1 where the icon is.
# So we invert the image.
img = ImageOps.invert(img)

threshold = 80
img = img.point(lambda p: 255 if p > threshold else 0, mode="1")

width, height = img.size
bytes_per_row = (width + 7) // 8

xbm = f"const unsigned char icon_wifi_menu_width = {width};\n"
xbm += f"const unsigned char icon_wifi_menu_height = {height};\n"
xbm += f"const unsigned char icon_wifi_menu_bits[] PROGMEM = {{\n  "

pixels = img.load()
byte_count = 0

for y in range(height):
    for x_byte in range(bytes_per_row):
        byte_val = 0
        for bit in range(8):
            x = x_byte * 8 + bit
            if x < width:
                if pixels[x, y] != 0:
                    byte_val |= (1 << bit)
        
        xbm += f"0x{byte_val:02X}, "
        byte_count += 1
        if byte_count % 12 == 0:
            xbm += "\n  "
            
xbm += "\n};\n"
with open("wifi_xbm.h", "w") as f:
    f.write(xbm)
