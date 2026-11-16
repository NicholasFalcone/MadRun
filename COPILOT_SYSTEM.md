# Sistema del Copilota - Documentazione

## Panoramica

**Stato del prototipo:** i componenti descritti sotto calcolano valori ed
emettono eventi; non modificano ancora le parti del veicolo, la torretta o
l'inventario. Il copilota non ha ancora logica autonoma o replica delle stats.
Ogni `MadRunPawn` possiede un Child Actor Component `Copilot`, configurabile
nel Blueprint del veicolo e accessibile tramite `Get Copilot`.

Il sistema del Copilota è un personaggio NPC non giocabile che supporta il giocatore in tre aree principali:
- **Riparazione**: Ripara i danni ai veicoli
- **Combattimento**: Spara ai nemici
- **Scavenging**: Raccoglie risorse e oggetti

Ogni abilità è influenzata dalle statistiche del copilota, creando un sistema dinamico dove le abilità del copilota migliorano le sue performance in questi compiti.

## Statistiche del Copilota

### FCopilotStats
Struct che contiene le statistiche base e i valori derivati del copilota.

**Statistiche Base (0-100):**
- `MechanicsSkill`: Influenza la velocità di riparazione
- `AccuracySkill`: Influenza la precisione nel tiro e il danno
- `ScavengingSkill`: Influenza la probabilità di trovare loot

**Statistiche Derivate (Calcolate automaticamente):**
- `RepairSpeedMultiplier`: Moltiplicatore di velocità di riparazione
  - Formula: 1.0 + (MechanicsSkill / 100) * 0.5
  - Intervallo: 1.0x (0% skill) - 1.5x (100% skill)
  
- `AccuracyBonus`: Riduzione della deviazione di tiro in gradi
  - Formula: (AccuracySkill / 100) * 5.0
  - Intervallo: 0° (0% skill) - 5° (100% skill)
  
- `ScavengingFindChance`: Probabilità di trovare loot
  - Formula: 0.4 + (ScavengingSkill / 100) * 0.5
  - Intervallo: 0.4 (0% skill) - 0.9 (100% skill)

## Componenti del Copilota

### 1. ACopilot (Character)
La classe principale del personaggio copilota.

**Proprietà:**
```cpp
FCopilotStats CopilotStats;                           // Statistiche del copilota
UCopilotRepairComponent* RepairComponent;             // Componente per le riparazioni
UCopilotCombatComponent* CombatComponent;             // Componente per il combattimento
UCopilotScavengingComponent* ScavengingComponent;     // Componente per lo scavenging
```

**Funzioni Principali:**
- `SetCopilotStats(FCopilotStats NewStats)`: Imposta tutte le statistiche
- `SetMechanicsSkill(float NewSkill)`: Modifica solo l'abilità di meccanica
- `SetAccuracySkill(float NewSkill)`: Modifica solo l'abilità di precisione
- `SetScavengingSkill(float NewSkill)`: Modifica solo l'abilità di scavenging

### 2. UCopilotRepairComponent
Gestisce il sistema di riparazione del copilota.

**Funzioni Principali:**
- `StartRepair(AActor* Target, float BaseRepairTime, float MaxRepairAmount)`: Inizia a riparare un bersaglio
- `CancelRepair()`: Annulla la riparazione in corso
- `GetRepairProgress()`: Restituisce il progresso (0.0 - 1.0)

**Eventi:**
- `OnRepairStarted`: Emesso quando inizia la riparazione
- `OnRepairCompleted`: Emesso quando la riparazione è completata
- `OnRepairCancelled`: Emesso quando la riparazione viene annullata

**Comportamento:**
- Il tempo di riparazione viene modificato in base a `RepairSpeedMultiplier`
- Il tempo effettivo = BaseRepairTime / RepairSpeedMultiplier
- Esempio: Con 100% Mechanics skill (1.5x multiplier), un'riparazione da 10 secondi diventa 6.67 secondi

### 3. UCopilotCombatComponent
Gestisce il combattimento e il tiro del copilota.

**Funzioni Principali:**
- `SetWeapon(AActor* Weapon, int32 MaxAmmo)`: Assegna un'arma al copilota
- `AimAtTarget(AActor* Target)`: Mira a un bersaglio
- `FireAtTarget()`: Spara al bersaglio corrente
- `Reload(float ReloadTime)`: Ricarica l'arma
- `AddAmmo(int32 Amount)`: Aggiunge munizioni
- `GetCurrentAmmo()`: Restituisce le munizioni attuali
- `GetCurrentAccuracyDeviation()`: Restituisce la deviazione di tiro in gradi

**Eventi:**
- `OnFireStarted`: Emesso quando inizia a sparare
- `OnFireCompleted`: Emesso quando il colpo è completo
- `OnAmmoCounted`: Emesso quando le munizioni cambiano

**Comportamento:**
- La deviazione di tiro diminuisce all'aumentare di AccuracySkill
- Deviazione = max(0, 10 - AccuracyBonus)
- Il danno aumenta con l'abilità di accuratezza
  - DamageBonus = (AccuracySkill / 100) * 0.3 (fino a 30% di danno extra)
  - TotalDamage = 25.0 * (1.0 + DamageBonus)

### 4. UCopilotScavengingComponent
Gestisce la ricerca e la raccolta di risorse.

**Funzioni Principali:**
- `StartScavenging(AActor* Target, float SearchDuration, TArray<FLootTable> LootTable)`: Inizia a cercare loot
- `CancelScavenging()`: Annulla la ricerca
- `GetScavengingProgress()`: Restituisce il progresso (0.0 - 1.0)
- `GetLootFindChanceModifier()`: Restituisce il modificatore di probabilità di loot

**Eventi:**
- `OnScavengeStarted`: Emesso quando inizia la ricerca
- `OnScavengeCompleted`: Emesso quando la ricerca è completa
- `OnLootFound`: Emesso quando viene trovato un oggetto

**Struttura FLootTable:**
```cpp
FString LootType;        // Tipo di loot (es. "Fuel", "Parts", "Ammo")
float DropChance;        // Probabilità di drop (0-100)
int32 MinAmount;         // Quantità minima
int32 MaxAmount;         // Quantità massima
```

**Comportamento:**
- La probabilità di trovare loot viene modificata da `ScavengingFindChance`
- Probabilità effettiva = DropChance * ScavengingFindChance
- Esempio: Con 100% Scavenging skill (0.9 chance), un loot con 50% di probabilità diventa 45%

## Esempio di Utilizzo

```cpp
// Ottenere il copilota figlio del veicolo
ACopilot* Copilot = Vehicle->GetCopilot();
if (!Copilot) { return; }

// Impostare le statistiche
FCopilotStats Stats;
Stats.MechanicsSkill = 80.0f;
Stats.AccuracySkill = 75.0f;
Stats.ScavengingSkill = 60.0f;
Copilot->SetCopilotStats(Stats);

// Usare il componente di riparazione
Copilot->RepairComponent->StartRepair(VehicleToRepair, 10.0f, 100.0f);

// Usare il componente di combattimento
Copilot->CombatComponent->SetWeapon(GunActor, 30);
Copilot->CombatComponent->AimAtTarget(Enemy);
Copilot->CombatComponent->FireAtTarget();

// Usare il componente di scavenging
TArray<FLootTable> LootTable;
FLootTable Fuel;
Fuel.LootType = TEXT("Fuel");
Fuel.DropChance = 60.0f;
Fuel.MinAmount = 10;
Fuel.MaxAmount = 30;
LootTable.Add(Fuel);

Copilot->ScavengingComponent->StartScavenging(LootContainer, 5.0f, LootTable);
```

## Configurazione Predefinita

Il copilota viene creato con statistiche predefinite:
- **MechanicsSkill**: 65.0 (Buono alle riparazioni)
- **AccuracySkill**: 75.0 (Buono al tiro)
- **ScavengingSkill**: 60.0 (Decente nello scavenging)

Queste statistiche possono essere facilmente modificate in Blueprint o tramite codice C++.

## Note di Implementazione

- Tutti i componenti vengono inizializzati automaticamente in `ACopilot::BeginPlay()`
- Le statistiche derivate vengono ricalcolate automaticamente ogni volta che cambia una statistica base
- Il tick è abilitato solo quando effettivamente necessario (durante una riparazione, ricarica, o ricerca)
- I log vengono scritti in tutti i punti critici per facilitare il debugging
