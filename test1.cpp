#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

using namespace std;

// Struttura per rappresentare una risorsa
struct Risorsa {
    int RI;  // Identificativo risorsa
    int RA;  // Costo di attivazione
    int RP;  // Costo periodico (manutenzione)
    int RW;  // Numero di turni attivi consecutivi
    int RM;  // Numero di turni di downtime
    int RL;  // Ciclo di vita totale (inclusi attivi e downtime)
    int RU;  // Numero di edifici alimentati per turno attivo
    char RT; // Tipo effetto speciale (A, B, C, D, E, X)
    int RE;  // Parametro dell’effetto speciale (se presente)
};

// Struttura per rappresentare un turno
struct Turno {
    int TM;  // Minimo edifici da alimentare
    int TX;  // Massimo edifici alimentabili
    int TR;  // Profitto per edificio alimentato
};

// Stato di una risorsa acquistata (per gestire ciclo di vita, attivazione, ecc.)
struct StatoRisorsa {
    Risorsa risorsa;
    int turniRimanenti;      // Turni rimanenti nell'attuale stato (attivo o downtime)
    int cicloVitaRimanente;  // Turni rimanenti del ciclo di vita totale
    bool attiva;             // true se la risorsa è attualmente attiva
    int accumulatore;        // Per risorse di tipo E (accumulatore), surplus memorizzato
};

// Aggiorna il budget in base a costi e profitto
void aggiornaBudget(int &budget, int costoAttivazioni, int costoManutenzione, int profitto) {
    budget = budget - costoAttivazioni + profitto - costoManutenzione;
}

// Applica gli effetti speciali delle risorse attive
// (Questa funzione è un esempio semplificato: per ogni risorsa attiva con effetto,
// si modifica qualche parametro globale. L’implementazione va adattata alle regole precise.)
void applicaEffettiSpeciali(const vector<StatoRisorsa>& risorseAttive, int &TM, int &TX, int &TR, vector<int>& RU_list, int turno) {
    // Per ogni risorsa attiva, applica l'effetto in base al tipo
    for (const auto& stato : risorseAttive) {
        char tipo = stato.risorsa.RT;
        int effetto = stato.risorsa.RE;
        if (tipo == 'A') {
            // Effetto "Smart Meter":
            // Se green (RE positivo) aumenta il numero di edifici alimentabili;
            // se non-green, lo riduce (fino a zero).
            // Esempio: modifica ogni RU della lista (approccio semplificato)
            for (auto &ru : RU_list) {
                ru = max(0, (int)floor(ru + ru * effetto / 100.0));
            }
        }
        // Implementare in maniera analoga per gli effetti B, C, D ed E...
        // Per esempio:
        // - Tipo B: modifica TM e TX
        // - Tipo C: modifica la durata (RL) delle risorse acquistate
        // - Tipo D: modifica TR (profitto per edificio)
        // - Tipo E: gestione degli accumulatori (qui si potrebbe accumulare surplus)
    }
}

// Calcola il profitto del turno corrente
int calcolaProfitto(const Turno &turno, const vector<StatoRisorsa>& risorseAttive, int surplusAccumulato) {
    int edificiAlimentati = 0;
    // Somma il contributo di tutte le risorse attive (eventualmente modificate dagli effetti)
    for (const auto &stato : risorseAttive) {
        if (stato.attiva) {
            edificiAlimentati += stato.risorsa.RU;
        }
    }
    // Considera eventuale surplus (ad esempio dagli accumulatori E)
    edificiAlimentati += surplusAccumulato;
    
    // Se non si raggiunge il minimo TM, il profitto del turno è zero
    if (edificiAlimentati < turno.TM)
        return 0;
    
    int edificiEffettivi = min(edificiAlimentati, turno.TX);
    return edificiEffettivi * turno.TR;
}

// Simula un singolo turno
void simulaTurno(int turnoIndex, int &budget, const Turno &turno, vector<StatoRisorsa>& risorseAttive) {
    cout << "\nTurno " << turnoIndex << ":\n";
    
    // 1. Acquisto risorse (placeholder)
    int costoAttivazioni = 0;
    // Qui si potrebbe inserire una funzione "scegliRisorseDaAcquistare" che ritorna un vettore di Risorsa.
    // Per ora non eseguiamo acquisti.
    cout << "Nessun acquisto in questo turno.\n";
    
    // 2. Calcola i costi di manutenzione per le risorse attive
    int costoManutenzione = 0;
    for (const auto &stato : risorseAttive) {
        if (stato.attiva)
            costoManutenzione += stato.risorsa.RP;
    }
    
    // 3. Applica gli effetti speciali
    // Prepara una lista dei valori RU per ogni risorsa attiva (per esempio, per eventuali modifiche)
    vector<int> RU_list;
    for (const auto &stato : risorseAttive) {
        RU_list.push_back(stato.risorsa.RU);
    }
    applicaEffettiSpeciali(risorseAttive, (int&)turno.TM, (int&)turno.TX, (int&)turno.TR, RU_list, turnoIndex);
    
    // 4. Calcola il profitto (supponendo che non vi sia surplus accumulato per semplicità)
    int surplusAccumulato = 0;
    int profitto = calcolaProfitto(turno, risorseAttive, surplusAccumulato);
    
    cout << "Profitto turno: " << profitto << "\n";
    cout << "Costo manutenzione: " << costoManutenzione << "\n";
    
    // 5. Aggiorna il budget per il turno successivo
    aggiornaBudget(budget, costoAttivazioni, costoManutenzione, profitto);
    cout << "Budget aggiornato: " << budget << "\n";
    
    // 6. Aggiorna lo stato delle risorse (ciclo di vita, attivazione/downtime)
    for (auto &stato : risorseAttive) {
        stato.cicloVitaRimanente--;
        if (stato.turniRimanenti > 0) {
            stato.turniRimanenti--;
        } else {
            // Passa dallo stato attivo al downtime e viceversa in modo semplificato
            stato.attiva = !stato.attiva;
            if (stato.attiva)
                stato.turniRimanenti = stato.risorsa.RW;
            else
                stato.turniRimanenti = stato.risorsa.RM;
        }
    }
}

// Funzione che gestisce l'intera simulazione del gioco
void simulaGioco(int budgetIniziale, const vector<Turno>& turni, vector<Risorsa>& risorseDisponibili) {
    int budget = budgetIniziale;
    
    // Stato delle risorse acquistate (inizialmente vuoto)
    vector<StatoRisorsa> risorseAttive;
    
    // Ciclo sui turni
    for (int t = 0; t < turni.size(); t++) {
        simulaTurno(t, budget, turni[t], risorseAttive);
        
        // Qui potresti inserire logica per acquistare nuove risorse:
        // ad esempio, valutare quali risorse tra quelle disponibili (risorseDisponibili)
        // convenga attivare in base al budget corrente e alle regole del gioco.
        // Se viene effettuato un acquisto, aggiornare "risorseAttive" e sottrarre il costo di attivazione.
    }
    
    cout << "\nSimulazione terminata. Budget finale: " << budget << "\n";
}

int main() {
    ifstream inputFile("0-demo.txt"); // File di input
    if (!inputFile) {
        cerr << "Errore nell'apertura del file!" << endl;
        return 1;
    }
    
    // Lettura dei parametri iniziali
    int D, R, T;
    inputFile >> D >> R >> T;
    
    // Lettura delle risorse
    vector<Risorsa> risorse(R);
    for (int i = 0; i < R; ++i) {
        inputFile >> risorse[i].RI >> risorse[i].RA >> risorse[i].RP 
                  >> risorse[i].RW >> risorse[i].RM >> risorse[i].RL 
                  >> risorse[i].RU >> risorse[i].RT;
        if (risorse[i].RT != 'X') {
            inputFile >> risorse[i].RE;
        } else {
            risorse[i].RE = 0;
        }
    }
    
    // Lettura dei turni
    vector<Turno> turni(T);
    for (int i = 0; i < T; ++i) {
        inputFile >> turni[i].TM >> turni[i].TX >> turni[i].TR;
    }
    inputFile.close();
    
    // Visualizzazione dei dati di input (debug)
    cout << "Budget iniziale: " << D << "\nNumero risorse: " << R << "\nNumero turni: " << T << "\n";
    cout << "\nElenco risorse:\n";
    for (const auto& r : risorse) {
        cout << "ID: " << r.RI << ", RA: " << r.RA << ", RP: " << r.RP
             << ", RW: " << r.RW << ", RM: " << r.RM << ", RL: " << r.RL
             << ", RU: " << r.RU << ", RT: " << r.RT << ", RE: " << r.RE << "\n";
    }
    
    cout << "\nElenco turni:\n";
    for (int i = 0; i < T; i++) {
        cout << "Turno " << i << " -> TM: " << turni[i].TM 
             << ", TX: " << turni[i].TX << ", TR: " << turni[i].TR << "\n";
    }
    
    // Avvia la simulazione del gioco
    simulaGioco(D, turni, risorse);
    
    return 0;
}
