#include <iostream>
#include <fstream>
#include <vector>
#include <string>

struct Risorsa {
    int RI, RA, RP, RW, RM, RL, RU;
    char RT;
    int RE;
};

struct Turno {
    int TM, TX, TR;
};

int main() {
    std::ifstream inputFile("input.txt"); // Nome del file di input
    if (!inputFile) {
        std::cerr << "Errore nell'apertura del file!" << std::endl;
        return 1;
    }

    int D, R, T;
    inputFile >> D >> R >> T; // Legge i valori iniziali
    
    std::vector<Risorsa> risorse(R);
    for (int i = 0; i < R; ++i) {
        inputFile >> risorse[i].RI >> risorse[i].RA >> risorse[i].RP >> risorse[i].RW
                  >> risorse[i].RM >> risorse[i].RL >> risorse[i].RU >> risorse[i].RT;
        if (risorse[i].RT != 'X') { // Se ha effetto speciale, legge il parametro RE
            inputFile >> risorse[i].RE;
        } else {
            risorse[i].RE = 0; // Nessun effetto speciale
        }
    }
    
    std::vector<Turno> turni(T);
    for (int i = 0; i < T; ++i) {
        inputFile >> turni[i].TM >> turni[i].TX >> turni[i].TR;
    }
    
    inputFile.close();
    
    // Output per verificare la corretta lettura
    std::cout << "Budget iniziale: " << D << "\nNumero risorse: " << R << "\nNumero turni: " << T << "\n";
    std::cout << "\nElenco risorse:\n";
    for (const auto& r : risorse) {
        std::cout << "ID: " << r.RI << ", Costo attivazione: " << r.RA
                  << ", Costo manutenzione: " << r.RP << ", Turni attivi: " << r.RW
                  << ", Turni inattivi: " << r.RM << ", Ciclo di vita: " << r.RL
                  << ", Edifici supportati: " << r.RU << ", Tipo effetto: " << r.RT
                  << ", Effetto RE: " << r.RE << "\n";
    }
    
    std::cout << "\nElenco turni:\n";
    for (const auto& t : turni) {
        std::cout << "Min edifici: " << t.TM << ", Max edifici: " << t.TX
                  << ", Profitto per edificio: " << t.TR << "\n";
    }
    
    return 0;
}
