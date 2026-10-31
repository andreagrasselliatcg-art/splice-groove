# Splice Groove: generatore di pattern di batteria per Logic Pro (AU / VST3)

Splice Groove è uno strumento virtuale che **genera pattern di batteria** in 5 generi e li suona
con i **campioni Splice che hai già scaricato** sul Mac. Il pattern va a tempo con Logic, e puoi
trascinarlo nella timeline come regione MIDI.

Generi: **Tech House · House · Deep House · Afro Tech · Indie Tech**

---

## Come funziona con Splice

Splice non offre un'API pubblica per accedere al catalogo online. Il plugin quindi usa i suoni che
hai già scaricato con l'app Splice, che li salva in locale (di default in `~/Splice/sounds`).

1. All'avvio il plugin scansiona la cartella Splice e riconosce i **one-shot di batteria** dal nome
   del file e delle cartelle (kick, clap, snare, hat aperto/chiuso, shaker, perc, ride/top).
   I **loop vengono esclusi** automaticamente.
2. Per ogni genere sceglie preferibilmente suoni da pack con nomi coerenti (es. "Afro", "Tribal" e
   congas/bonghi per l'Afro Tech; "Deep" e snap per la Deep House; "Indie", "Nu Disco" e
   rullanti live per l'Indie Tech).
3. Puoi anche **trascinare un campione dall'app Splice (o dal Finder) su una corsia** per usarlo subito.

> Se hai cambiato la cartella in Splice (Preferenze → Folder location), clicca **"Splice folder..."**
> nel plugin e selezionala.

---

## Installazione

Logic Pro usa solo plugin **Audio Unit (AU)**, che vanno compilati su un Mac. Ci sono due strade.

### Opzione A: senza installare nulla (build automatica su GitHub)

1. Crea un repository su GitHub (anche privato) e carica tutta questa cartella.
2. GitHub compila da solo il plugin (tab **Actions** → "Build macOS (AU + VST3)", circa 10 minuti).
   La build verifica anche il plugin con `auval`, il validatore ufficiale di Apple.
3. Scarica l'artifact **SpliceGroove-macOS.zip**, decomprimilo e fai doppio clic su
   **`install_mac.command`**. Se macOS lo blocca: tasto destro → Apri.

### Opzione B: compilare sul tuo Mac

```bash
xcode-select --install      # una volta sola
brew install cmake          # una volta sola
cd SpliceGroove
./build_mac.sh              # compila, installa e valida
```

Poi in Logic: **nuova traccia Software Instrument → AU Instruments → Andrea Grasselli → Splice Groove**.

---

## Uso in Logic

| Controllo | Funzione |
|---|---|
| **Genere** | Cambiando genere ottieni un nuovo groove, un nuovo kit e lo swing adatto |
| **GENERATE** | Nuovo pattern dello stesso genere, mantenendo i suoni |
| **NEW SOUNDS** | Nuovo kit dalla tua libreria Splice |
| **Rnd** (per corsia) | Cambia solo quel suono |
| Clic sul nome del campione | Ascolta il suono |
| **DENSITY** | Da groove minimale a groove pieno, con fill a fine loop |
| **SWING** | 50% dritto, fino a 75% molto shuffle (Deep House ~60%) |
| **HUMANIZE** | Variazione di velocity |
| **SEQ ON** | Acceso: il plugin suona il pattern a tempo col play di Logic |
| **PREVIEW** | Ascolta il pattern anche a Logic fermo |
| **DRAG MIDI TO LOGIC** | Trascina il pattern (2/4/8/16 battute) su una traccia |
| **Save MIDI...** | Salva il file .mid |

**Editing della griglia:** clic = accendi/spegni · Shift+clic = accento · clic destro = cambia velocity · trascina = disegna.

### Due modi di lavorare
- **Plugin come drum machine:** lascia SEQ ON e premi play in Logic. Il pattern (2 battute) va in loop a tempo.
- **Regione MIDI in Logic:** trascina il MIDI sulla traccia del plugin, poi **spegni SEQ ON**.
  Il plugin suona le note della regione e puoi modificarle nel Piano Roll.

Note MIDI (standard GM): Kick C1 (36) · Snare D1 (38) · Clap D#1 (39) · Closed Hat F#1 (42) ·
Open Hat A#1 (46) · Ride D#2 (51) · Perc D3 (62) · Shaker A#3 (70)

Tutto (pattern, suoni scelti, parametri) viene salvato nel progetto Logic.

---

## Caratteristiche dei generi

| Genere | BPM | Swing | Carattere |
|---|---|---|---|
| Tech House | 126 | 54% | Cassa dritta, hat in sedicesimi con accenti, open hat in levare, perc groovy, ghost kick |
| House | 124 | 56% | Classico: clap su 2 e 4, open hat in levare, ride, shaker leggero |
| Deep House | 120 | 60% | Più swing, dinamiche morbide, snap/rim, shaker continuo |
| Afro Tech | 122 | 52% | Congas e tom in tresillo (3-3-2), shaker con accenti afro, top percussivi |
| Indie Tech | 120 | 50% | Rullante vero su 2 e 4, hat in ottavi, tamburello, crash/ride |

---

## Problemi comuni

- **"Splice folder not found"**: clicca "Splice folder..." e seleziona la cartella dei suoni Splice.
- **Una corsia è vuota**: nella libreria non c'è nessun one-shot di quel tipo. Scarica qualche
  one-shot da Splice e clicca **Rescan**, oppure trascina un campione sulla corsia.
- **Il plugin non compare in Logic**: Logic → Impostazioni → Plug-in Manager → Reset & Rescan Selection.
- **Un suono finisce nella corsia sbagliata**: il riconoscimento si basa sui nomi dei file.
  Trascina il campione giusto sulla corsia.

## Struttura del progetto
```
Source/GenreEngine.*     template ritmici e generatore per genere
Source/SampleLibrary.*   scansione e classificazione della libreria Splice
Source/PluginProcessor.* sequencer sincronizzato all'host, sampler, export MIDI, stato
Source/PluginEditor.*    interfaccia grafica
Tests/                   test automatici (libreria finta, transport simulato, MIDI, GUI)
```
Il plugin è costruito con JUCE 8 (licenza GPLv3 / commerciale).
