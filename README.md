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
La hitbox e solo per query, blocca per impostazione predefinita il canale
Visibility e ignora gli altri canali: adattare dimensioni, posizione e
risposte di collisione al modello e al sistema di colpi.

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
passaggio a zero e il ritorno sopra zero. La salute della ruota non modifica
ancora la fisica della ruota.
