# Captures de la interfície

Les captures PNG i el vídeo MP4 s’han fet amb `deck-preview`, que utilitza els widgets Qt reals de Botonera i un backend de demostració. Les escenes, les dreceres i les dades són exemples, no un directe real.

- `realitzacio.png`: mode realització, graella, previ/directe, DSK i transicions.
- `configuracio.png`: mode configuració, imatges i dreceres.
- `editor.png`: editor real d’un botó.
- `dsk-inactiu.png` / `dsk-actiu.png`: el mateix botó amb dos estats de demostració.
- `retroilluminacio.mp4`: 30 fotogrames reals del pintat Qt a intervals de 40 ms; el número del previ pulsa mentre el directe manté la llum.

El suport de captura s’ha ampliat només a `tests/deck-preview.cpp`; no s’ha modificat el comportament del plugin per fer la web.

## Reproducció

Compila l’objectiu `deck-preview` de `build-local` en configuració `RelWithDebInfo`. A macOS, defineix `DYLD_FRAMEWORK_PATH=/Applications/OBS.app/Contents/Frameworks` i `QT_QPA_PLATFORM_PLUGIN_PATH=/Applications/OBS.app/Contents/PlugIns/platforms`.

Exemples des de l’arrel del repositori:

```
build-local/RelWithDebInfo/deck-preview --website --locked --plain --pulse-high --capture web/images/realitzacio.png
build-local/RelWithDebInfo/deck-preview --website --artwork web/images/app-icon.png --capture web/images/configuracio.png
build-local/RelWithDebInfo/deck-preview --website --editor --capture web/images/editor.png
```

Les captures estan etiquetades com a demostració a la web. La icona és il·lustrativa i se’n documenta el prompt a `ICONA.md`.
