# Generació dels instal·ladors

Aquests scripts només empaqueten els binaris ja compilats; no instal·len ni
executen el plugin. La versió prové de `buildspec.json`.

En un Mac amb les eines de desenvolupament Apple i Homebrew:

```sh
brew install makensis sevenzip
python3 packaging/build-installers.py
python3 packaging/verify-installers.py
```

Cal tenir prèviament el DLL x64 a
`dist/windows/obs-plugins/64bit/obs-control-deck.dll`, el paquet universal a
`dist/macos/obs-control-deck.plugin` i les traduccions a `data/locale`.
Aquests són els binaris de release, no els de Debug.

El resultat és un EXE NSIS, un PKG productbuild, la guia `Botonera-LLEGEIX-ME.txt`
i els hashes de tots els ZIP, EXE i PKG a `dist/SHA256SUMS.txt`.

## Windows

L'assistent en català és un bootstrapper NSIS Unicode de 32 bits que instal·la
exclusivament el **DLL x64**. Valida Windows 10 o posterior i arquitectura
Intel/AMD x64; demana elevació per instal·lar per a tots els usuaris.
`SetShellVarContext all` fa que `$APPDATA` de NSIS sigui ProgramData. La destinació
és fixa; no s'accepta una redirecció amb `/D`.

Estructura actual d'OBS 33:

```text
C:\ProgramData\obs-studio\plugins\obs-control-deck\
  obs-control-deck.dll
  data\locale\ca-ES.ini
  data\locale\en-US.ini
  LLEGEIX-ME.txt
  LICENSE.txt
  Desinstal·lar Botonera.exe
```

L'assistent comprova `obs64.exe` amb tasklist i ofereix reintentar quan l'usuari
l'hagi tancat; mai mata el procés. La mateixa comprovació s'aplica en desinstal·lar.
Només s'eliminen els fitxers propis i les carpetes buides; no s'accedeix a perfils,
col·leccions, imatges integrades ni dreceres de l'usuari.

La ubicació està destinada a OBS instal·lat de manera normal. OBS portable ignora
ProgramData i necessita una distribució diferent. No s'ha executat el DLL ni
l'assistent en Windows: la verificació extreu el paquet amb 7-Zip, comprova la
capçalera PE AMD64 i compara els fitxers amb els binaris i traduccions d'origen.
Les dependències importades són OBS, la seva API de frontend, Qt6 i el runtime
MSVC/UCRT que necessita la distribució normal d'OBS. No s'empaqueten Qt ni OBS.

## macOS

El PKG només habilita el domini `CurrentUserHomeDirectory`; el seu payload
`Library/Application Support/obs-studio/plugins/obs-control-deck.plugin` es resol
sota el directori de l'usuari, no sota `/Library` del sistema. L'assistent demana
tancar `com.obsproject.obs-studio`. El bundle no és relocatable; una actualització
substitueix només el paquet del plugin, que no conté la configuració de les escenes.

La comprovació amb `installer -dominfo` i `-showChoicesXML` no instal·la res.
També s'expandeix el PKG, es comparen tots els fitxers amb l'original i es verifica
la signatura ad hoc del bundle. El PKG en si no està signat ni notaritzat.

Fonts de les rutes:
[OBS: ubicacions i estructura actuals](https://obsproject.com/kb/legacy-plugin-developer-info),
[guia de plugins](https://obsproject.com/kb/plugins-guide).

## Primera prova real

Segueix la guia que acompanya els paquets amb una col·lecció de proves. Comprova
la càrrega, persistència, números, imatges, reordenació, transicions i tres cicles
DSK mostrar/amagar. No cal començar cap transmissió. Si falla, cal el registre
que es pot desar des d'Ajuda → Fitxers de registre i les versions de Windows/OBS.
La compilació, extracció i comparació de fitxers no acrediten aquesta prova nativa.
