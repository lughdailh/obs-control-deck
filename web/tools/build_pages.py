#!/usr/bin/env python3
"""Build the three static language pages. No runtime build or server is required."""
from pathlib import Path
import json
from html import escape as esc
ROOT=Path(__file__).resolve().parents[1]
REPO='https://github.com/lughdailh/obs-control-deck'
RELEASE=REPO+'/releases/tag/v0.2.6'
ASSETS=REPO+'/releases/download/v0.2.6/'
D={
'ca':{
 'title':'Botonera — El directe no espera que trobis el botó. Plugin per a OBS Studio',
 'description':'Escenes, gràfics DSK, transicions i dreceres en una botonera visual dins d’OBS Studio. Personalitza les tecles i realitza des de Mac o Windows. Versió de proves 0.2.6.',
 'skip':'Ves al contingut','nav':['Què fa','Com funciona','Per a qui és','Descarrega'],'beta':'0.2.6 · Versió de proves',
 'eyebrow':'Plugin per a OBS Studio · Mac i Windows','headline':'El directe no espera que trobis <em>el botó.</em>',
 'hero':'L’escena, el gràfic, la transició… tot passa alhora. Posa els controls que fas servir en una botonera feta a la teva mida. I deixa de perseguir finestres.',
 'cta':'Descarrega la botonera','watch':'Mira com funciona','note':'Programari lliure · Sense maquinari addicional','art':'Una petita taula de realització. Dins d’OBS.',
 'intro':'<strong>Botonera és un plugin natiu per a OBS Studio.</strong> Reuneix les escenes que tries, els gràfics del DSK i les transicions en un únic acoblador visual. El prepares al teu gust i passes a realització.',
 'compat':['Fet per treballar amb OBS','macOS universal · Windows x64'],
 'features_kicker':'Què fa','features_title':'Els controls que necessites.<br>On els esperes trobar.',
 'features_lead':'No cal posar-hi totes les escenes. Tria les que fas servir i dona a cada tecla una imatge, una etiqueta i una drecera.',
 'features':[
 ('▦','La teva graella','Decideix columnes i files. Ajusta la mida o deixa que els botons s’adaptin a l’espai, sempre quadrats.'),
 ('↧','Una imatge, un gest','Arrossega una imatge sobre una tecla. Queda integrada a la botonera, sense preparar-ne les dimensions.'),
 ('⌨','També amb el teclat','Assigna una drecera a cada botó. En configuració veus les combinacions que has preparat.'),
 ('◉','Previ i directe, d’un cop d’ull','Llum càlida en directe, pulsació al previ i els marcs vermell i verd. El número sempre hi és.'),
 ('⇄','Transicions a mà','Tria el tipus, els mil·lisegons o un preset. Després prem TRANSICIÓ per passar a programa.'),
 ('▣','Configura. I realitza.','Reordena arrossegant, edita amb clic dret i entra en realització per protegir la distribució.')],
 'gallery_kicker':'La interfície','gallery_title':'Això és el que tindràs davant.',
 'gallery_lead':'La mateixa botonera en tres moments: quan estàs treballant, quan la prepares i quan personalitzes una tecla.',
 'tabs':['En realització','En configuració','Edita un botó'],
 'captions':[
 'Escenes i elements seleccionats a l’esquerra. Tipus de transició, temps ràpids i presets a la dreta. En mode estudi, tries el previ i després executes la transició.',
 'Les dreceres es veuen sobre les tecles. Pots arrossegar per ordenar, ajustar el separador i fer clic dret per editar. Les accions queden aturades mentre configures.',
 'Tria l’acció, la imatge, el color i la drecera. El candau del camp de teclat evita canviar la combinació sense voler.'],
 'alts':['Botonera en realització amb escenes, gràfics i panell de transicions','Botonera en configuració amb imatges i etiquetes de dreceres','Editor real d’un botó amb acció, imatge i drecera de teclat'],
 'zoom':'Amplia la captura','fit':'Ajusta a pantalla','zoom_actual':'Mida real','close':'Tanca','capture_note':'Captures de la interfície Qt del plugin, en català i amb dades de demostració.',
 'workflow_kicker':'Flux d’ús','workflow_title':'Primer la prepares.<br>Després, a realitzar.',
 'steps':[
 ('Obre la botonera','A OBS, ves a Acobladors → Botonera i posa-la on et vagi millor.'),
 ('Fes-la teva','Tria la graella, assigna escenes i DSK, afegeix imatges i dreceres. Arrossega per ordenar.'),
 ('Entra en realització','El clic dret et permet passar de configuració a realització i protegir els botons.'),
 ('Prepara i passa a programa','En mode estudi, selecciona l’escena de previ. Tria la transició i el temps, o un preset, i prem TRANSICIÓ.')],
 'light_labels':['EN DIRECTE · Llum fixa','PREVI · Llum pulsant'],
 'light_caption':'Retroil·luminació capturada del plugin. Els marcs mantenen el seu color mentre la llum del previ puja i baixa.',
 'play':'Reprodueix la pulsació','pause':'Atura la pulsació',
 'dsk_kicker':'Downstream Keyer','dsk_title':'El gràfic entra.<br>El gràfic surt.',
 'dsk_text':'Un botó per al logotip, el rètol del convidat o el marcador. Un clic el mostra i el següent l’amaga, amb l’efecte configurat al Downstream Keyer.',
 'dsk_points':['Els gràfics actuen sobre la sortida en directe.','Cada canal DSK manté una sola composició seleccionada.','Tria una altra composició del mateix canal i substitueix l’anterior.'],
 'dsk_more':'Cal tenir instal·lat <a href="https://obsproject.com/forum/resources/downstream-keyer.1254/">Downstream Keyer d’Exeldro</a>. La botonera en controla la selecció; no substitueix el complement.',
 'dsk_show':'Mostra el gràfic','dsk_hide':'Amaga el gràfic','dsk_note':'Exemple interactiu fet amb captures. Aquesta pàgina no controla el teu OBS.',
 'dsk_alt_off':'Botó DSK amb el gràfic inactiu','dsk_alt_on':'El mateix botó DSK amb el gràfic actiu i il·luminat',
 'audience_kicker':'Per a qui és','audience_title':'Per a qui realitza.<br>I ja té prou feina.',
 'audience_lead':'Si fas de càmera, tècnic de so i realitzador alhora, buscar un control no hauria de ser una quarta feina.',
 'audiences':[
 ('Multicàmera','Canvia de pla tenint clar què és al previ i què està sortint en directe.'),
 ('Podcasts i entrevistes','Tingues a mà els plans, els vídeos i els rètols dels convidats.'),
 ('Directes i actes','Prepara les escenes que toca fer servir, amb transicions i gràfics al mateix lloc.'),
 ('Equips petits','Organitza el control amb el ratolí o el teclat, dins del mateix OBS.')],
 'quote':'«El convidat ja parla. Tu encara busques l’escena.»',
 'download_kicker':'Descarrega','download_title':'Posa ordre al teu OBS.',
 'download_lead':'Versió 0.2.6 de proves. Tanca OBS, instal·la el plugin i torna’l a obrir. El trobaràs a Acobladors → Botonera.',
 'windows':'Windows 10/11 · Intel/AMD x64 · OBS 33','mac':'Apple Silicon i Intel · Instal·lador PKG',
 'download_note':'<strong>Estat de la versió:</strong> macOS revisat durant el desenvolupament. Windows compilat i paquets verificats, pendent de la primera execució en un PC. Els instal·ladors no tenen signatura comercial ni notarització Apple.',
 'guide':'Guia d’instal·lació','releases':'Totes les versions','source':'Codi i documentació',
 'faq_title':'Abans de començar',
 'faqs':[
 ('Necessito una botonera física?','No. És una graella de botons dins d’OBS. La controles amb el ratolí o les dreceres del teclat. La màquina de la icona és la il·lustració del projecte.'),
 ('Què fa exactament un preset?','Guarda un tipus de transició i una durada. Quan el prems, prepara aquests valors; després prems TRANSICIÓ per executar el canvi. El preset no fa el pas a programa tot sol.'),
 ('Es desa la configuració?','Sí. La graella, els botons, les imatges i les dreceres es desen amb la col·lecció d’escenes. També pots exportar i importar la botonera amb les imatges integrades.'),
 ('Puc editar durant una gravació?','Quan comença una gravació o transmissió, la botonera entra en realització. Després pots tornar manualment a configuració per canviar alguna cosa.'),
 ('Què necessito per als botons DSK?','Downstream Keyer instal·lat i les composicions afegides als seus canals. Amb TIE actiu, la botonera bloqueja l’acció d’aquell canal. DSK és independent del previ i de la transició principal.')],
 'footer_project':'Un projecte de Moiz i Bartra Produccions, SL & El Català Emprenyat.',
 'footer_credit':'Software ideat i creat íntegrament a Catalunya. Desenvolupament assistit per OpenAI Codex/ChatGPT, sota direcció i revisió humanes.',
 'footer_back':'Tots els projectes','support':'Dona’ns suport amb PayPal','coffee':'Necessito un cafè','antifa':'Programari català i antifeixista'
},
'en':{
 'title':'Botonera — Your live show won’t wait for you to find the button. OBS Studio plugin',
 'description':'Scenes, DSK graphics, transitions and shortcuts in one visual control deck inside OBS Studio. Customise your buttons on Mac or Windows. Preview release 0.2.6.',
 'skip':'Skip to content','nav':['Features','How it works','Who it’s for','Download'],'beta':'0.2.6 · Preview release',
 'eyebrow':'OBS Studio plugin · Mac and Windows','headline':'Your live show won’t wait for you to find <em>the button.</em>',
 'hero':'The next scene, a graphic, a transition… everything happens at once. Put the controls you actually use on a deck of your own. Stop chasing windows.',
 'cta':'Download Botonera','watch':'See how it works','note':'Free software · No extra hardware','art':'A small production switcher. Inside OBS.',
 'intro':'<strong>Botonera is a native OBS Studio plugin.</strong> It brings your chosen scenes, DSK graphics and transitions together in one visual dock. Set it up your way, then switch to production mode.',
 'compat':['Built to work inside OBS','Universal macOS · Windows x64'],
 'features_kicker':'Features','features_title':'The controls you need.<br>Where you expect them.',
 'features_lead':'You don’t have to include every scene. Pick the ones you use and give each button an image, a label and a shortcut.',
 'features':[
 ('▦','Your own grid','Choose columns and rows. Set the size or let the buttons fit the available space, always square.'),
 ('↧','An image. One gesture.','Drop an image onto a button. It becomes part of the deck without preparing its dimensions.'),
 ('⌨','Use your keyboard too','Assign a shortcut to each button. Configuration mode shows the combinations you have set.'),
 ('◉','Preview and program at a glance','A steady warm light for program, a pulse for preview, and red and green frames. The number is always there.'),
 ('⇄','Transitions within reach','Pick a type, duration or preset. Then press TRANSICIÓ to send preview to program.'),
 ('▣','Configure. Then produce.','Drag to reorder, right-click to edit, then enter production mode to protect the layout.')],
 'gallery_kicker':'The interface','gallery_title':'This is what you’ll be working with.',
 'gallery_lead':'The same deck at three different moments: running the show, setting it up and customising a button.',
 'tabs':['Production mode','Configuration mode','Edit a button'],
 'captions':[
 'Your selected scenes and graphics on the left. Transition types, quick timings and presets on the right. In studio mode, choose preview and then run the transition.',
 'Keyboard shortcuts appear on the buttons. Drag to reorder, adjust the divider and right-click to edit. Operational actions are suspended while you configure.',
 'Choose the action, image, colour and shortcut. A lock beside the keyboard field prevents accidental changes to a binding.'],
 'alts':['Botonera in production mode with scene buttons, graphics and transition controls','Botonera in configuration mode with images and shortcut badges','Actual button editor with action, image and keyboard shortcut fields'],
 'zoom':'Enlarge screenshot','fit':'Fit to screen','zoom_actual':'Actual size','close':'Close','capture_note':'Screenshots of the plugin’s actual Qt interface, in Catalan, with demonstration data.',
 'workflow_kicker':'Workflow','workflow_title':'Set it up first.<br>Then run the show.',
 'steps':[
 ('Open the deck','In OBS, go to Docks → Botonera and place it where it suits you.'),
 ('Make it yours','Choose the grid, assign scenes and DSK graphics, then add images and shortcuts. Drag to reorder.'),
 ('Enter production mode','Use the right-click menu to switch from configuration to production and protect the buttons.'),
 ('Prepare, then take to program','In studio mode, select your preview scene. Pick a transition and duration, or a preset, then press TRANSICIÓ.')],
 'light_labels':['PROGRAM · Steady light','PREVIEW · Pulsing light'],
 'light_caption':'Backlighting recorded from the plugin. The frames keep their colours while preview illumination rises and falls.',
 'play':'Play the pulse','pause':'Pause the pulse',
 'dsk_kicker':'Downstream Keyer','dsk_title':'Graphic on.<br>Graphic off.',
 'dsk_text':'A button for your logo, guest lower third or scoreboard. One click shows it, the next hides it, using the effect configured in Downstream Keyer.',
 'dsk_points':['Graphics act directly on the output.','Each DSK channel keeps one composition selected.','Choose a different composition on the same channel to replace the previous one.'],
 'dsk_more':'Requires <a href="https://obsproject.com/forum/resources/downstream-keyer.1254/">Downstream Keyer by Exeldro</a>. Botonera controls its selection; it doesn’t replace the plugin.',
 'dsk_show':'Show graphic','dsk_hide':'Hide graphic','dsk_note':'Interactive example made from screenshots. This page does not control your OBS.',
 'dsk_alt_off':'DSK button with the graphic inactive','dsk_alt_on':'The same DSK button with the graphic active and illuminated',
 'audience_kicker':'Who it’s for','audience_title':'For the person running the show.<br>You have enough to do.',
 'audience_lead':'If you are operating the camera, mixing sound and directing at the same time, finding a control shouldn’t be a fourth job.',
 'audiences':[
 ('Multicamera production','Change shots while keeping track of what’s in preview and what’s live.'),
 ('Podcasts and interviews','Keep your shots, clips and guest graphics close at hand.'),
 ('Live streams and events','Prepare the scenes you’ll need, with transitions and graphics in the same place.'),
 ('Small teams','Organise control with your mouse or keyboard, inside OBS itself.')],
 'quote':'“The guest is already talking. You’re still looking for the scene.”',
 'download_kicker':'Download','download_title':'Bring order to your OBS.',
 'download_lead':'Preview release 0.2.6. Close OBS, install the plugin and reopen it. You’ll find it under Docks → Botonera.',
 'windows':'Windows 10/11 · Intel/AMD x64 · OBS 33','mac':'Apple Silicon and Intel · PKG installer',
 'download_note':'<strong>Release status:</strong> macOS reviewed during development. Windows has been built and its packages verified, but the first execution on a PC is still pending. Installers have no commercial publisher signature or Apple notarisation.',
 'guide':'Installation guide','releases':'All releases','source':'Source and documentation',
 'faq_title':'Before you start',
 'faqs':[
 ('Do I need a hardware control deck?','No. It is a grid of buttons inside OBS, controlled with your mouse or keyboard shortcuts. The machine in the icon is the project illustration.'),
 ('What exactly does a preset do?','It stores a transition type and duration. Pressing it prepares those values; you then press TRANSICIÓ to run the change. A preset does not take preview to program by itself.'),
 ('Is the configuration saved?','Yes. The grid, buttons, images and shortcuts are saved with your scene collection. You can also export and import the deck with embedded images.'),
 ('Can I edit during a recording?','When recording or streaming starts, the deck enters production mode. You can then manually return to configuration to make a change.'),
 ('What do the DSK buttons need?','Downstream Keyer must be installed, with compositions added to its channels. If TIE is active, Botonera blocks that channel’s action. DSK is independent of preview and the main transition.')],
 'footer_project':'A project by Moiz i Bartra Produccions, SL & El Català Emprenyat.',
 'footer_credit':'Software conceived and created entirely in Catalonia. Development assisted by OpenAI Codex/ChatGPT, under human direction and review.',
 'footer_back':'All projects','support':'Support us with PayPal','coffee':'I need a coffee','antifa':'Catalan and antifascist software'
},
'es':{
 'title':'Botonera — El directo no espera a que encuentres el botón. Plugin para OBS Studio',
 'description':'Escenas, gráficos DSK, transiciones y atajos en una botonera visual dentro de OBS Studio. Personaliza las teclas en Mac o Windows. Versión de prueba 0.2.6.',
 'skip':'Ir al contenido','nav':['Qué hace','Cómo funciona','Para quién es','Descarga'],'beta':'0.2.6 · Versión de prueba',
 'eyebrow':'Plugin para OBS Studio · Mac y Windows','headline':'El directo no espera a que encuentres <em>el botón.</em>',
 'hero':'La escena, el gráfico, la transición… todo pasa a la vez. Pon los controles que utilizas en una botonera a tu medida. Y deja de perseguir ventanas.',
 'cta':'Descarga la botonera','watch':'Mira cómo funciona','note':'Software libre · Sin hardware adicional','art':'Una pequeña mesa de realización. Dentro de OBS.',
 'intro':'<strong>Botonera es un plugin nativo para OBS Studio.</strong> Reúne las escenas que eliges, los gráficos del DSK y las transiciones en un único panel visual. Lo preparas a tu gusto y pasas a realización.',
 'compat':['Hecho para trabajar dentro de OBS','macOS universal · Windows x64'],
 'features_kicker':'Qué hace','features_title':'Los controles que necesitas.<br>Donde esperas encontrarlos.',
 'features_lead':'No hace falta incluir todas las escenas. Elige las que utilizas y dale a cada tecla una imagen, una etiqueta y un atajo.',
 'features':[
 ('▦','Tu propia cuadrícula','Decide columnas y filas. Ajusta el tamaño o deja que los botones se adapten al espacio, siempre cuadrados.'),
 ('↧','Una imagen, un gesto','Arrastra una imagen sobre una tecla. Queda integrada en la botonera, sin preparar sus dimensiones.'),
 ('⌨','También con el teclado','Asigna un atajo a cada botón. En configuración ves las combinaciones que has preparado.'),
 ('◉','Previo y directo, de un vistazo','Luz cálida en directo, pulsación en previo y los marcos rojo y verde. El número siempre está visible.'),
 ('⇄','Transiciones a mano','Elige el tipo, los milisegundos o un preset. Después pulsa TRANSICIÓ para pasar a programa.'),
 ('▣','Configura. Y realiza.','Reordena arrastrando, edita con clic derecho y entra en realización para proteger la distribución.')],
 'gallery_kicker':'La interfaz','gallery_title':'Esto es lo que tendrás delante.',
 'gallery_lead':'La misma botonera en tres momentos: cuando trabajas, cuando la preparas y cuando personalizas una tecla.',
 'tabs':['En realización','En configuración','Edita un botón'],
 'captions':[
 'Escenas y elementos seleccionados a la izquierda. Tipos de transición, tiempos rápidos y presets a la derecha. En modo estudio, eliges el previo y después ejecutas la transición.',
 'Los atajos aparecen sobre las teclas. Puedes arrastrar para ordenar, ajustar el separador y editar con clic derecho. Las acciones quedan suspendidas mientras configuras.',
 'Elige la acción, la imagen, el color y el atajo. El candado del campo de teclado evita cambiar la combinación sin querer.'],
 'alts':['Botonera en realización con escenas, gráficos y panel de transiciones','Botonera en configuración con imágenes y etiquetas de atajos','Editor real de un botón con acción, imagen y atajo de teclado'],
 'zoom':'Amplía la captura','fit':'Ajusta a pantalla','zoom_actual':'Tamaño real','close':'Cierra','capture_note':'Capturas de la interfaz Qt del plugin, en catalán y con datos de demostración.',
 'workflow_kicker':'Flujo de uso','workflow_title':'Primero la preparas.<br>Después, a realizar.',
 'steps':[
 ('Abre la botonera','En OBS, ve a Paneles (Docks) → Botonera y colócala donde te vaya mejor.'),
 ('Hazla tuya','Elige la cuadrícula, asigna escenas y DSK, añade imágenes y atajos. Arrastra para ordenar.'),
 ('Entra en realización','El clic derecho permite pasar de configuración a realización y proteger los botones.'),
 ('Prepara y pasa a programa','En modo estudio, selecciona la escena de previo. Elige transición y tiempo, o un preset, y pulsa TRANSICIÓ.')],
 'light_labels':['EN DIRECTO · Luz fija','PREVIO · Luz pulsante'],
 'light_caption':'Retroiluminación capturada del plugin. Los marcos mantienen su color mientras la luz del previo sube y baja.',
 'play':'Reproduce la pulsación','pause':'Detén la pulsación',
 'dsk_kicker':'Downstream Keyer','dsk_title':'El gráfico entra.<br>El gráfico sale.',
 'dsk_text':'Un botón para el logotipo, el rótulo del invitado o el marcador. Un clic lo muestra y el siguiente lo oculta, con el efecto configurado en Downstream Keyer.',
 'dsk_points':['Los gráficos actúan sobre la salida en directo.','Cada canal DSK mantiene una sola composición seleccionada.','Elige otra composición del mismo canal para sustituir la anterior.'],
 'dsk_more':'Necesitas <a href="https://obsproject.com/forum/resources/downstream-keyer.1254/">Downstream Keyer de Exeldro</a>. La botonera controla su selección; no sustituye al complemento.',
 'dsk_show':'Muestra el gráfico','dsk_hide':'Oculta el gráfico','dsk_note':'Ejemplo interactivo hecho con capturas. Esta página no controla tu OBS.',
 'dsk_alt_off':'Botón DSK con el gráfico inactivo','dsk_alt_on':'El mismo botón DSK con el gráfico activo e iluminado',
 'audience_kicker':'Para quién es','audience_title':'Para quien realiza.<br>Y ya tiene bastante trabajo.',
 'audience_lead':'Si haces de cámara, técnico de sonido y realizador a la vez, buscar un control no debería ser un cuarto trabajo.',
 'audiences':[
 ('Multicámara','Cambia de plano teniendo claro qué está en previo y qué sale en directo.'),
 ('Podcasts y entrevistas','Ten a mano los planos, los vídeos y los rótulos de los invitados.'),
 ('Directos y actos','Prepara las escenas que vas a utilizar, con transiciones y gráficos en el mismo sitio.'),
 ('Equipos pequeños','Organiza el control con ratón o teclado, dentro del propio OBS.')],
 'quote':'«El invitado ya habla. Tú todavía buscas la escena.»',
 'download_kicker':'Descarga','download_title':'Pon orden en tu OBS.',
 'download_lead':'Versión de prueba 0.2.6. Cierra OBS, instala el plugin y vuelve a abrirlo. Lo encontrarás en Paneles (Docks) → Botonera.',
 'windows':'Windows 10/11 · Intel/AMD x64 · OBS 33','mac':'Apple Silicon e Intel · Instalador PKG',
 'download_note':'<strong>Estado de la versión:</strong> macOS revisado durante el desarrollo. Windows compilado y paquetes verificados, pendiente de la primera ejecución en un PC. Los instaladores no tienen firma comercial ni notarización de Apple.',
 'guide':'Guía de instalación','releases':'Todas las versiones','source':'Código y documentación',
 'faq_title':'Antes de empezar',
 'faqs':[
 ('¿Necesito una botonera física?','No. Es una cuadrícula de botones dentro de OBS, controlada con el ratón o los atajos de teclado. La máquina del icono es la ilustración del proyecto.'),
 ('¿Qué hace exactamente un preset?','Guarda un tipo de transición y una duración. Al pulsarlo, prepara esos valores; después pulsas TRANSICIÓ para ejecutar el cambio. El preset no pasa a programa por sí solo.'),
 ('¿Se guarda la configuración?','Sí. La cuadrícula, los botones, las imágenes y los atajos se guardan con la colección de escenas. También puedes exportar e importar la botonera con las imágenes integradas.'),
 ('¿Puedo editar durante una grabación?','Cuando comienza una grabación o transmisión, la botonera entra en realización. Después puedes volver manualmente a configuración para cambiar algo.'),
 ('¿Qué necesito para los botones DSK?','Downstream Keyer instalado y las composiciones añadidas a sus canales. Con TIE activo, la botonera bloquea la acción de ese canal. DSK es independiente del previo y de la transición principal.')],
 'footer_project':'Un proyecto de Moiz i Bartra Produccions, SL & El Català Emprenyat.',
 'footer_credit':'Software ideado y creado íntegramente en Cataluña. Desarrollo asistido por OpenAI Codex/ChatGPT, bajo dirección y revisión humanas.',
 'footer_back':'Todos los proyectos','support':'Apóyanos con PayPal','coffee':'Necesito un café','antifa':'Software catalán y antifascista'
}}


def render(lang,t):
    prefix='' if lang=='ca' else '../'
    img=prefix+'images/'
    def e(text): return esc(text,quote=True)
    links={'ca':('index.html' if lang=='ca' else '../index.html'),'en':('en/index.html' if lang=='ca' else '../en/index.html'),'es':('es/index.html' if lang=='ca' else '../es/index.html')}
    languages=''.join(f'<a href="{url}" hreflang="{code}" lang="{code}"'+(' aria-current="page"' if code==lang else '')+f'>{code.upper()}</a>' for code,url in links.items())
    nav=''.join(f'<a href="#{id}" class="{ "nav-cta" if i==3 else "optional" if i==2 else ""}">{e(label)}</a>' for i,(id,label) in enumerate(zip(['que-fa','funcionament','per-a-qui','descarrega'],t['nav'])))
    features=''.join(f'<article class="feature"><span class="feature-icon" aria-hidden="true">{icon}</span><h3>{e(title)}</h3><p>{e(text)}</p></article>' for icon,title,text in t['features'])
    tabs=''.join(f'<button id="tab-{i}" type="button" role="tab" aria-controls="gallery-panel" aria-selected="{str(i==0).lower()}" tabindex="{0 if i==0 else -1}" data-tab="{i}">{e(label)}</button>' for i,label in enumerate(t['tabs']))
    steps=''.join(f'<li><span class="step-num" aria-hidden="true">0{i+1}</span><div><h3>{e(title)}</h3><p>{e(text)}</p></div></li>' for i,(title,text) in enumerate(t['steps']))
    audiences=''.join(f'<article><span class="number" aria-hidden="true">0{i+1}</span><h3>{e(title)}</h3><p>{e(text)}</p></article>' for i,(title,text) in enumerate(t['audiences']))
    faqs=''.join(f'<details><summary>{e(q)}</summary><p>{e(a)}</p></details>' for q,a in t['faqs'])
    config={'images':[img+x for x in ['realitzacio.png','configuracio.png','editor.png']], 'captions':t['captions'],'alts':t['alts'],'tabs':t['tabs'],'dskImages':[img+'dsk-inactiu.png',img+'dsk-actiu.png'],'dskLabels':[t['dsk_show'],t['dsk_hide']],'dskAlts':[t['dsk_alt_off'],t['dsk_alt_on']],'play':t['play'],'pause':t['pause'],'fit':t['fit'],'actual':t['zoom_actual']}
    canonical='https://emprenyat.cat/obs/botonera/'+('' if lang=='ca' else lang+'/')
    return f'''<!doctype html>
<html lang="{lang}"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>{e(t['title'])}</title><meta name="description" content="{e(t['description'])}">
<link rel="canonical" href="{canonical}"><link rel="alternate" hreflang="ca" href="https://emprenyat.cat/obs/botonera/"><link rel="alternate" hreflang="en" href="https://emprenyat.cat/obs/botonera/en/"><link rel="alternate" hreflang="es" href="https://emprenyat.cat/obs/botonera/es/">
<meta property="og:type" content="website"><meta property="og:title" content="{e(t['title'])}"><meta property="og:description" content="{e(t['description'])}"><meta property="og:image" content="https://emprenyat.cat/obs/botonera/images/app-icon.png"><meta property="og:url" content="{canonical}">
<meta name="theme-color" content="#0b0d10"><link rel="icon" type="image/png" href="{img}favicon.png">
<link rel="preconnect" href="https://fonts.googleapis.com"><link rel="preconnect" href="https://fonts.gstatic.com" crossorigin><link href="https://fonts.googleapis.com/css2?family=Montserrat:wght@400;500;600;700;800&family=Cookie&display=swap" rel="stylesheet">
<link rel="stylesheet" href="{prefix}style.css"><script src="{prefix}script.js" defer></script></head>
<body><a class="skip" href="#main">{e(t['skip'])}</a>
<header class="site-header"><div class="wrap header-inner"><a class="brand" href="#top"><img src="{img}app-icon.webp" alt="" width="64" height="64"><span>Botonera<small>{e(t['beta'])}</small></span></a><nav class="main-nav" aria-label="{e({'ca':'Navegació principal','en':'Main navigation','es':'Navegación principal'}[lang])}">{nav}<span class="langs">{languages}</span></nav></div></header>
<main id="main"><section id="top" class="hero"><div class="wrap hero-inner"><div><div class="eyebrow">{e(t['eyebrow'])}</div><h1>{t['headline']}</h1><p>{e(t['hero'])}</p><div class="actions"><a class="btn btn-primary" href="#descarrega">{e(t['cta'])}<span aria-hidden="true">↓</span></a><a class="btn btn-quiet" href="#funcionament">{e(t['watch'])}<span aria-hidden="true">↗</span></a></div><p class="hero-note">{e(t['note'])}</p></div><figure class="hero-art"><img src="{img}hero-machine.png" width="1254" height="1254" alt="Botonera" fetchpriority="high"><figcaption>{e(t['art'])}</figcaption></figure></div></section>
<div class="intro-strip"><div class="wrap"><p>{t['intro']}</p><div class="compat"><b>{e(t['compat'][0])}</b>{e(t['compat'][1])}</div></div></div>
<section class="section" id="que-fa"><div class="wrap"><p class="section-kicker">{e(t['features_kicker'])}</p><h2>{t['features_title']}</h2><p class="section-lead">{e(t['features_lead'])}</p><div class="features">{features}</div></div></section>
<section class="section section-alt" id="funcionament"><div class="wrap"><p class="section-kicker">{e(t['gallery_kicker'])}</p><h2>{e(t['gallery_title'])}</h2><p class="section-lead">{e(t['gallery_lead'])}</p><div class="tabs" role="tablist" aria-label="{e(t['gallery_kicker'])}">{tabs}</div><div id="gallery-panel" role="tabpanel" aria-labelledby="tab-0" tabindex="0"><div class="shot-frame"><div class="shot-topbar"><span><i aria-hidden="true"></i>Botonera · OBS Studio</span><span class="optional">v0.2.6</span></div><button class="shot-button" id="open-shot" type="button" aria-label="{e(t['zoom'])}"><img id="gallery-image" src="{img}realitzacio.png" alt="{e(t['alts'][0])}" width="1840" height="1300" loading="lazy"><span class="zoom-label">{e(t['zoom'])} ↗</span></button></div><p class="shot-caption" id="gallery-caption">{e(t['captions'][0])}</p></div><p class="capture-note">{e(t['capture_note'])}</p></div></section>
<section class="section" id="flux"><div class="wrap split"><div><p class="section-kicker">{e(t['workflow_kicker'])}</p><h2>{t['workflow_title']}</h2><ol class="steps">{steps}</ol></div><figure class="light-example"><video id="light-video" src="{img}retroilluminacio.mp4" poster="{img}llums-poster.png" loop muted playsinline preload="metadata" aria-label="{e(t['light_caption'])}"></video><div class="light-labels"><span>{e(t['light_labels'][0])}</span><span>{e(t['light_labels'][1])}</span></div><figcaption>{e(t['light_caption'])}</figcaption><button class="motion-toggle" id="toggle-motion" type="button">{e(t['play'])}</button></figure></div></section>
<section class="section section-alt" id="dsk"><div class="wrap split"><div class="dsk-copy"><p class="section-kicker">{e(t['dsk_kicker'])}</p><h2>{t['dsk_title']}</h2><p>{e(t['dsk_text'])}</p><ul class="check-list">{''.join('<li>'+e(x)+'</li>' for x in t['dsk_points'])}</ul><p class="capture-note">{t['dsk_more']}</p></div><div class="dsk-demo"><img id="dsk-image" src="{img}dsk-inactiu.png" width="330" height="330" alt="{e(t['dsk_alt_off'])}" loading="lazy"><button class="btn btn-primary" id="toggle-dsk" type="button" aria-pressed="false">{e(t['dsk_show'])}</button><p class="demo-label">{e(t['dsk_note'])}</p><noscript><img src="{img}dsk-actiu.png" alt="{e(t['dsk_alt_on'])}"></noscript></div></div></section>
<section class="section" id="per-a-qui"><div class="wrap"><p class="section-kicker">{e(t['audience_kicker'])}</p><h2>{t['audience_title']}</h2><p class="section-lead">{e(t['audience_lead'])}</p><p class="quote">{e(t['quote'])}</p><div class="audiences">{audiences}</div></div></section>
<section class="section section-alt" id="descarrega"><div class="wrap"><p class="section-kicker">{e(t['download_kicker'])}</p><h2>{e(t['download_title'])}</h2><p class="section-lead">{e(t['download_lead'])}</p><div class="download-row"><a class="download-card" href="{ASSETS}Botonera-0.2.6-Windows-x64-Setup.exe"><span class="os-icon" aria-hidden="true"><i></i><i></i><i></i><i></i></span><div><h3>Windows</h3><small>{e(t['windows'])}</small></div><span class="download-arrow" aria-hidden="true">↓</span></a><a class="download-card" href="{ASSETS}Botonera-0.2.6-macOS-universal.pkg"><span class="mac-icon" aria-hidden="true">⌘</span><div><h3>macOS</h3><small>{e(t['mac'])}</small></div><span class="download-arrow" aria-hidden="true">↓</span></a></div><p class="beta-note">{t['download_note']}</p><div class="download-links"><a href="{ASSETS}Botonera-LLEGEIX-ME.txt">{e(t['guide'])} ↗</a><a href="{REPO}/releases">{e(t['releases'])} ↗</a><a href="{REPO}">{e(t['source'])} ↗</a></div></div></section>
<section class="section"><div class="wrap faq"><h2>{e(t['faq_title'])}</h2>{faqs}</div></section></main>
<footer class="site-footer"><div class="wrap"><div class="footer-top"><strong>Botonera · OBS Studio</strong><a href="https://emprenyat.cat/">{e(t['footer_back'])} ↗</a></div><div class="credits"><p><strong>{e(t['footer_project'])}</strong></p><p>{e(t['footer_credit'])}</p><p><a href="{REPO}">{e(t['source'])}</a> · <a href="{REPO}/blob/main/LICENSE">GPL-2.0-or-later</a> · v0.2.6</p></div><div class="footer-bottom"><img src="{img}antifa-cat.svg" class="antifa" alt="{e(t['antifa'])}" loading="lazy"><div class="support"><a href="https://www.paypal.com/ncp/payment/PLB-LY4PW9EJNVML" target="_blank" rel="noopener noreferrer">{e(t['support'])} ↗</a><a href="https://www.buymeacoffee.com/totalamandanga" target="_blank" rel="noopener noreferrer">{e(t['coffee'])} ↗</a></div><a href="https://emprenyat.cat/"><img src="{img}ece-banda.svg" class="ece" alt="El Català Emprenyat" loading="lazy"></a></div></div></footer>
<div id="lightbox" class="lightbox" role="dialog" aria-modal="true" aria-label="{e(t['zoom'])}" hidden><button id="close-lightbox" type="button">{e(t['close'])} ×</button><img id="lightbox-image" src="{img}realitzacio.png" alt=""></div>
<script id="page-data" type="application/json">{json.dumps(config,ensure_ascii=False)}</script></body></html>'''

for lang, content in D.items():
    folder=ROOT if lang=='ca' else ROOT/lang
    folder.mkdir(exist_ok=True)
    (folder/'index.html').write_text(render(lang,content))
print('Pàgines CA, EN i ES generades.')
