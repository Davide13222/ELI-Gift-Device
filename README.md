# ELI 💕

Un cadou personal — robot/stand cu ecran OLED, printat 3D, controlat de pe telefon prin WiFi captive portal.

Stand printat 3D (roz) cu ESP32-S2 + display OLED SSD1306. Stă în priză pe birou și arată ceas + animații romantice. Persoana căreia îi e dăruit își alege ce apare pe ecran direct din browserul telefonului — fără nicio aplicație instalată.

## Ce face

- 🕐 **Ceas** — oră + dată, sincronizate automat de pe telefon la conectare
- 💖 **Inimi zburătoare** — animație simplă pe OLED
- 💔 **Love Me Not** — animație cu versuri, frame-by-frame
- 🎵 **No One Noticed** — animație cu versuri, frame-by-frame
- 💌 **Mesaj personalizat** — scrii orice text și apare pe ecran
- 🎲 **Surprize automate** — la interval aleatoriu (5/10/15/20/25/30 min) rulează o animație random
- 📱 **Captive portal** — pagina web se deschide automat la conectarea pe WiFi (ca rețelele din mall/aeroport)

## Hardware

| Componentă | Detalii |
|---|---|
| Microcontroler | ESP32-S2 (testat pe modul tip Mini, 4MB flash) |
| Display | OLED SSD1306 128×64, I2C |
| Carcasă | Printată 3D, design tip "stand triunghiular" |

### Wiring

| OLED | ESP32-S2 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 8 |
| SCL | GPIO 9 |

> Pinii I2C sunt setați explicit în cod cu `Wire.begin(8, 9)`. Dacă folosești altă placă ESP32, ajustează-i în `setup()`.

## Librării necesare (Arduino IDE → Library Manager)

- `Adafruit SSD1306`
- `Adafruit GFX Library`
- Placă: pachetul **ESP32 by Espressif Systems** (Boards Manager)

## Structura proiectului

```
ELI_captive_portal/
├── ELI_captive_portal.ino   # codul principal (WiFi, server, ceas, UI)
├── loveMeNot_frames.h        # 102 cadre bitmap — animația "Love Me Not"
└── noOne_frames.h            # 199 cadre bitmap — animația "No One Noticed"
```

⚠️ Toate fișierele trebuie să fie **în același folder**, iar folderul trebuie să aibă **exact același nume** ca fișierul `.ino` (cerință Arduino IDE).

## Cum funcționează

1. La pornire, ESP32 creează propriul Access Point WiFi (nu are nevoie de internet).
2. Telefonul se conectează la rețeaua `ELI`.
3. Datorită DNS-ului captiv, pagina web se deschide **automat** (sau manual la `192.168.4.1`).
4. La încărcarea paginii, ora/data curentă a telefonului se trimite automat către ESP32.
5. Din pagină se alege animația sau se trimite un mesaj text — totul prin cereri HTTP simple (`/set?mode=...`).

## Configurare înainte de upload

În `ELI_captive_portal.ino`, secțiunea de sus:

```cpp
const char* AP_SSID = "ELI";
const char* AP_PASS = "iloveeli";   // minim 8 caractere
```

Schimbă-le dacă vrei alt nume/parolă de rețea.

## Memorie / limitări

Cele două animații bitmap ocupă împreună aproximativ 1.9MB în flash. Pe o placă cu 4MB acestea încap alături de codul WiFi + server, dar pe un ESP8266 clasic (~1MB) ar trebui eliminată una dintre ele sau rulate ca proiecte separate.

## Credit

Animațiile cu versuri ("Love Me Not", "No One Noticed") sunt adaptate din proiecte open-source de animație pe OLED stil lyrics-video.

---

Făcut cu drag, pentru o singură persoană. 🩷
