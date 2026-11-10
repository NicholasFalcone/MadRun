# MadRun

Il progetto usa Git LFS per gli asset Unreal (`.uasset` e `.umap`).
Installare Git LFS ed eseguire `git lfs install` prima di clonare il
repository o scaricare gli asset con `git lfs pull` dopo la clonazione.
Android File Server e disabilitato; se necessario, configurarlo localmente
senza includere token di sicurezza nei commit.

Il progetto usa la mappa e il GameMode della variante OffRoad come predefiniti.
Alcuni materiali e texture nella cartella `Vehicles/SportsCar` sono condivisi
con il veicolo OffRoad e devono essere mantenuti.

## Prototipo online Steam

Questa prima versione permette a due giocatori di provare la gara su un
listen server: uno ospita la partita e l'altro si collega tramite una lobby
Steam. Usa temporaneamente l'App ID pubblico di test `480` (Spacewar), non
l'ID definitivo di MadRun. Prima della distribuzione sostituirlo con l'App ID
del gioco e configurare la distribuzione Steam.

Per provare servono due computer con Steam avviato, due account distinti e la
stessa build Windows Development del progetto, inclusa la mappa OffRoad. Steam
Sockets non funziona nel processo Play in Editor di Unreal 5.7: usare processi
di gioco standalone o build pacchettizzate. Assicurarsi che la mappa contenga
almeno due Player Start e che tutti i checkpoint della gara abbiano
`Is Spatially Loaded` disattivato, cosi il GameMode puo trovarli all'avvio.

Aprire la console nella build Development:

1. L'host esegue `MadRunHost` e aspetta la creazione della lobby e il
   caricamento della mappa.
2. L'altro giocatore esegue `MadRunFind`, attende i risultati e usa
   `MadRunJoin 0` (o l'indice della lobby desiderata). I risultati e gli
   errori sono mostrati a schermo e nel log.
3. `MadRunLeave` chiude la sessione locale e torna alla mappa in singolo;
   se eseguito dall'host, disconnette anche l'altro giocatore.

Le lobby sono filtrate con un identificatore MadRun per non unirsi ad altre
applicazioni che usano l'App ID 480. Il server gestisce il percorso e invia
lo stato della gara al giocatore interessato; il movimento usa la replica di
Chaos Vehicles. Si tratta di un prototipo: non include ancora matchmaking,
inviti, classifica, riconnessione o verifica anti-cheat. Tiro, danni,
carburante e stato delle parti sono gestiti dal server e sincronizzati con i
client. Anche scavenging ed equipaggiamento sono gestiti dal server.

## Carburante e stato dei componenti

Ogni veicolo basato su `MadRunPawn` possiede un Child Actor Component
`Fuel Tank Actor`, collegato alla mesh del veicolo. Espone l'Actor tramite
`Get Fuel Tank`. La posizione relativa del componente e configurabile nel
Blueprint del veicolo; si puo assegnare una sottoclasse Blueprint di
`MadRunFuelTank` per configurare la hitbox e i valori del serbatoio.
La hitbox blocca i canali Visibility e MadRunProjectile e ignora gli altri:
adattare dimensioni, posizione e risposte di collisione al modello.
La root del serbatoio e un componente `Tank Mesh`: assegnargli una static
mesh nel Blueprint del serbatoio. La mesh non ha collisione per impostazione
predefinita; la hitbox figlia continua a gestire i colpi.

La UI del veicolo mostra in alto a sinistra carburante corrente e capacita
in litri, salute del serbatoio e salute delle ruote con i rispettivi massimi.
Le ruote sono identificate dagli indici di `Get Wheel By Index`. Una parte
non disponibile viene indicata con `--`; senza veicolo le righe delle ruote
vengono rimosse. Velocita e marcia restano separate in basso a destra.

Il serbatoio riceve il danno Unreal (`Apply Damage`, `Apply Point Damage` e
`Apply Radial Damage`): ogni valore positivo viene arrotondato per eccesso
alla salute intera. Gli urti fisici non infliggono danno automaticamente.
Se si usa un trace, applicare il danno all'Actor colpito; la mesh del veicolo
puo schermare la hitbox a seconda del posizionamento e delle collisioni.

Nei valori predefiniti dell'Actor serbatoio si possono configurare capacita,
consumo a pieno acceleratore, perdita massima e salute massima. La capacita
e il carburante sono espressi in litri; i due consumi in litri al secondo.
Il serbatoio parte pieno: con le impostazioni iniziali contiene 100 litri,
consuma 0,2 litri al secondo a pieno acceleratore e perde fino a 0,1 litri
al secondo a salute zero.

Il consumo e proporzionale all'acceleratore, anche in retromarcia. La perdita
e proporzionale alla salute mancante e continua anche a veicolo fermo;
riparare il serbatoio interrompe la perdita senza ripristinare il carburante.
Quando e vuoto, la coppia motore viene azzerata; dopo un rifornimento viene
ripristinata. `Add Fuel` aggiunge carburante fino alla capacita del serbatoio.

Ruote e serbatoio espongono l'interfaccia Blueprint `MadRun Part Health`:
`Get Health`, `Get Max Health`, `Set Health`, `Take Damage` e `Do Repair`.
La salute resta tra zero e il massimo; `Do Repair` la ripristina al massimo.
Gli eventi `On OutofWork` e `On Repaired` segnalano rispettivamente il
passaggio a zero e il ritorno sopra zero.

Le ruote sono oggetti Chaos, non Actor. Ogni ruota OffRoad ha un Actor
hitbox che segue la sua mesh e inoltra `Apply Point Damage` alla ruota Chaos
corrispondente. La hitbox e configurabile nei componenti del Blueprint del
veicolo. Anche i colpi che intercettano un bone fisico della ruota nella mesh
del veicolo vengono inoltrati alla stessa ruota. Per riparare una ruota,
ottenere la ruota dalla hitbox con `Get Wheel`
o dal veicolo con `Get Wheel By Index` e chiamare `Do Repair` dell'interfaccia
Health. OffRoad usa gli indici 0/1 anteriori sinistra/destra e 2/3 posteriori
sinistra/destra.

Ogni ruota e il serbatoio hanno un indicatore di salute nello spazio di
gioco con una barra di progresso non interattiva. Compare quando la salute cambia per danno, riparazione o `Set Health`
e rimane visibile per tre secondi dall'ultima variazione; una chiamata che
non modifica la salute non riattiva il timer. Mostra la salute della parte,
non la quantita di carburante. Il componente `Health Display` permette di
configurare altezza sopra il centro della parte, dimensione e durata.
L'altezza resta verticale nel mondo anche se ruota la parte. L'indicatore e orientato verso
la prima camera locale e puo essere coperto dalla geometria della scena.

Quando una ruota raggiunge zero salute, trazione, sterzo e frenata vengono
disabilitati; sospensione e mesh rimangono presenti. La riparazione ripristina
le impostazioni della ruota e del differenziale. Durante la disattivazione
le coppie esterne della ruota vengono azzerate: eventuali sistemi che le
gestiscono devono riapplicarle dopo la riparazione.

Il progetto include `MadRunProjectile`, un Actor base configurabile con
collisione, movimento e danno. Un'arma puo generarlo rivolto nella direzione
di tiro, impostando Owner al veicolo che spara e Instigator al relativo pawn.
All'impatto applica Point Damage all'Actor colpito e viene distrutto; ignora
il proprietario e i suoi Actor collegati. I proiettili personalizzati devono usare il canale oggetto
`MadRunProjectile`, bloccare WorldDynamic e applicare danno all'Actor colpito.
La carrozzeria puo schermare le hitbox: verificarne posizione e dimensioni
nel Blueprint del veicolo.

## Equipaggiamento del veicolo

Ogni equipaggiamento e un Data Asset distinto: creare un asset di tipo
`MadRunEngineEquipment`, `MadRunWeaponEquipment`, `MadRunTiresEquipment` o
`MadRunFuelTankEquipment`. Le classi possono essere estese in Blueprint
per aggiungere dati specifici. Nei valori predefiniti del Blueprint del
veicolo assegnare gli asset ai quattro slot `Initial Engine Equipment`,
`Initial Weapon Equipment`, `Initial Tires Equipment` e
`Initial Fuel Tank Equipment`.

Gli asset configurati vengono equipaggiati all'avvio. Gli slot lasciati vuoti
non rimuovono le parti o la grafica preesistenti. `Equip` seleziona direttamente
un asset, senza livelli o ordine obbligatorio, e restituisce se l'operazione e
riuscita. Arma, gomme e serbatoio
richiedono una static mesh: la mesh viene applicata rispettivamente alla canna
della torretta, a tutte le gomme OffRoad o alla mesh del serbatoio.
`Mesh Transform` dell'arma controlla posizione, rotazione e scala rispetto alla
torretta; le gomme mantengono le trasformazioni dei componenti del veicolo.
Il motore non ha grafica separata. I getter sul veicolo, sulla torretta, sulle
ruote e sul serbatoio restituiscono l'asset selezionato, oppure `None`.

L'equipaggiamento arma configura anche le statistiche della torretta descritte
sotto; gli altri equipaggiamenti non applicano ancora statistiche specifiche.
Il salvataggio non e implementato; un nuovo veicolo riparte con gli slot iniziali del Blueprint.

## Gara a waypoint

La mappa usa `BP_OffRoadGameMode`, che eredita la logica di gara di
`AMadRunGameMode`. Per definire il percorso, posizionare nella mappa degli
Actor `MadRunRaceCheckpoint` o Blueprint derivati: un checkpoint `Start`,
zero o piu checkpoint `Waypoint` con indici consecutivi da 1, e un checkpoint
`Finish`. Il componente `Trigger` definisce il volume di passaggio; regolarne
le dimensioni nel Blueprint o nell'istanza. `Visual` puo ricevere una mesh
facoltativa per rendere visibili partenza, waypoint e traguardo.

La gara parte quando il veicolo attraversa la partenza. I waypoint devono
essere raggiunti in ordine; il traguardo completa la gara solo dopo tutti i
waypoint. L'HUD mostra l'avanzamento e il tempo finale. La gara non prevede
ancora giri, classifiche o salvataggio dei tempi. Indici duplicati o non
consecutivi, oppure una partenza o un traguardo mancanti, disabilitano il
percorso e vengono segnalati nel log.

## Scavenger Point

Posizionare nella mappa un Actor `MadRunScavengerPoint` o un suo Blueprint.
Il componente `Area` definisce il raggio di interazione (400 cm predefiniti),
misurato dal centro del veicolo; `Marker` e una mesh provvisoria configurabile.
In `Possible Equipment`, assegnare i Data Asset e un peso non negativo a
ciascuna voce. La probabilita e il peso diviso per la somma dei pesi:
ad esempio 3 e 7 corrispondono al 30% e al 70%. Il peso zero esclude una
voce. Almeno una voce con peso positivo e asset valido e necessaria.

Entrando nell'area appare l'indicazione per cercare con `E`. Il punto estrae
un pezzo una sola volta e mostra nome e descrizione: `Y` lo equipaggia, `N`
chiude la proposta. Rifiutando o uscendo dall'area il pezzo resta disponibile;
il punto scompare solo dopo un equipaggiamento riuscito. Online il server
estrae un'unica offerta condivisa, verifica distanza e disponibilita e
sincronizza equipaggiamento e stato del punto. Se due giocatori accettano lo
stesso pezzo, solo il primo accettato dal server lo ottiene. Impostare
`Is Spatially Loaded` disattivato anche sui punti scavenging, cosi il server
e i client possono trovarli con World Partition. In caso di errore
controllare la configurazione dell'asset e il log. I tasti sono configurabili
nel Player Controller; `Start Scavenging`, `Accept Scavenged Equipment` e
`Cancel Scavenging` possono essere collegati a controlli aggiuntivi.
Non sono previsti controlli touch predefiniti o salvataggio dei punti.

## Torretta

Il veicolo OffRoad ha una torretta `Roof Turret` sul tetto. Tenendo premuto
il tasto sinistro del mouse spara proiettili. Il Data Asset dell'arma definisce
`Fire Interval` (secondi tra colpi), `Damage`, `Magazine Size` e `Reload Time`
(secondi). Il caricatore parte pieno, si ricarica automaticamente quando e
vuoto e le munizioni di riserva sono infinite. Se si continua a tenere premuto,
il fuoco riprende dopo la ricarica e l'intervallo tra colpi; rilasciando il tasto
la ricarica continua senza sparare. Cambiare arma interrompe la ricarica e
riempie il nuovo caricatore, senza annullare il tempo di attesa del colpo precedente.
Senza un Data Asset arma si usano i valori predefiniti della torretta:
0,2 secondi tra colpi, 5 danni, 30 colpi e 2 secondi di ricarica.
La torretta espone `Get Ammo In Magazine`, `Get Magazine Size` e `Is Reloading`.
In multiplayer il proprietario richiede inizio e fine fuoco al server, che
gestisce mira, cadenza, munizioni, impatti e danni. I proiettili vengono
replicati; l'effetto visivo dell'impatto e inviato ai client interessati, ma
puo andare perso in caso di perdita di pacchetti. `OnTurretFired` e
`OnProjectileHit` vengono eseguiti sul server, non sui client remoti.
La salute di ruote e serbatoio, il carburante, le munizioni, la ricarica e
l'orientamento della torretta sono sincronizzati dal veicolo; le modifiche a
salute e carburante devono essere richieste ed eseguite sul server.
Mira al
pezzo funzionante piu vicino di un altro veicolo MadRun entro 100 metri,
se il percorso del proiettile verso il centro della sua hitbox non e bloccato.
La torretta considera il serbatoio e le hitbox delle ruote, oltre agli Actor
collegati al veicolo che implementano `MadRun Part Health`. I pezzi con salute
zero non sono bersagli; dopo una riparazione possono essere selezionati di nuovo.
`Get Target` restituisce l'Actor della parte, oppure la hitbox nel caso di una ruota.
Tutti gli altri veicoli sono considerati nemici. Senza bersagli spara in avanti rispetto al veicolo.
La selezione non predice il movimento del bersaglio e la carrozzeria puo
schermare ruote e serbatoio.

La posizione del Child Actor Component e la sua classe sono configurabili
nel Blueprint del veicolo. Una sottoclasse Blueprint di `MadRunTurret` puo
personalizzare mesh della canna, raggio di ricerca, intervallo di fuoco,
distanza della volata e classe del proiettile. Le forme di torretta e
proiettile predefinite sono provvisorie. Il tasto `Fire Key` e configurabile
nei valori predefiniti del Player Controller; `Do Fire Start` e `Do Fire Stop`
sul veicolo sono disponibili per collegare controlli aggiuntivi, inclusi
quelli touch. Non e previsto un pulsante touch predefinito.
