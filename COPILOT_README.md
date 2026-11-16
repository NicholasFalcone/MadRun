# Sistema del Copilota - MadRun

## Introduzione

Il **Sistema del Copilota** è un sistema completo per gestire un personaggio NPC non giocabile che supporta il giocatore in tre aree principali: **Riparazione**, **Combattimento** e **Scavenging**. Ogni abilità è influenzata dalle statistiche del copilota, creando un'esperienza dinamica e progressiva.

## 🎯 Caratteristiche Principali

### 1. **Statistiche Dinamiche**
- Tre abilità base: Meccanica, Precisione e Scavenging (0-100)
- Statistiche derivate calcolate automaticamente
- Modifica in tempo reale delle abilità del copilota

### 2. **Sistema di Riparazione**
- Ripara i danni ai veicoli
- La velocità di riparazione aumenta con l'abilità di meccanica
- Fino a 1.5x di velocità con 100% skill

### 3. **Sistema di Combattimento**
- Tiro accurato con probabilità di colpo basata sulle abilità
- Precisione migliorata riduce la deviazione di tiro
- Danno aumentato fino al 30% con 100% di precisione
- Sistema di munizioni e ricarica

### 4. **Sistema di Scavenging**
- Raccolta di risorse da contenitori
- Probabilità di drop modificata dalle abilità
- Tabelle di loot personalizzabili
- Fino a 90% di probabilità di trovare loot con 100% skill

## 📁 File Principali

### Core System
- `MadRunCopilotStats.h/cpp` - Definizione e calcolo delle statistiche
- `MadRunCopilot.h/cpp` - Classe principale del personaggio copilota
- `MadRunCopilotRepairComponent.h/cpp` - Sistema di riparazione
- `MadRunCopilotCombatComponent.h/cpp` - Sistema di combattimento
- `MadRunCopilotScavengingComponent.h/cpp` - Sistema di scavenging

### Documentazione e Esempi
- `COPILOT_SYSTEM.md` - Documentazione tecnica completa
- `MadRunCopilotExample.h/cpp` - Classe di esempio con dimostrazioni

## 🚀 Quick Start

### Creare un Copilota in Codice C++

```cpp
// In un GameMode o Controller
ACopilot* Copilot = GetWorld()->SpawnActor<ACopilot>();

// Impostare le statistiche
FCopilotStats Stats;
Stats.MechanicsSkill = 80.0f;
Stats.AccuracySkill = 75.0f;
Stats.ScavengingSkill = 60.0f;
Copilot->SetCopilotStats(Stats);
```

### Usare la Riparazione

```cpp
// Iniziare la riparazione di un target
// BaseRepairTime: tempo base in secondi
// MaxRepairAmount: quantità massima di riparo
Copilot->RepairComponent->StartRepair(VehicleToRepair, 10.0f, 100.0f);

// Ascoltare gli eventi
Copilot->RepairComponent->OnRepairCompleted.AddDynamic(this, &AMyClass::OnRepairDone);
```

### Usare il Combattimento

```cpp
// Configurare l'arma
Copilot->CombatComponent->SetWeapon(GunActor, 30);

// Mirare e sparare
Copilot->CombatComponent->AimAtTarget(EnemyActor);
Copilot->CombatComponent->FireAtTarget();

// Ricaricare
Copilot->CombatComponent->Reload(2.0f);
```

### Usare lo Scavenging

```cpp
// Creare una tabella di loot
TArray<FLootTable> LootTable;

FLootTable Fuel;
Fuel.LootType = TEXT("Fuel");
Fuel.DropChance = 60.0f;
Fuel.MinAmount = 20;
Fuel.MaxAmount = 50;
LootTable.Add(Fuel);

// Iniziare la ricerca
Copilot->ScavengingComponent->StartScavenging(LootContainer, 5.0f, LootTable);

// Ascoltare quando viene trovato il loot
Copilot->ScavengingComponent->OnLootFound.AddDynamic(
    this, &AMyClass::OnLootFoundHandler);
```

## 📊 Formule delle Statistiche

### Velocità di Riparazione
```
RepairSpeedMultiplier = 1.0 + (MechanicsSkill / 100) * 0.5

Tempo Effettivo = TempoBase / RepairSpeedMultiplier
```

**Esempio**: Con 100% skill, una riparazione da 10 secondi diventa 6.67 secondi

### Precisione di Tiro
```
AccuracyBonus = (AccuracySkill / 100) * 5.0
Deviazione = max(0, 10 - AccuracyBonus)
DamageBonus = (AccuracySkill / 100) * 0.3
```

**Esempio**: Con 100% skill
- Deviazione: 5° (da 10°)
- Danno: Base 25 × 1.3 = 32.5

### Probabilità di Loot
```
ScavengingFindChance = 0.4 + (ScavengingSkill / 100) * 0.5
ProbabilitàEffettiva = DropChance * ScavengingFindChance
```

**Esempio**: Con 100% skill, un loot al 50% diventa 45%

## 🎓 Classe di Esempio

La classe `AMadRunCopilotExample` fornisce dimostrazioni funzionanti di come usare ogni aspetto del sistema:

1. **SpawnAndInitializeCopilot()** - Crea un copilota con statistiche predefinite
2. **DemonstrateRepair()** - Mostra come usare il sistema di riparazione
3. **DemonstrateCombat()** - Mostra come usare il sistema di combattimento
4. **DemonstrateScavenging()** - Mostra come usare lo scavenging

```cpp
// Da Blueprint o Codice
AMadRunCopilotExample* Example = Cast<AMadRunCopilotExample>(ExampleActor);
Example->SpawnAndInitializeCopilot();
Example->DemonstrateRepair();
Example->DemonstrateCombat();
Example->DemonstrateScavenging();
```

## 🔧 Configurazione e Personalizzazione

### Modificare le Statistiche Base
```cpp
// Cambio individuale
Copilot->SetMechanicsSkill(90.0f);
Copilot->SetAccuracySkill(95.0f);
Copilot->SetScavengingSkill(85.0f);

// O tutte insieme
FCopilotStats NewStats;
// ... configura gli stat ...
Copilot->SetCopilotStats(NewStats);
```

### Personalizzare i Moltiplicatori
Tutti i moltiplicatori sono definiti nella funzione `FCopilotStats::RecalculateDerivedStats()`:

```cpp
// Repair Speed
RepairSpeedMultiplier = 1.0f + (MechanicsSkill / 100.0f) * 0.5f;

// Accuracy
AccuracyBonus = (AccuracySkill / 100.0f) * 5.0f;

// Scavenging
ScavengingFindChance = 0.4f + (ScavengingSkill / 100.0f) * 0.5f;
```

Modifica questi valori in `MadRunCopilotStats.cpp` per personalizzare l'equilibrio del gioco.

## 📋 Variabili Importanti

### Componente di Riparazione
- `CurrentState` - Stato attuale (Idle, Repairing, Completed, Cancelled)
- `ElapsedRepairTime` - Tempo trascorso della riparazione
- `AdjustedRepairTime` - Tempo totale di riparazione con moltiplicatori

### Componente di Combattimento
- `CurrentAmmo` - Munizioni attuali
- `MaxAmmo` - Munizioni massime
- `CurrentState` - Stato (Idle, Aiming, Firing, Reloading)

### Componente di Scavenging
- `ElapsedSearchTime` - Tempo di ricerca trascorso
- `TotalSearchDuration` - Durata totale della ricerca
- `FoundLoot` - Mappa dei loot trovati

## 🎮 Integrazione con Blueprint

Tutti i componenti e le funzioni sono accessibili da Blueprint:
- Trascina `ACopilot` nell'editor per posizionarlo nel livello
- Accedi alle funzioni tramite chiamate Blueprint
- Collega gli eventi a custom events in Blueprint per gestire i completamenti

## 🐛 Debug e Logging

Il sistema registra automaticamente le azioni importanti:
```
LogTemp: [Component::Function] - Descrizione dell'azione
```

Tutti i log sono scritti con `UE_LOG(LogTemp, Log, TEXT(...))` per facilità di debugging.

## 📝 Note

- I componenti si inizializzano automaticamente in `BeginPlay()`
- Il tick è abilitato solo quando necessario (durante operazioni attive)
- Le statistiche derivate vengono ricalcolate automaticamente quando cambiano le statistiche base
- Tutti i valori sono clampati tra 0 e 100 per le abilità base

## 🎯 Prossimi Passi

Possibili miglioramenti e estensioni:
1. Aggiungere un sistema di livellamento per le abilità
2. Implementare un'IA per il copilota che scelga autonomamente le azioni
3. Aggiungere animazioni per le diverse azioni
4. Creare una UI che mostri lo stato e le statistiche del copilota
5. Implementare un sistema di esperienza basato sulle azioni eseguite

---

**Versione**: 1.0  
**Motore**: Unreal Engine 5.7  
**Linguaggio**: C++ con support Blueprint
