# Validació · 4 de novembre de 2026

## Compilacions

- macOS: plugin universal arm64 + x86_64, compilat amb Xcode. Signatura ad hoc comprovada.
- Windows: DLL PE32+ x86-64, compilada i enllaçada amb clang-cl/lld, Qt 6.8.3 i l'SDK OBS 31.1.1.
  Exportacions de mòdul comprovades. Compilació creuada al Mac; encara sense execució en un PC.
- No s'ha executat el binari x86_64 en un Mac Intel.

## Proves automàtiques

Catorze grups de proves Qt, amb esdeveniments de ratolí i arrossegament:

1. Exportació/importació amb imatges i presets.
2. Rebuig atòmic de formats desconeguts, identificadors duplicats i imatges incorrectes.
3. Selecció d'escenes en mode estudi i presets que no executen AUTO.
4. Bloqueig d'accions en edició, durant canvis de col·lecció i transicions.
5. Distribució de caselles després d'afegir elements incrementalment.
6. Imatge vertical arrossegada, integrada sense executar cap acció i disponible després d'esborrar l'original.
7. Assignació d'elements des d'un panell d'escenes/DSK, caselles fixes, reordenació, bloqueig persistent.
8. Arrossegament vertical d'AUTO per ajustar la durada sense executar una transició; botó per a cada tipus disponible.
9. Graella 4 × 3 / 3 × 4, amplada del dock independent, configuració pel menú contextual i migració del format antic.

El backend de les proves automàtiques és simulat. Aquestes proves no substitueixen
les proves de les API d'OBS ni d'un dispositiu real. La maqueta de `deck-preview`
utilitza els mateixos widgets del plugin amb aquest backend de demostració.

## Proves dins d'OBS al Mac

El plugin s'ha carregat a OBS Studio 33.0.1 amb la col·lecció de proves autoritzada.
S'han afegit i desat botons d'escena i s'ha inspeccionat visualment el panell real.
S'han observat canvis de previ/programa i controls de transició en aquesta sessió.

El DSK antic instal·lat fallava amb `Symbol not found: _obs_frontend_add_dock`.
S'ha conservat una còpia a `build-local/backup-before-testing` i s'ha substituït
per Downstream Keyer 0.4.5, de la publicació oficial d'Exeldro. Ara carrega a OBS
33.0.1, mostra el canal DSK 1 i l'element de prova 02. La botonera consulta
correctament aquests canals amb l'API local d'obs-websocket.

Les comprovacions manuals finals de dreceres, arrossegament entre panells i
activació/retirada DSK es documenten a continuació a mesura que es completen.
No s'han fet proves d'emissió ni d'enregistrament.

## Revisió externa

Claude Code ha respost a una comprovació real de connexió i ha revisat el codi
sense modificar fitxers (`claude-review.txt`). S'ha reforçat la conservació de dades
que no es poden llegir i el bloqueig d'un segon CUT mentre hi ha restauració pendent.

Dues observacions de la revisió no es corresponen amb el contracte real de DSK:
`dsk_select_scene` retorna explícitament `success`, i buidar un canal es fa amb
una cadena buida a `scene`. S'ha contrastat amb el codi oficial de Downstream Keyer;
s'ha corregit el comentari imprecís, mantenint el protocol correcte.

## Comprovació final pendent

Després de carregar l’últim binari, OBS ha mostrat un avís modal natiu. El control
remot retorna `timeoutReached` en llegir la finestra i la captura. Una mostra del
procés confirma que el fil principal està dins `NSAlert runModal`, atenent esdeveniments,
i no acredita un bloqueig del plugin. S’ha demanat el text de l’avís a l’usuari.
Fins a resoldre’l no es dona per completada la prova manual final de dreceres,
arrossegament entre panells, DSK i CUT. Les compilacions i les vuit proves Qt passen.

Claude també ha revisat el tractament final dels arrossegaments i els bloqueigs
(`claude-final-review.txt`). S’han reforçat la detecció del primer canal DSK
corresponent a la vista d’origen i la supressió del clic després d’arrossegar
encara que el callback d’ajust no estigui definit. Les dreceres també es bloquegen
mentre hi ha oberta una finestra modal de configuració de la botonera.

## Iteració de configuració contextual

La barra superior s'ha retirat. Clic dret per editar, definir columnes × files i mida,
importar/exportar, configurar presets o entrar en realització. En realització s'amaguen
els presets i les eines; el menú contextual permet tornar a configurar.
Les nou proves passen, inclosa la configuració a través del menú i del diàleg reals.
S'han revisat les captures del widget de demostració en configuració i realització.
L'usuari ha tancat l'avís d'altres plugins i s'ha verificat la nova interfície carregada
al dock d'OBS. L'usuari ha confirmat que es veu bé. Això no amplia la validació de DSK,
CUT o dreceres, que conserven els límits de les proves manuals anteriors.

## Adaptació automàtica de mida

Casella «Adaptar la mida» al costat de la mida en píxels. Desa l'opció amb la
col·lecció i l'exportació; les configuracions antigues mantenen la mida manual.
La mida és el mínim entre l'espai per columna i per fila, descomptant el marc.
Es recalcula quan canvia el viewport, sense modificar la mida manual ni les accions.
Deu grups de proves passen: configuració real de la casella, persistència,
finestres amples/estretes, quadrats, graella completa dins el viewport i retorn a mida manual.
Captura del mateix widget de demostració revisada: `botonera-adaptada.png`.
Compilats Mac universal i Windows x64; aquesta iteració no s'ha executat dins d'OBS
ni en Windows. Instal·lada al Mac per carregar-la en el pròxim reinici d'OBS.

## Dreceres dins l'editor i previsualització de graella

Onze grups Qt superats. La prova nova obre l'editor real i comprova el candau,
canvi de drecera, persistència, avís de coincidència amb un altre botó, conservació
de dreceres addicionals i cancel·lació. Es verifica la conversió de modificadors,
tecles de funció, navegació i teclat numèric al format simbòlic d'OBS.
La configuració de graella previsualitza canvis sense desar al backend i cancel·lar
restaura els valors anteriors. Revisats visualment `editor-drecera.png` i
`graella-adaptacio.png` amb els mateixos widgets i backend simulat.
Compilacions macOS universal i Windows x64 correctes. La validació Windows continua
limitada a compilació. La prova de pulsació global de la drecera dins d'OBS continua pendent.

Després de reiniciar OBS i tancar l'avís d'altres plugins, s'ha verificat que la
nova versió es carrega i la graella configurada es mostra al dock. El control remot
no ha aconseguit obrir el menú contextual natiu amb clic dret; la inspecció visual
de l'editor i els tests d'edició corresponen als mateixos widgets en el backend de
proves. No es declara completada la prova manual de captura i execució global d'una drecera.

## Correcció: presets en realització

Els presets es mantenen visibles en entrar en realització. El bloqueig protegeix
la configuració, però permet seleccionar presets i executar AUTO/CUT. La prova
de mode estudi ara entra en realització, comprova que el preset és visible i actiu,
el selecciona sense executar AUTO i verifica després la transició.

## Tipografia Montserrat

La botonera i els seus editors prefereixen Montserrat quan Qt la detecta instal·lada.
Si no està disponible, es conserva la font heretada d'OBS. Es mantenen les mides i
els pesos existents. La demostració confirma `QFontInfo: Montserrat` en aquest Mac;
revisada la captura `botonera-tipografia.png`. Compilacions Mac universal i Windows
x64 correctes. No s'ha reiniciat OBS per aquest canvi de tipografia.

## Correcció del clic dret i coherència dels modes

El mode edició es derivava d'un botó ocult independent del bloqueig persistent:
existia un estat intermedi que permetia operar i editar alhora. Ara el bloqueig
és la font única del mode i el botó intern d'edició només el reflecteix. La capçalera
mostra sempre el mode actual; carregar una col·lecció sincronitza també el peu.
Les caselles buides no mostren controls d'edició en realització.

La graella tracta explícitament pressió i alliberament del clic dret (i Control+clic
a Mac), obre el menú després de l'alliberament i consumeix l'esdeveniment contextual
natiu del ratolí per evitar un segon menú. El clic dret no executa cap escena.
El menú de realització ofereix tornar a configurar; el de configuració permet
editar un botó i tornar a realització.

Dotze grups Qt passen. La regressió nova envia esdeveniments de ratolí al viewport,
comprova el menú real, obre l'editor, canvia de mode, executa una escena en realització
i verifica els dos modes després de recarregar. També cobreix Control+clic al Mac.
Això corregeix la manca de les proves anteriors, que invocaven directament el senyal
contextual. Compilació Mac universal i Windows x64 correctes. Windows no executat.
Captura del widget de realització revisada. La prova en OBS s'ha intentat després
de reiniciar; continua pendent mentre l'avís d'altres plugins impedeix el control remot.

La correcció es distribueix com a **0.1.1**; el registre d’OBS indica `Loaded 0.1.1`
quan la carrega, per distingir-la dels binaris anteriors.

## Text sobre les imatges sense bandes

Retirades les franges fosques superior i inferior dels botons. Les etiquetes són
blanques amb ombra negra al voltant de les lletres; els indicadors de previ i directe
conserven verd i vermell. La imatge queda visible sota tot el text. Revisada la
captura del widget `botonera-sense-bandes.png`. Compilat per a Mac universal i
Windows x64; aquesta iteració visual no s'ha carregat dins d'OBS.

## Càrrega real de 0.1.1 i inspecció visual posterior

El registre de la sessió d'OBS iniciada a les 10:40:56 confirma `Loaded 0.1.1`.
S'ha cancel·lat el diàleg de fitxers absents sense modificar les fonts. A la finestra
real s'han vist el text sense bandes i les capçaleres REALITZACIÓ/CONFIGURACIÓ;
les transicions i el preset existent s'habiliten en realització i es deshabiliten
en configuració. L'usuari estava interactuant durant la inspecció; els canvis de
mode observats no s'atribueixen a l'automatització. El control remot no ha aconseguit
reproduir el gest de clic dret (inclòs un error de coordenades `windowNotFoundAtPosition`).
S'ha demanat a l'usuari la comprovació manual del menú. Aquesta part continua pendent.

## Controls de transició personalitzables · 0.2.0

AUTO, CUT, els selectors de tipus i els presets comparteixen un editor contextual
amb imatge integrada i drecera protegida amb candau. També admeten arrossegar imatges
en configuració. Les accions en directe s'inhibeixen durant l'edició, tot i mantenir
el control accessible per al clic dret. Les imatges mantenen la proporció i el text
sobreposat continua llegible amb ombra negra.

Els controls tenen identificadors reservats i els presets disposen d'un UUID estable:
canviar-ne el nom conserva la personalització i la drecera. S'afegeixen a les dreceres
natives d'OBS, amb el mateix callback protegit que les escenes. Els controls de tipus
no disponibles temporalment conserven les seves assignacions. AUTO/CUT no s'executen
fora del mode estudi ni mentre hi ha una transició ocupada o un editor modal obert.

Tretze grups Qt superats. La prova nova cobreix arrossegament d'imatge, captura real
de Return al camp Qt, Shift+Return, bloqueig del camp, desar/cancel·lar, importació,
rebuig d'imatges incorrectes, identitat del preset i selecció versus execució d'AUTO/CUT.
Backend simulat: la captura i l'execució global a través d'OBS no formen part d'aquesta prova.
Revisades les captures `editor-control-transicio.png` i `controls-personalitzats.png`.

La versió 0.2.0 s'ha carregat a OBS al Mac després de reiniciar, amb AUTO i CUT
visibles en realització. S'ha cancel·lat l'avís de fitxers absents sense canviar les
fonts i s'ha tornat al dock. El control remot no permet navegar de manera fiable
als gestos contextuals ni a la llista lateral de Configuració; no es declara provada
la pulsació global d'Enter/Shift+Enter en OBS. L'editor real Qt i l'encaminament de
les accions sí que estan coberts pel backend de proves. Windows continua sense prova d'execució.
L'usuari ja havia confirmat manualment que el clic dret de la graella funciona a 0.1.1.

## Entrada automàtica a realització · 0.2.1

S'escolten exclusivament STREAMING_STARTING i RECORDING_STARTING. Cadascun posa
la botonera en realització immediatament i desa l'estat de manera diferida per
no tornar a entrar al desat d'OBS durant el callback. No s'aplica des del sondeig
ni a STARTED: l'usuari pot tornar a configurar després de l'inici sense que el
programa el torni a bloquejar. Aturar o reprendre una pausa no canvia el mode.
Els editors modals es cancel·len i els menús oberts es tanquen quan arriba l'inici.
Cancel·lar una previsualització de graella conserva el nou mode de realització.

Catorze grups Qt superats. La prova nova verifica el canvi inicial, la possibilitat
de desbloquejar després, un segon inici i l'inici amb una previsualització modal
oberta, amb restauració de la graella sense restaurar el mode d'edició. No es llança
cap transmissió ni enregistrament real durant aquestes proves. Compilat per a
macOS universal i Windows x64; verificació dels senyals reals d'OBS pendent.

## Prioritat de Montserrat davant del tema d’OBS · 0.2.2

El tema Yami instal·lat imposa Open Sans mitjançant una regla global QWidget.
Aquesta regla prevalia sobre setFont(Montserrat), de manera que la comprovació
anterior sense el tema del host no detectava el problema. La botonera ara aplica
una regla local de font-family quan Montserrat està disponible, limitada al dock
i als seus descendents, inclosos els editors. La resta d’OBS conserva el seu tema.

Quinze grups de proves Qt superats. La regressió imposa Helvetica globalment i
comprova Montserrat al dock, a tots els controls descendents i a un diàleg, alhora
que un widget extern conserva la lletra del host. Revisada la captura
`montserrat-host-theme.png`. Instal·lada al Mac; cal reiniciar OBS per carregar-la.
La comprovació visual correspon al programa de previsualització amb tema simulat,
no a una nova sessió d’OBS. Windows compilat, sense prova d’execució.

## Bloc AUTO/CUT i semàntica DSK · 0.2.3

El títol TRANSICIÓ és fora dels controls. AUTO groc i CUT vermell són dues
meitats de 72 px, separades per 3 px, amb radi de 8 px només a les cantonades
exteriors. Les imatges personalitzades també respecten aquesta silueta. Es
conserven identificadors, imatges, dreceres i l’ajust vertical de durada d’AUTO.

El DSK ja alternava entre nom de composició i cadena buida via dsk_select_scene.
S’ha contrastat amb el codi oficial 0.4.5: change_scene → SwitchDSK → SwitchToScene
canvia la selecció; apply_selected_source/apply_source aplica la transició del DSK,
les variants show/hide i les sobreescriptures. No es modifica la transició del
complement ni s’envia cap ordre AUTO/CUT per mostrar o amagar una composició.
Referències: https://github.com/exeldro/obs-downstream-keyer/blob/0.4.5/downstream-keyer-dock.cpp
i https://github.com/exeldro/obs-downstream-keyer/blob/0.4.5/downstream-keyer.cpp.
L’editor ara diu «activar / desactivar», explica l’efecte i els botons mostren
ACTIU/INACTIU amb ajuda «Clic per amagar/mostrar». Un canal continua tenint una
sola composició seleccionada; triar-ne una altra substitueix l’anterior.

Setze grups Qt superats. La prova nova verifica la separació, les dimensions i
l’alternança DSK sense canviar previ/programa ni disparar transicions principals.
Revisades les captures de previsualització sense imatge i amb imatges als controls.
La prova DSK usa backend simulat; l’animació real al complement no s’ha tornat a
provar dins d’OBS. macOS universal i Windows x64 compilats; Windows sense execució.
Instal·lació macOS feta al disc, pendent de reiniciar OBS.

## Números, retroil·luminació, transició compacta i correcció DSK · 0.2.4

Número de posició sempre visible, també sobre imatges, DSK i caselles buides,
amb Montserrat ExtraBold (800) o el pes equivalent de la font disponible. La
tipografia s’ajusta a l’amplada i alçada que deixen les llegendes; sota 44 px
interiors es prioritza el número. Les etiquetes s’escurcen amb punts suspensius.
Dues opcions persistents a Graella i mida permeten amagar etiquetes i textos
d’estat per separat, amb previsualització, desat i restauració en cancel·lar.
Els JSON anteriors mantenen totes dues opcions visibles.

El gris predeterminat passa a neutre (#a4a8ad), també en representar l’antic
#374151; es conserven colors personalitzats i imatges. Els números i etiquetes
són gris fosc en repòs i gairebé negres encesos, amb relleu clar discret. Sobre
imatges s’afegeix un halo clar fi per conservar llegibilitat. Llum radial blanca
groguenca en composició Plus sota els textos: contínua a programa/DSK actiu,
pulsació suau de 1,2 segons al previ. Marcs vermell i verd fixos. El temporitzador
repinta només la casella PREVI i s’atura visualment quan el dock no és visible.

D’acord amb la darrera indicació, un únic botó groc TRANSICIÓ, sense títol
superior ni CUT visible. Les dreceres CUT existents continuen funcionant.
Selectors foscos de 36 × 36 px, abreviatura de tres lletres, nom accessible i
ajuda complets. Alçada del contenidor reservada i rail amb desplaçament si
no hi cap, per evitar l’encavalcament observat en el dock natiu amb el tema OBS.

La fallada DSK del segon clic era la lectura de `scene` a get_downstream_keyers:
la resposta prové de Save i utilitza currentItem, que és el focus. SwitchToScene
pot canviar selectedItems sense canviar aquest focus. Es consulta dsk_get_scene
per a cada canal; és la selecció que el complement aplica. La mateixa lectura
s’utilitza per als indicadors i per decidir nom de composició o cadena buida.
No s’accepta un valor de focus com a alternativa si falla la consulta.
Codi contrastat: downstream-keyer.cpp (Save, GetScene, SwitchToScene) i
downstream-keyer-dock.cpp (get_downstream_keyers, get_scene), versió oficial 0.4.5.

Divuit grups de proves Qt superats. La regressió de protocol executa 18 clics
alternant mostrar/amagar amb focus buit, antic i d’una altra composició, més
selecció externa, buidatge explícit i error de consulta. La prova de dibuix cobreix
números de tres dígits i botons de 48, 80, 112 i 200 px, amb imatge i sense;
comprova variació de llum PREVI, estabilitat de programa i un cicle real del
temporitzador. També previsualització/cancel·lació/desat de les opcions. Revisades
les captures botons-petits-illuminats.png i botons-imatges-sense-text.png.

Compilació macOS universal i Windows x64, signatura ad hoc Mac verificada.
Carregada una compilació 0.2.4 a OBS per observar els controls i els números.
El clic natiu sobre la casella DSK no s’ha pogut reproduir amb el control remot
(windowNotFoundAtPosition); el protocol es verifica amb una API simulada, no es
declara completada la prova de mostrar/amagar real ni l’animació DSK en OBS.
Windows continua sense prova d’execució. El darrer ajust de gris i tinta necessita
recarregar el mòdul en reiniciar OBS.

## Instal·ladors · 0.2.4

Generats Botonera-0.2.4-Windows-x64-Setup.exe (NSIS Unicode) i
Botonera-0.2.4-macOS-universal.pkg (pkgbuild/productbuild). La compilació NSIS
s'ha completat sense avisos. El bootstrapper és x86 i comprova arquitectura
AMD64 abans d'instal·lar el DLL x64; Windows ARM64 i 32 bits queden rebutjats.
El Windows instal·lat normalment usa el format nou d'OBS 33: DLL directament a
ProgramData/obs-studio/plugins/obs-control-deck, amb data al costat. Inclou
comprovació d'OBS tancat, entrada de desinstal·lació i guia de primera prova.
No es mata OBS ni s'eliminen perfils o col·leccions en instal·lar/desinstal·lar.

El PKG limita el domini a CurrentUserHomeDirectory. `installer -dominfo` retorna
només aquest domini i `-showChoicesXML` accepta la selecció sense instal·lar res.
El payload conté Library/Application Support/... sota el destí de l'usuari;
la distribució demana tancar com.obsproject.obs-studio. Bundle no relocatable.

`python3 packaging/verify-installers.py` ha extret tots dos artefactes. Comparats
SHA-256 del DLL, traduccions i tots els fitxers del bundle Mac amb l'origen;
capçalera PE 0x8664 del DLL confirmada. Comprovats guia, llicència i desinstal·lador
Windows. Signatura ad hoc del bundle Mac extret correcta i versió 0.2.4 al plist.
`pkgbuild --analyze` emet quatre missatges «write: Permission denied» però retorna
0; l'expansió i la comparació byte a byte del payload final han estat correctes.
No hi ha metadades AppleDouble al PKG final.

Els instal·ladors no tenen signatura comercial ni notarització. No s'han executat
els passos d'instal·lació/desinstal·lació ni el plugin Windows en un PC; la
validació del paquet no acredita aquesta primera prova real. La guia distribuïda
ho indica i proposa la càrrega, persistència, transició i tres cicles DSK, sense
necessitat de transmetre. No s'ha publicat ni enviat cap artefacte a tercers.


## 0.2.6 — panell de transicions i numeració (4 de novembre de 2026)

Build universal macOS i CTest correctes (inclou disposició a quatre mides,
selecció de tipus i temps sense executar, tall sense temps, inicials segons
tipus intern d’OBS, persistència de l’amplada i etiquetes de drecera en edició).
Signatura ad hoc verificada després de copiar fora d’iCloud. Plugin instal·lat
i carregat a OBS 33.0.1 amb la col·lecció existent; còpia prèvia a /private/tmp.

Inspecció visual dins d’OBS: una columna dreta, selectors C/F/M/S repartits
en amplada, graella de mil·lisegons, TRANSICIÓ al centre vertical, presets
F100/M400 al peu. Sense superposicions entre seccions ni botons retallats.
Números 01–18 grossos, amb el contorn real dels glifs centrat respecte del botó
sencer. Etiquetes Q/W/E/R/Z i Retorn observades en edició abans de la darrera
actualització; les captures de demostració finals també inclouen etiquetes.
Les seleccions del programa observades durant la sessió eren de l’usuari;
no s’ha iniciat cap transmissió, enregistrament ni transició per provar-ho.

Claude Code ha llegit i revisat les captures finals de demostració petita en
edició i gran en realització. No detecta retalls de controls ni superposicions,
i confirma centrat i contrast. Única observació lleu: una etiqueta descriptiva
llarga d’un botó d’escena apareix abreujada amb punts suspensius a mida petita
(comportament deliberat d’elision; no afecta números ni controls).

Això valida el grafisme observat i les proves amb backend simulat. No acredita
proves en Windows ni una sessió completa de producció/DSK. Els instal·ladors
distribuïts de 0.2.4 no s’han regenerat en aquesta sessió.


## Paquets per enviar — 0.2.6

Recompilació macOS universal (arm64 i x86_64) i Windows x64 Release finalitzada.
Generats Botonera-0.2.6-macOS-universal.pkg i
Botonera-0.2.6-Windows-x64-Setup.exe. Verificació dels instal·ladors superada:
extracció, hashes dels binaris i locales, DLL PE AMD64, desinstal·lador, guia,
domini current-user del PKG, versió 0.2.6 i signatura ad hoc del bundle extret.
Els missatges «write: Permission denied» de pkgbuild --analyze no han impedit
la generació; extracció i comparació de tot el payload correctes.
També s’ha generat el ZIP del bundle Mac i verificat el seu contingut.

La carpeta dist/Botonera 0.2.6 - Per enviar conté només els dos instal·ladors,
el ZIP Mac, la guia i els SHA-256 de la versió actual. Els instal·ladors no
s’han executat; Windows continua pendent de prova nativa. No tenen signatura
comercial ni notarització. La 0.2.6 del plugin ja s’havia carregat i revisat
dins d’OBS al Mac durant la sessió de desenvolupament.


## 0.2.7 — Transició fixa i instal·lador Windows 32/33

El botó principal ja no viu dins d’un QScrollArea. La columna reserva tres bandes; només els selectors superiors i els presets inferiors tenen desplaçament propi. Es redueix l’alçada mínima de la columna perquè s’adapti als acobladors baixos sense retallar tota la columna. Els selectors mantenen l’amplada mínima compacta.

Regressió: acoblador de 600 × 340, nou tipus de transició i vint presets addicionals. Es comprova l’amplada visible dels selectors, es desplacen totes dues zones, es dona focus a l’últim preset i s’alterna l’estat ocupat abans de sis clics reals simulats a TRANSICIÓ. El botó queda sencer i manté la posició. Suite Qt superada amb estil natiu i Fusion a escala 150%.

Compilació macOS universal i Windows x64. Instal·ladors extrets i verificats: tots dos DLL de Windows són idèntics i corresponen al binari compilat; les traduccions, la guia i la versió del paquet macOS són correctes. OBS 32 escaneja bin/64bit; OBS 33 prioritza el DLL de l’arrel i descarta el duplicat legacy després de carregar-lo. S’eliminen totes dues còpies en desinstal·lar. No és un instal·lador per a OBS portable.

La captura de revisió es genera amb deck-preview, no és una captura d’OBS Windows. Aquesta correcció 0.2.7 encara no s’ha executat en un PC; les fotos de l’usuari documenten el problema anterior en Windows/OBS 32.2.2.
