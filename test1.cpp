#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <cstdlib>

using namespace std;

struct Risorsa {
    int RI, RA, RP, RW, RM, RL, RU;
    char RT;
    int RE; // Per risorse con effetto speciale (se RT != 'X')
};

struct Turno {
    int TM, TX, TR;
};

// Struttura per rappresentare una istanza (acquisto) di una risorsa durante il gioco.
struct ResourceInstance {
    const Risorsa* res;   // Puntatore alla risorsa “template”
    int turnsUsed;        // Turni già consumati da questa istanza
    int stateRemaining;   // Turni residui nello stato corrente (attivo o downtime)
    bool active;          // true se attiva, false se in downtime
    int effectiveRL;      // Ciclo di vita effettivo (potrebbe essere modificato da effetto C)
};

int main() {
    // Lettura dell'input da file "input.txt"
    ifstream inputFile("input.txt");
    if (!inputFile) {
        cerr << "Errore nell'apertura del file!" << endl;
        return 1;
    }
    
    int D, R, T;
    inputFile >> D >> R >> T;
    
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
    
    vector<Turno> turni(T);
    for (int i = 0; i < T; ++i) {
        inputFile >> turni[i].TM >> turni[i].TX >> turni[i].TR;
    }
    inputFile.close();
    
    // Registrazione delle decisioni di acquisto per ogni turno.
    // Ogni elemento conterrà: [turno, numero di risorse acquistate, lista degli ID (RI) delle risorse]
    vector<vector<int>> purchasesPerTurn(T);
    
    // Stato di simulazione
    int budget = D;
    double globalAccumulator = 0.0; // Valore globale per l'accumulatore (tipo E)
    vector<ResourceInstance> instances; // Istanze acquistate (attive o in downtime)
    
    // Strategia base di acquisto: ogni turno si prova ad acquistare la risorsa a minor costo di attivazione.
    int minRA_index = 0;
    int minRA = risorse[0].RA;
    for (int i = 1; i < R; ++i) {
        if (risorse[i].RA < minRA) {
            minRA = risorse[i].RA;
            minRA_index = i;
        }
    }
    
    // Simulazione turno per turno
    for (int t = 0; t < T; ++t) {
        // --- 1. Inizio turno: acquisti ---
        // Se il budget consente, compra una copia della risorsa a minor RA.
        if (budget >= risorse[minRA_index].RA) {
            // Effetto C: se in questo turno sono attive risorse di tipo C, modificare RL per i nuovi acquisti.
            double c_effect = 0.0;
            for (auto& inst : instances) {
                if (inst.active && inst.res->RT == 'C') {
                    // Se RE > 0 allora la risorsa è "green", altrimenti "non-green" (già negativa)
                    c_effect += static_cast<double>(inst.res->RE) / 100.0;
                }
            }
            int effectiveRL_new = risorse[minRA_index].RL;
            if (c_effect != 0.0) {
                effectiveRL_new = max(1, static_cast<int>(floor(risorse[minRA_index].RL * (1.0 + c_effect))));
            }
            // Sottrai il costo di attivazione dal budget.
            budget -= risorse[minRA_index].RA;
            
            // Crea la nuova istanza della risorsa acquistata.
            ResourceInstance newInst;
            newInst.res = &risorse[minRA_index];
            newInst.turnsUsed = 0;
            newInst.active = true; // Inizia attiva
            newInst.stateRemaining = risorse[minRA_index].RW; // Durata iniziale del periodo attivo
            newInst.effectiveRL = effectiveRL_new;
            instances.push_back(newInst);
            
            // Registra l'acquisto per il turno corrente.
            purchasesPerTurn[t].push_back(risorse[minRA_index].RI);
        }
        
        // --- 2. Calcolo degli effetti speciali globali (dalle istanze attive) ---
        double total_A_effect = 0.0;
        double total_green_B_effect = 0.0, total_non_green_B_effect = 0.0;
        double total_green_D_effect = 0.0, total_non_green_D_effect = 0.0;
        bool accumulatorActive = false;
        
        for (auto& inst : instances) {
            if (inst.active) {
                char type = inst.res->RT;
                if (type == 'A') {
                    total_A_effect += static_cast<double>(inst.res->RE) / 100.0;
                } else if (type == 'B') {
                    if (inst.res->RE > 0)
                        total_green_B_effect += static_cast<double>(inst.res->RE) / 100.0;
                    else
                        total_non_green_B_effect += fabs(static_cast<double>(inst.res->RE)) / 100.0;
                } else if (type == 'D') {
                    if (inst.res->RE > 0)
                        total_green_D_effect += static_cast<double>(inst.res->RE) / 100.0;
                    else
                        total_non_green_D_effect += fabs(static_cast<double>(inst.res->RE)) / 100.0;
                } else if (type == 'E') {
                    accumulatorActive = true;
                }
            }
        }
        
        // Calcola i parametri del turno applicando gli effetti di tipo B e D.
        int base_TM = turni[t].TM;
        int base_TX = turni[t].TX;
        int base_TR = turni[t].TR;
        int effective_TM = max(0, static_cast<int>(floor(base_TM * (1.0 + total_green_B_effect - total_non_green_B_effect))));
        int effective_TX = max(0, static_cast<int>(floor(base_TX * (1.0 + total_green_B_effect - total_non_green_B_effect))));
        int effective_TR = max(0, static_cast<int>(floor(base_TR * (1.0 + total_green_D_effect - total_non_green_D_effect))));
        
        // --- 3. Calcolo della produzione totale ---
        int totalProduction = 0;
        // Per ogni istanza attiva (indipendentemente dal tipo), si somma la produzione.
        // L’effetto A incrementa il numero base RU per un fattore (1 + somma(RE di A)/100).
        for (auto& inst : instances) {
            if (inst.active) {
                int base_RU = inst.res->RU;
                int effective_RU = static_cast<int>(floor(base_RU * (1.0 + total_A_effect)));
                effective_RU = max(0, effective_RU);
                totalProduction += effective_RU;
            }
        }
        
        // --- 4. Uso dell'accumulatore (tipo E) ---
        if (accumulatorActive) {
            // Se la produzione eccede effective_TX, si accumula lo surplus.
            if (totalProduction > effective_TX) {
                globalAccumulator += (totalProduction - effective_TX);
            }
            // Se la produzione è inferiore a effective_TM, si cerca di compensare il deficit prelevando dall'accumulatore.
            if (totalProduction < effective_TM) {
                int shortfall = effective_TM - totalProduction;
                if (globalAccumulator >= shortfall) {
                    totalProduction = effective_TM;
                    globalAccumulator -= shortfall;
                }
            }
        } else {
            // Se nessun accumulatore è attivo, si resetta il valore globale.
            globalAccumulator = 0.0;
        }
        
        // --- 5. Calcolo del profitto del turno ---
        int profit = 0;
        if (totalProduction >= effective_TM) {
            profit = min(totalProduction, effective_TX) * effective_TR;
        }
        
        // --- 6. Calcolo dei costi periodici ---
        int maintenanceCost = 0;
        for (auto& inst : instances) {
            maintenanceCost += inst.res->RP;
        }
        
        // Aggiornamento del budget (si sottraggono i costi di manutenzione e si aggiunge il profitto)
        budget = budget - maintenanceCost + profit;
        
        // --- 7. Aggiornamento dello stato delle risorse ---
        vector<ResourceInstance> newInstances;
        for (auto& inst : instances) {
            inst.turnsUsed++;
            inst.stateRemaining--;
            // Se il ciclo di vita è terminato, l'istanza viene eliminata.
            if (inst.turnsUsed >= inst.effectiveRL)
                continue;
            // Se il tempo nello stato corrente è esaurito, si passa all'altro stato.
            if (inst.stateRemaining <= 0) {
                if (inst.active) {
                    // Passa in downtime
                    inst.active = false;
                    inst.stateRemaining = inst.res->RM;
                } else {
                    // Torna ad essere attiva
                    inst.active = true;
                    inst.stateRemaining = inst.res->RW;
                }
            }
            newInstances.push_back(inst);
        }
        instances = newInstances;
        
        // (Opzionale: qui potresti stampare un riepilogo del turno per debug)
        cout << "Turno " << t << ": Budget = " << budget 
             << ", Produzione = " << totalProduction
             << ", Profitto = " << profit 
             << ", Costo manutenzione = " << maintenanceCost << "\n";
    }
    
    // --- 8. Output delle decisioni di acquisto ---
    // Vengono scritti solo i turni in cui sono stati effettuati acquisti.
    ofstream outputFile("output.txt");
    for (int t = 0; t < T; ++t) {
        if (!purchasesPerTurn[t].empty()) {
            outputFile << t << " " << purchasesPerTurn[t].size();
            for (int id : purchasesPerTurn[t]) {
                outputFile << " " << id;
            }
            outputFile << "\n";
        }
    }
    outputFile.close();
    
    return 0;
}
