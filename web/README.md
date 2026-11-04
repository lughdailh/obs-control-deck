# Web de Botonera

Pàgina estàtica en català, anglès i castellà, preparada per a `https://emprenyat.cat/obs/botonera/`.

## Publicació al domini

Descomprimeix `dist/Botonera-0.2.6-web-emprenyat.zip` i puja el contingut de `obs/botonera/` a la carpeta homònima del servidor. No cal Node, PHP ni cap base de dades. Els instal·ladors es descarreguen de la release pública de GitHub v0.2.6.

La web està publicada a https://emprenyat.cat/obs/botonera/ i el projecte Botonera apareix a la portada. Instal·lada des del gestor `/admin/`, amb carpeta base `obs`, carpeta `botonera` i etiqueta «Plugin d’OBS». La previsualització de Sites continua sent privada i independent del domini.

Per actualitzar-la al gestor, edita el projecte Botonera i puja un ZIP amb `index.html` a l’arrel, com `dist/Botonera-0.2.6-web-admin.zip`. El ZIP per FTP té una estructura diferent. Conserva la icona quadrada a la portada; la capçalera de la pàgina utilitza `hero-machine.png` amb transparència.

El ZIP també conté `integracio-portada/`: un fragment HTML amb les mateixes classes que les targetes de projectes de la portada i una icona de 400 píxels d’alçada. Copia la imatge a `assets/img/projectes/` de la portada i insereix el fragment al lloc que prefereixis. No substitueixis la portada sencera.

## Edició i previsualització

Els textos i la plantilla són a `tools/build_pages.py`; després de modificar-los, executa `python3 web/tools/build_pages.py` des de l’arrel del repositori. CSS i JS s’editen directament. Serveix `web/` amb un servidor estàtic, per exemple `python3 -m http.server 8766 --directory web`.

Les fonts Montserrat i Cookie es carreguen des de Google Fonts, com a les pàgines de referència; hi ha tipografies alternatives si no hi ha connexió.

## Verificació

Revisió visual d’escriptori i mòbil a 390 × 844, canvi CA/EN/ES, pestanyes amb ratolí i fletxes, ampliació i Escape, DSK en ambdós sentits, pausa de l’animació i desplegable de presets. Captures i animació provenen dels widgets Qt del plugin amb backend de demostració; no constitueixen una prova nova d’OBS en producció ni de Windows.
