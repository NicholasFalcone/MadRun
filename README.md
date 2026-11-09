# MadRun

Il progetto usa Git LFS per gli asset Unreal (`.uasset` e `.umap`).
Installare Git LFS ed eseguire `git lfs install` prima di clonare il
repository o scaricare gli asset con `git lfs pull` dopo la clonazione.
Android File Server e disabilitato; se necessario, configurarlo localmente
senza includere token di sicurezza nei commit.

Il progetto usa la mappa e il GameMode della variante OffRoad come predefiniti.
Alcuni materiali e texture nella cartella `Vehicles/SportsCar` sono condivisi
con il veicolo OffRoad e devono essere mantenuti.

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
gioco con uno slider non interattivo. Compare quando la salute cambia per danno, riparazione o `Set Health`
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

## Torretta

Il veicolo OffRoad ha una torretta `Roof Turret` sul tetto. Tenendo premuto
il tasto sinistro del mouse spara proiettili ogni 0,2 secondi. Mira al
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
