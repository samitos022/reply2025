#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

using namespace std;

// Global budget variable
int budget;

// Global Battery structure
struct Battery {
    int surplusGained;
    int capacity;

    Battery() : surplusGained(0), capacity(0) {}
};

Battery globalBattery;

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
    int RE;  // Parametro dell’effetto speciale (se presente), for E type it is capacity
};

// Struttura per rappresentare un turno
struct Turno {
    int TM_base;  // Minimo edifici da alimentare base
    int TX_base;  // Massimo edifici alimentabili base
    int TR_base;  // Profitto per edificio alimentato base
    int TM;       // Minimo edifici da alimentare (modified by effects)
    int TX;       // Massimo edifici alimentabili (modified by effects)
    int TR;       // Profitto per edificio alimentato (modified by effects)
};

// Stato di una risorsa acquistata (per gestire ciclo di vita, attivazione, ecc.)
struct StatoRisorsa {
    Risorsa risorsa;
    int turniRimanenti;      // Turni rimanenti nell'attuale stato (attivo o downtime)
    int cicloVitaRimanente;  // Turni rimanenti del ciclo di vita totale
    bool attiva;             // true se la risorsa è attualmente attiva
    int accumulatore;        // Per risorse di tipo E (accumulatore), surplus memorizzato (not used anymore)
    int rl_modificato;      // RL modificato da effetto C
};

// Aggiorna il budget in base a costi e profitto (ora usa la variabile globale budget)
void aggiornaBudget(int costoAttivazioni, int costoManutenzione, int profitto) {
    budget = budget - costoAttivazioni - costoManutenzione + profitto;
}

// Applica gli effetti speciali delle risorse attive
void applicaEffettiSpeciali(const vector<StatoRisorsa>& risorseAttive, Turno &turno, vector<int>& RU_list) {
    double TM_multiplier = 1.0;
    double TX_multiplier = 1.0;
    double TR_multiplier = 1.0;
    double RU_multiplier_A_green = 1.0;
    double RU_multiplier_A_nongreen = 1.0;


    for (const auto& stato : risorseAttive) {
        if (!stato.attiva) continue;
        char tipo = stato.risorsa.RT;
        int effetto = stato.risorsa.RE;

        if (tipo == 'A') {
            if (effetto >= 0) { // Green
                RU_multiplier_A_green += (double)effetto / 100.0;
            } else { // Non-Green
                RU_multiplier_A_nongreen -= (double)abs(effetto) / 100.0;
            }
        } else if (tipo == 'B') {
            if (effetto >= 0) { // Green
                TM_multiplier += (double)effetto / 100.0;
                TX_multiplier += (double)effetto / 100.0;
            } else { // Non-Green
                TM_multiplier -= (double)abs(effetto) / 100.0;
                TX_multiplier -= (double)abs(effetto) / 100.0;
            }
        }  else if (tipo == 'D') {
            if (effetto >= 0) { // Green
                TR_multiplier += (double)effetto / 100.0;
            } else { // Non-Green
                TR_multiplier -= (double)abs(effetto) / 100.0;
            }
        }
        // Effect C is handled at resource purchase time, no dynamic effect here.
        // Effect E (Accumulator) capacity is updated in simulaTurno. Effect E surplus usage is in profit calculation.
    }

    turno.TM = max(0, (int)floor(turno.TM_base * TM_multiplier));
    turno.TX = max(0, (int)floor(turno.TX_base * TX_multiplier));
    turno.TR = max(0, (int)floor(turno.TR_base * TR_multiplier));

    for (int i = 0; i < RU_list.size(); ++i) {
        RU_list[i] = max(0, (int)floor(RU_list[i] * RU_multiplier_A_green * RU_multiplier_A_nongreen));
    }
}


// Calcola il profitto del turno corrente
int calcolaProfitto(int turnoIndex, Turno &turno, vector<StatoRisorsa>& risorseAttive, vector<Risorsa>& risorseDisponibili) {
    int edificiAlimentati = 0;
    vector<int> RU_list_active_resources;
    vector<int> accumulator_indices;

    for (size_t i = 0; i < risorseAttive.size(); ++i) {
        if (risorseAttive[i].attiva) {
            RU_list_active_resources.push_back(risorseAttive[i].risorsa.RU);
        }
    }

    applicaEffettiSpeciali(risorseAttive, turno, RU_list_active_resources);

    int ru_index = 0;
    for (size_t i = 0; i < risorseAttive.size(); ++i) {
        if (risorseAttive[i].attiva) {
            risorseAttive[i].risorsa.RU = RU_list_active_resources[ru_index++]; // Update RU after effect A
            edificiAlimentati += risorseAttive[i].risorsa.RU;
        }
    }

    int profitto = 0;
    
    if (edificiAlimentati < turno.TM){
        int deficit = turno.TM - edificiAlimentati;
        if(deficit <= globalBattery.surplusGained){
            edificiAlimentati += deficit;
            globalBattery.surplusGained-=deficit;
            profitto = min(edificiAlimentati, turno.TX) * turno.TR;
        }
    }else{
        profitto = min(edificiAlimentati, turno.TX) * turno.TR;
    }

    // Accumulate surplus for type E resources
    if (edificiAlimentati > turno.TX) { // Only accumulate if profit is generated (TM is met)
        int surplusGained = edificiAlimentati - turno.TX;
        if (globalBattery.surplusGained + surplusGained > globalBattery.capacity){
            globalBattery.surplusGained = globalBattery.capacity;
        }
        else {
            globalBattery.surplusGained += surplusGained;
        }
    }

    return profitto;
}


// Simula un singolo turno
void simulaTurno(int turnoIndex, Turno &turno, vector<StatoRisorsa>& risorseAttive, vector<Risorsa>& risorseDisponibili, ofstream& outputFile) {
    cout << "\nTurno " << turnoIndex << ":\n";
    cout << "Budget disponibile: " << budget << "\n";
    cout << "Battery Surplus: " << globalBattery.surplusGained << ", Capacity: " << globalBattery.capacity << "\n";


    turno.TM = turno.TM_base; // Reset TM, TX, TR for effects in each turn
    turno.TX = turno.TX_base;
    turno.TR = turno.TR_base;

    // 1. Acquisto risorse (Esempio di acquisto fisso per testing output)
    vector<int> risorseAcquistateIds;
    int costoAttivazioni = 0;
    if (turnoIndex == 0) {
        risorseAcquistateIds = {5};
    } else if (turnoIndex == 1) {
        risorseAcquistateIds = {2};
    } else if (turnoIndex == 2) {
        risorseAcquistateIds = {2};
    } else if (turnoIndex == 4) {
        risorseAcquistateIds = {2, 2};
    } else if (turnoIndex == 5) {
        risorseAcquistateIds = {2};
    }


    vector<StatoRisorsa> risorseTurnoAcquistate;
    for (int id : risorseAcquistateIds) {
        for (const auto& risorsa_def : risorseDisponibili) {
            if (risorsa_def.RI == id) {
                if (budget >= risorsa_def.RA) {
                    costoAttivazioni += risorsa_def.RA;
                    StatoRisorsa nuovaRisorsaStato;
                    nuovaRisorsaStato.risorsa = risorsa_def;
                    nuovaRisorsaStato.turniRimanenti = nuovaRisorsaStato.risorsa.RW;
                    nuovaRisorsaStato.cicloVitaRimanente = nuovaRisorsaStato.risorsa.RL;
                    nuovaRisorsaStato.attiva = true;
                    nuovaRisorsaStato.accumulatore = 0; // Not used anymore
                    nuovaRisorsaStato.rl_modificato = nuovaRisorsaStato.risorsa.RL;

                    //Effect C - modify RL at purchase time if a type C resource is active
                    for(const auto& active_res : risorseAttive) {
                        if(active_res.attiva && active_res.risorsa.RT == 'C') {
                            if (active_res.risorsa.RE >= 0) { // Green
                                nuovaRisorsaStato.rl_modificato = max(1, (int)floor(nuovaRisorsaStato.rl_modificato * (1.0 + (double)active_res.risorsa.RE / 100.0)));
                            } else { // Non-Green
                                nuovaRisorsaStato.rl_modificato = max(1, (int)floor(nuovaRisorsaStato.rl_modificato * (1.0 - (double)abs(active_res.risorsa.RE) / 100.0)));
                            }
                            nuovaRisorsaStato.cicloVitaRimanente = nuovaRisorsaStato.rl_modificato; //update remaining life too.
                            break; // Assuming only one active C type resource influences purchase.
                        }
                    }

                    risorseTurnoAcquistate.push_back(nuovaRisorsaStato);
                    break; // Found the resource, no need to continue searching
                } else {
                    cout << "Budget insufficient to buy resource " << id << endl;
                }
            }
        }
    }

    // Update Battery capacity at the start of each turn based on active E resources
    globalBattery.capacity = 0;
    for (const auto &stato : risorseAttive) {
        if (stato.attiva && stato.risorsa.RT == 'E') {
            globalBattery.capacity += stato.risorsa.RE;
        }
    }

    if (budget >= costoAttivazioni) {
         aggiornaBudget(costoAttivazioni, 0, 0); // Deduct activation costs immediately
         for(auto& acquired_res : risorseTurnoAcquistate) {
            risorseAttive.push_back(acquired_res);
         }

        if (!risorseAcquistateIds.empty()) {
            outputFile << turnoIndex;
            outputFile << " " << risorseAcquistateIds.size();
            for (int id : risorseAcquistateIds) {
                outputFile << " " << id;
            }
            outputFile << endl;
            cout << "Acquistate risorse: ";
            for (int id : risorseAcquistateIds) {
                cout << id << " ";
            }
            cout << endl;
        } else {
            cout << "Nessun acquisto in questo turno.\n";
        }

    } else {
        cout << "Acquisto non valido per budget insufficiente.\n";
        costoAttivazioni = 0; // Reset costs if purchase invalid
    }


    // 2. Calcola i costi di manutenzione per le risorse attive
    int costoManutenzione = 0;
    for (const auto &stato : risorseAttive) {
        if (stato.attiva) {
            costoManutenzione += stato.risorsa.RP;
        }
    }

    // 3. Calcola il profitto
    int profitto = calcolaProfitto(turnoIndex, turno, risorseAttive, risorseDisponibili);

    cout << "Profitto turno: " << profitto << "\n";
    cout << "Costo manutenzione: " << costoManutenzione << "\n";

    // 4. Aggiorna il budget per il turno successivo (manutenzione sottratta qui, attivazione gia' sottratta)
    aggiornaBudget(0, costoManutenzione, profitto);
    cout << "Budget aggiornato: " << budget << "\n";

    // 5. Aggiorna lo stato delle risorse (ciclo di vita, attivazione/downtime)
    for (auto it = risorseAttive.begin(); it != risorseAttive.end(); ) {
        it->cicloVitaRimanente--;
        if (it->cicloVitaRimanente <= 0) {
            if (it->risorsa.RT == 'E') {
                // globalBattery.surplusGained = 0; // Do not reset global surplus when one E resource dies.
                globalBattery.capacity -= it->risorsa.RE; // Reduce capacity
                globalBattery.capacity = max(0, globalBattery.capacity); // Ensure capacity is not negative
                globalBattery.surplusGained = min(globalBattery.surplusGained, globalBattery.capacity); // Cap surplus again.
            }
            it = risorseAttive.erase(it); // Resource becomes obsolete and is removed
            continue; // Skip to next resource after erase
        }

        if (it->turniRimanenti > 0) {
            it->turniRimanenti--;
        } else {
            // Passa dallo stato attivo al downtime e viceversa
            it->attiva = !it->attiva;
            if (it->attiva) {
                it->turniRimanenti = it->risorsa.RW;
            } else {
                it->turniRimanenti = it->risorsa.RM;
            }
        }
        ++it;
    }
}


// Funzione che gestisce l'intera simulazione del gioco
void simulaGioco(int budgetIniziale, const vector<Turno>& turni, vector<Risorsa>& risorseDisponibili, ofstream& outputFile) {
    budget = budgetIniziale; // Initialize global budget
    globalBattery = Battery(); // Initialize global battery

    // Stato delle risorse acquistate (inizialmente vuoto)
    vector<StatoRisorsa> risorseAttive;

    // Ciclo sui turni
    for (size_t t = 0; t < turni.size(); ++t) {
        simulaTurno(t, const_cast<Turno&>(turni[t]), risorseAttive, risorseDisponibili, outputFile); //Pass turn as non-const to allow modification of TM, TX, TR by effects.
    }

    cout << "\nSimulazione terminata. Budget finale: " << budget << "\n";
    cout << "Surplus finale in batteria: " << globalBattery.surplusGained << "\n";
}

int main() {
    ifstream inputFile("0-demo.txt"); // File di input
    if (!inputFile) {
        cerr << "Errore nell'apertura del file!" << endl;
        return 1;
    }

    ofstream outputFile("output.txt"); // File di output
    if (!outputFile) {
        cerr << "Errore nell'apertura del file di output!" << endl;
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
        inputFile >> turni[i].TM_base >> turni[i].TX_base >> turni[i].TR_base;
        turni[i].TM = turni[i].TM_base;
        turni[i].TX = turni[i].TX_base;
        turni[i].TR = turni[i].TR_base;
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
        cout << "Turno " << i << " -> TM: " << turni[i].TM_base
             << ", TX: " << turni[i].TX_base << ", TR: " << turni[i].TR_base << "\n";
    }

    // Avvia la simulazione del gioco
    simulaGioco(D, turni, risorse, outputFile);

    outputFile.close();
    return 0;
}