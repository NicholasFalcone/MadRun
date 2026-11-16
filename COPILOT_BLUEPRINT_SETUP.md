# Configurazione del Copilota per Blueprint

Questa guida mostra come configurare e usare il sistema del Copilota direttamente da Unreal Engine Blueprint Editor.

I componenti del copilota sono prototipi: i loro eventi non riparano
automaticamente il veicolo, non sparano con la torretta e non aggiungono
risorse all'inventario. Le sezioni su UI, animazioni e test sono suggerimenti,
non funzionalita gia presenti o verifiche eseguite.

## 🔵 Creare il Blueprint del Copilota

### Passo 1: Creare una nuova Blueprint Class
1. Nel Content Browser, fai clic destro → "Blueprint Class"
2. Scegli "Copilot" come classe genitore
3. Nomina il blueprint: `BP_Copilot`
4. Fai doppio clic per aprire l'editor

### Passo 2: Creare il Data Asset del copilota

Nel Content Browser crea un Data Asset di tipo `MadRunCopilotData`. Assegna
`Body Mesh` a una Skeletal Mesh e imposta in `Stats` le tre abilita tra 0 e
100. `Name` e un'etichetta descrittiva. Nei Class Defaults del Blueprint
dell'auto assegna l'asset a `Initial Copilot Data`. Il veicolo applica la mesh
e le statistiche all'avvio; se lo slot e vuoto restano i valori del Blueprint
del copilota. Un asset senza mesh o con abilita non valide viene rifiutato
con un errore nel log.

La mesh usa la posa o l'Anim Blueprint configurati sul personaggio: scegli uno
scheletro compatibile. Le statistiche modificate durante il gioco non vengono
replicate.

### Valori predefiniti senza Data Asset

Nel Blueprint Editor, vai alla scheda "Details" e cerca "Copilot Stats":

```
Copilot Stats
├── Mechanics Skill: 65.0 (0-100)
├── Accuracy Skill: 75.0 (0-100)
└── Scavenging Skill: 60.0 (0-100)
```

**Suggested Configurations:**

**Expert Copilot** (End-game)
- Mechanics Skill: 100.0
- Accuracy Skill: 95.0
- Scavenging Skill: 90.0

**Balanced Copilot** (Mid-game)
- Mechanics Skill: 75.0
- Accuracy Skill: 70.0
- Scavenging Skill: 70.0

**Specialist Mechanic** (Repair focused)
- Mechanics Skill: 100.0
- Accuracy Skill: 50.0
- Scavenging Skill: 50.0

**Specialist Gunner** (Combat focused)
- Mechanics Skill: 50.0
- Accuracy Skill: 100.0
- Scavenging Skill: 50.0

**Specialist Scavenger** (Looting focused)
- Mechanics Skill: 50.0
- Accuracy Skill: 50.0
- Scavenging Skill: 100.0

## 📍 Inserire il copilota nel Blueprint dell'auto

Ogni veicolo derivato da `MadRunPawn` include il Child Actor Component
`Copilot`, collegato alla mesh del veicolo. Non serve posizionare un
copilota separato nel livello.

1. Compila il progetto e apri il Blueprint dell'auto, ad esempio
   `BP_OffroadCar_Pawn`.
2. Nel pannello **Components**, seleziona il componente ereditato `Copilot`.
3. In **Child Actor Class**, scegli `BP_Copilot`; per default viene usata
   la classe C++ `Copilot`, priva di mesh e animazioni.
4. Configura una posa o Anim Blueprint compatibile con la mesh del Data Asset
   in `BP_Copilot`. Nel Blueprint
   dell'auto, regola posizione e rotazione relative del componente, oppure
   seleziona un socket della mesh del veicolo, per collocarlo sul sedile.
5. Nel graph dell'auto usa `Get Copilot` per ottenere il personaggio e accedere
   alle sue statistiche e ai suoi componenti. Gestisci il caso in cui non sia
   valido, ad esempio se non e stata assegnata una classe.

Il copilota figlio segue il veicolo; movimento, collisioni e possesso automatico
sono disabilitati per non interferire con la fisica dell'auto. Il sistema non aggiunge
ancora animazioni, IA, replica delle statistiche o collegamenti alle meccaniche
esistenti di riparazione, torretta e scavenging.

## 🔗 Collegare gli Eventi

### Nella Construction Script

```
Construct Event
├── Set Copilot Stats (se vuoi statistiche personalizzate)
└── Print String (debug)
```

### Nell'Event Graph

Puoi ascoltare gli eventi dei componenti:

**Repair Component Events:**
- On Repair Started → Trigger animazione di riparazione
- On Repair Completed → Aggiorna UI, dai reward
- On Repair Cancelled → Annulla animazioni

**Combat Component Events:**
- On Fire Started → Trigger effetti sonori e visuali
- On Fire Completed → Mostra risultato del colpo
- On Ammo Counted → Aggiorna contatore munizioni

**Scavenging Component Events:**
- On Scavenge Started → Mostra animazione di ricerca
- On Scavenge Completed → Mostra risultati finali
- On Loot Found → Aggiungi loot all'inventario, effetti VFX

## 🎬 Creare Animazioni

Crea animazioni per i seguenti stati:

### Repair Component
```
Character State: Repairing
├── Start: Animazione di preparazione (0.5s)
├── Loop: Animazione di riparazione attiva (loop)
└── End: Animazione di completamento (0.5s)
```

### Combat Component
```
Weapon State: Armed
├── Idle: Punta normale
├── Aiming: Punta migliore, corpo teso
└── Firing: Rinculo e effetto fuoco

Character State: Reloading
├── Start: Afferra munizioni (0.3s)
├── Middle: Inserisce munizioni (0.8s)
└── End: Spalla arma (0.3s)
```

### Scavenging Component
```
Character State: Searching
├── Start: Guarda intorno (0.3s)
├── Loop: Fruga in contenitori (loop)
└── End: Si raddrizza, satisfatto (0.5s)
```

## 🎨 Creare una UI

### Panel per il Copilota

```
Canvas Panel
├── Copilot Status Panel
│   ├── Nome: "BP_Copilota"
│   ├── Barra HP
│   ├── Statistiche
│   │   ├── Mechanics: 65
│   │   ├── Accuracy: 75
│   │   └── Scavenging: 60
│   └── Azione Attuale
│       ├── Repair Progress Bar (se in riparazione)
│       ├── Ammo Counter (se armato)
│       └── Scavenge Progress Bar (se ricercando)
└── Event Notifications
    └── Toast per loot trovato
```

### Collegarsi agli Eventi

```cpp
// In Event Graph del Widget

Event Tick
├── Get Copilot Reference
├── Branch: Is Repairing?
│   ├── True: Update Repair Progress Bar
│   │       └── Set Percent: Get Repair Progress
│   └── False: Hide Repair Progress Bar
├── Branch: Is Armed?
│   ├── True: Update Ammo Counter
│   │       └── Set Text: Get Current Ammo / Get Max Ammo
│   └── False: Hide Ammo Counter
└── Branch: Is Scavenging?
    ├── True: Update Scavenge Progress Bar
    │       └── Set Percent: Get Scavenging Progress
    └── False: Hide Scavenge Progress Bar

On Repair Started Event
├── Play Animation: Show Repair Progress
├── Set Text: "Repairing Vehicle..."
└── Play Sound: Repair Start SFX

On Loot Found Event
├── Create Toast Notification
├── Set Text: "Found {Amount}x {LootType}"
├── Add to Inventory
└── Play Sound: Item Found SFX
```

## 🧪 Testare il Sistema

### Da Blueprint Event Graph

Crea funzioni test per verificare ogni componente:

```
Test Repair Function
├── Get Copilot
├── Call Start Repair
│   ├── Target: Self
│   ├── Base Repair Time: 10.0
│   └── Max Repair Amount: 100.0
├── Wait for Event: On Repair Completed
└── Print String: "Repair Test Completed"

Test Combat Function
├── Get Copilot
├── Set Weapon
│   ├── Weapon: Self
│   └── Max Ammo: 30
├── Aim At Target
│   └── Target: Self
├── Fire At Target
└── Print String: "Fire Test Completed"

Test Scavenging Function
├── Get Copilot
├── Make Loot Table
│   ├── Fuel (Drop Chance: 60%, Min: 20, Max: 50)
│   ├── Parts (Drop Chance: 40%, Min: 5, Max: 15)
│   └── Ammo (Drop Chance: 50%, Min: 30, Max: 100)
├── Start Scavenging
│   ├── Target: Self
│   ├── Search Duration: 5.0
│   └── Loot Table: (created above)
└── Print String: "Scavenge Test Completed"
```

## 📊 Monitorare le Statistiche in Tempo Reale

Nel Detail Panel della Blueprint in Editor:

1. Seleziona il Copilot nel Viewport
2. Vai alle "Details"
3. Espandi "Copilot Stats"
4. Guarda i valori derivati aggiornati in tempo reale:
   - Repair Speed Multiplier
   - Accuracy Bonus
   - Scavenging Find Chance

## 🎯 Configurazioni Avanzate

### Modifiare Statistiche Dinamicamente

Crea una funzione nel Game Mode per modificare le abilità del copilota:

```
Upgrade Copilot Skill (Skill Type, Amount)
├── Switch Skill Type
├── Case Mechanics: Set Mechanics Skill (Current + Amount)
├── Case Accuracy: Set Accuracy Skill (Current + Amount)
├── Case Scavenging: Set Scavenging Skill (Current + Amount)
└── Print String: "Upgraded {Skill} to {New Value}"
```

### Salvare/Caricare Configurazioni

```
Save Copilot Config
├── Get Current Stats
├── Save to Config File
└── Print String: "Config Saved"

Load Copilot Config
├── Load from Config File
├── Set Copilot Stats
└── Print String: "Config Loaded"
```

---

**Tips:**
- Usa le statistiche per aumentare la difficoltà dei nemici che il copilota affronta
- Bilancia le abilità in base al ruolo che vuoi che il copilota abbia nel gioco
- Crea feedback visivi forti quando i compiti vengono completati
- Usa suoni e animazioni per rendere le azioni del copilota soddisfacenti
