/**
 * @file    main.cpp
 * @author  Anastasiia Chumachenko - xchuma00
 * @author  Jakub Phan - xphanj00
 * @brief   Main file controling the simulation.
 * @date    2025-11-01
 */


#include <iostream>
#include <simlib.h>
#include <iomanip>
#include <string>

// SWITCHES FOR IMPROVEMENTS
bool IMPROVEMENT_FRYER = true;
bool AUTOMATED_QC = true;   // true = machine vision, false = human inspector

// CONSTANTS
const double MEAN_INTERARRIVAL = 0.075;
const double SIM_TIME = 3600.0;

const double M_PEEL = 0.90;
const double M_CUT  = 0.90;
const double M_DRY  = 0.75;
const double M_FRY  = 0.5;

const double P_DEFECT_TRUE      = 0.02; // 2% of potatoes have defects
const double AUTO_ACCURACY      = 0.995; 
const double AUTO_QC_TIME       = 0.15;

// STATISTICS
Stat sQCInspected("QC: inspected");
Stat sTotalBeforeQC("QC: TOTAL before QC");
Stat sTotalAFTERQC("QC: TOTAL that were QC or bypassed");
Stat sQCRejected("QC: rejections");
Stat sQCEscapes("QC: defects escaped (undetected)");
double finalmass = 0.0;


// STORES 
Store Washer("Washer", 400);
Store Peeler("Peeler", 534);
Store Cutter("Cutter", 67);
Store Dryer("Dryer", 70000);    
Store Fryer("Fryer", 225000);     
Store FryerA("FryerA", 112500);  
Store FryerB("FryerB", 112500);  
Store SeasonerA("SeasonerA", 800);
Store SeasonerB("SeasonerB", 800);
Store QCInspectorA("QC_A", 1);
Store QCInspectorB("QC_B", 1);
Store PackagerA("Packager", 800);
Store PackagerB("Packager", 800);

// STATISTICS 
Histogram hTotalTime("Total time per potato batch", 0, 1200, 24);
Stat sProcessed("Processed (good) potatoes");
Stat sSpoiled("Spoiled potatoes");
Stat sPotatoesGenerated("Total potatoes generated");

// PROCESSES
class FryerBatch : public Process {
public:
    static double current_batch_mass;
    static const double BATCH_TARGET;   // in grams
    static Queue waiting;               // potatoes waiting for a batch
    static bool active;                 // whether fryer controller is running

    void Behavior() override {
        active = true;

        // Wait until enough mass has accumulated
        while (current_batch_mass < BATCH_TARGET) {
            Passivate(); // wait until some potato wakes us
        }

        // Start frying 
        Enter(Fryer, BATCH_TARGET);
        Wait(180);                    // Fry for 3 minutes
        Leave(Fryer, BATCH_TARGET);

        // Reactivate ALL potatoes in this batch
        while (!waiting.Empty()) {
            Process *p = (Process*) waiting.GetFirst();
            p->Activate();
        }

        // Reset batch
        current_batch_mass = 0;
        active = false;
    }
};

double FryerBatch::current_batch_mass = 0;
const double FryerBatch::BATCH_TARGET = 225000.0; // 225 kg
Queue FryerBatch::waiting;
bool FryerBatch::active = false;


class FryerBatchA : public Process {
public:
    static double current_batch_mass;
    static const double BATCH_TARGET;
    static Queue waiting;
    static bool active;

    void Behavior() override {
        active = true;

        while (current_batch_mass < BATCH_TARGET)
            Passivate();

        Enter(FryerA, BATCH_TARGET);
        Wait(180);
        Leave(FryerA, BATCH_TARGET);

        while (!waiting.Empty()) {
            Process *p = (Process*) waiting.GetFirst();
            p->Activate();
        }

        current_batch_mass = 0;
        active = false;
    }
};


double FryerBatchA::current_batch_mass = 0;
const double FryerBatchA::BATCH_TARGET = 112500.0; // HALF of 225kg
Queue FryerBatchA::waiting;
bool FryerBatchA::active = false;

class FryerBatchB : public Process {
public:
    static double current_batch_mass;
    static const double BATCH_TARGET;
    static Queue waiting;
    static bool active;

    void Behavior() override {
        active = true;

        while (current_batch_mass < BATCH_TARGET)
            Passivate();

        Enter(FryerB, BATCH_TARGET);
        Wait(180);
        Leave(FryerB, BATCH_TARGET);

        while (!waiting.Empty()) {
            Process *p = (Process*) waiting.GetFirst();
            p->Activate();
        }

        current_batch_mass = 0;
        active = false;
    }
};

double FryerBatchB::current_batch_mass = 0;
const double FryerBatchB::BATCH_TARGET = 112500.0;
Queue FryerBatchB::waiting;
bool FryerBatchB::active = false;


class Potato : public Process {
private:
    double weight;
    bool isSpoiled;

public:
    Potato(bool spoiled, double weight)
        : weight(weight), isSpoiled(spoiled) {}

    void Behavior() override {
        double startTime = Time;

        // Washing
        Enter(Washer, 1); Wait(Uniform(20,40)); Leave(Washer,1);

        // Peeling
        Enter(Peeler, 1); Wait(Uniform(20,60)); Leave(Peeler,1);
        weight *= M_PEEL;

        // Upon peeling 5% of potatoes are found to be spoiled
        if (Random() < 0.05) {
            sSpoiled(1);
            return;
        }

        // Cutting
        Enter(Cutter,1); Wait(5); Leave(Cutter,1);
        weight *= M_CUT;

        // Drying
        Enter(Dryer, weight);
        Wait(Uniform(30,60));
        Leave(Dryer, weight);
        weight *= M_DRY;

        // Frying - IMPROVEMENT_FRYER determines if improvement of fryer is used
        if (!IMPROVEMENT_FRYER) {
            // Fry 225kg batches
            FryerBatch::current_batch_mass += weight;

            // Enter waiting queue
            Into(FryerBatch::waiting);

            // If fryer not active & enough mass -> wake it
            if (!FryerBatch::active &&
                FryerBatch::current_batch_mass >= FryerBatch::BATCH_TARGET) 
            {
                FryerBatch::active = true;
                FryerBatch *fb = new FryerBatch;
                fb->Activate();
            }
            // Wait until fryer batch controller wakes us
            Passivate();
        } else {
            bool A_can_fill = !FryerBatchA::active;
            bool B_can_fill = !FryerBatchB::active;

            if (A_can_fill && !B_can_fill) {
                // Fill A only
                FryerBatchA::current_batch_mass += weight;
                Into(FryerBatchA::waiting);

                if (FryerBatchA::current_batch_mass >= FryerBatchA::BATCH_TARGET) {
                    FryerBatchA::active = true;
                    (new FryerBatchA)->Activate();
                }

            } 
            else if (!A_can_fill && B_can_fill) {
                // Fill B only
                FryerBatchB::current_batch_mass += weight;
                Into(FryerBatchB::waiting);

                if (FryerBatchB::current_batch_mass >= FryerBatchB::BATCH_TARGET) {
                    FryerBatchB::active = true;
                    (new FryerBatchB)->Activate();
                }

            } 
            else {
                // Both inactive OR both active -> prioritize queue closer to completing a batch
                if ((FryerBatchA::waiting.Length() >= FryerBatchB::waiting.Length() && ((FryerBatchA::current_batch_mass + weight) < FryerBatchA::BATCH_TARGET))) {
                    FryerBatchA::current_batch_mass += weight;
                    Into(FryerBatchA::waiting);

                    if (!FryerBatchA::active && FryerBatchA::current_batch_mass >= FryerBatchA::BATCH_TARGET) {
                        FryerBatchA::active = true;
                        (new FryerBatchA)->Activate();
                    }
                } 
                else {
                    FryerBatchB::current_batch_mass += weight;
                    Into(FryerBatchB::waiting);

                    if (!FryerBatchB::active && FryerBatchB::current_batch_mass >= FryerBatchB::BATCH_TARGET) {
                        FryerBatchB::active = true;
                        (new FryerBatchB)->Activate();
                    }
                }
            }
            Passivate();
        }

        // After frying
        weight *= M_FRY;

        // Seasoning
        double r = Random();
        double seasonTime = Uniform(1,2);
        if (r < 0.5) {
            Enter(SeasonerA, weight); Wait(seasonTime); Leave(SeasonerA, weight);
        } else {
            Enter(SeasonerB, weight); Wait(seasonTime); Leave(SeasonerB, weight);
        } 

        // Quality Control 
        sTotalBeforeQC(1);

        bool is_defect = (Random() < P_DEFECT_TRUE);   // 2% of chips are defective

        // Manual inspection
        if (!AUTOMATED_QC) {
            Store* chosenInspector;
            if (r < 0.5) chosenInspector = &QCInspectorA;
            else chosenInspector = &QCInspectorB;

            // If successfully enter inspector - begin check
            if (chosenInspector->Free() > 0) {
                Enter(*chosenInspector, 1);
                sQCInspected(1);

                Wait(1);
                Leave(*chosenInspector, 1);

                if (is_defect) {
                    sQCRejected(1);
                    return;   // Removed from line
                }
            }
            else {
                // Inspector busy -> chip is NOT inspected
                if (is_defect) {
                    sQCEscapes(1);   // defect escaped due to not being chosen for inspection
                }
            }
        } else {
            // Automated system QC
            sQCInspected(1);

            Store* qcLine = nullptr;
            if (r < 0.5) {
                qcLine = &QCInspectorA;
            } else {
                qcLine = &QCInspectorB;
            } 

            Enter(*qcLine, 1);
            Wait(AUTO_QC_TIME);
            Leave(*qcLine, 1);

            bool detected = (Random() < AUTO_ACCURACY); // 99.5% accurate machine detection

            if (is_defect && detected) {
                sQCRejected(1);
                return;   // removed by machine
            }

            if (is_defect && !detected) {
                sQCEscapes(1);   // extremely rare miss
            }
        }

        sTotalAFTERQC(1);

        // Packaging
        double packTime =Uniform(2,3);
        if (r < 0.5) {
            Enter(PackagerA, weight); Wait(packTime); Leave(PackagerA, weight);
        } else {
            Enter(PackagerB, weight); Wait(packTime); Leave(PackagerB, weight);
        }

        // End
        sProcessed(1);
        hTotalTime(Time - startTime);
        finalmass += weight;
    }
};

// Event
class PotatoGenerator : public Event {
    void Behavior() override {
        double weight = Uniform(100, 200);
        (new Potato(false, weight))->Activate();
        sPotatoesGenerated(1);
        Activate(Time + Exponential(MEAN_INTERARRIVAL));
    }
};


int main(int argc, char *argv[]) {
    std::cout << "=== IMS Potato Chip Production Simulation ===\n";

    // --- Parse CLI arguments ---
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <fryer_mode> <qc_mode>\n";
        std::cerr << "  fryer_mode:  basic | improved\n";
        std::cerr << "  qc_mode:     auto | manual\n";
        return 1;
    }

    std::string fryerMode = argv[1];
    std::string qcMode    = argv[2];

    // Fryer mode
    if (fryerMode == "basic") {
        IMPROVEMENT_FRYER = false;
    } else if (fryerMode == "improved") {
        IMPROVEMENT_FRYER = true;
    } else {
        std::cerr << "Invalid fryer_mode: " << fryerMode << "\n";
        std::cerr << "Allowed: basic | improved\n";
        return 1;
    }

    // QC mode
    if (qcMode == "auto") {
        AUTOMATED_QC = true;
    } else if (qcMode == "manual") {
        AUTOMATED_QC = false;
    } else {
        std::cerr << "Invalid qc_mode: " << qcMode << "\n";
        std::cerr << "Allowed: auto | manual\n";
        return 1;
    }

    std::cout << "Selected fryer mode: " 
              << (IMPROVEMENT_FRYER ? "improved (two fryers)" : "basic (single fryer)") 
              << "\n";
    std::cout << "Selected QC mode: " 
              << (AUTOMATED_QC ? "automatic QC" : "manual QC") 
              << "\n\n";

    // --- Simulation setup ---
    Init(0, SIM_TIME);
    (new PotatoGenerator)->Activate();
    Run();

    std::cout << "\nSimulation complete\n";
    
    sProcessed.Output();
    sSpoiled.Output();
    sPotatoesGenerated.Output();
    
    sQCInspected.Output();
    sQCRejected.Output();
    sQCEscapes.Output();

    Washer.Output(); 
    Peeler.Output(); 
    Cutter.Output();
    Dryer.Output(); 
    
    if (!IMPROVEMENT_FRYER){    
       std::cout << FryerBatch::waiting.Length() << " potatoes waiting for frying.\n";
        Fryer.Output();
        FryerBatch::waiting.Output();
    }
    else {
        std::cout << "A: " << FryerBatchA::waiting.Length()
                  << " B: " << FryerBatchB::waiting.Length()
                  << " potatoes waiting for frying combined AB.\n";
        FryerA.Output();
        FryerB.Output();
    }

    sTotalBeforeQC.Output();
    sTotalAFTERQC.Output();
    
    SeasonerA.Output(); 
    SeasonerB.Output();
    PackagerA.Output(); 
    PackagerB.Output(); 

    std::cout << std::fixed << std::setprecision(3)
              << "Final mass: " << finalmass/1000 << " kg\n";

    return 0;
}