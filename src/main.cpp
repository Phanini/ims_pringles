#include <iostream>
#include <simlib.h>
#include <iomanip>

// --------------------- MASS & TIMING CONSTANTS ---------------------
const double M_PEEL = 0.90;
const double M_CUT  = 0.90;
const double M_DRY  = 0.80;
const double M_FRY  = 0.40;

const double MEAN_INTERARRIVAL = 0.075;
const double SIM_TIME = 3600.0;

int after_fry = 0;
int before_fry = 0;
double finalmass = 0.0;


// SWITCHES FOR IMPROVEMENTS
const bool IMPROVEMENT_FRYER = false;
const bool AUTOMATED_QC = true;   // true = machine vision, false = human inspector


//---------- QC------------
const double P_DEFECT_TRUE = 0.02; // 2% of potatoes have defects
// --------- MANUAL QC PARAMETERS ---------
const double MANUAL_SAMPLE_RATE = 0.05;    // 5% checked
const double MANUAL_ACCURACY    = 0.90;    // 90% detection of defects
const double MANUAL_QC_TIME_MIN = 1.0;
const double MANUAL_QC_TIME_MAX = 2.0;

// --------- AUTOMATED QC PARAMETERS ---------
const double AUTO_ACCURACY      = 0.995;   // 99.5% detection
const double AUTO_QC_TIME       = 0.15;    // very fast (seconds)



// Stats
Stat sQCInspected("QC: inspected");
Stat sTotalBeforeQC("QC: TOTAL before QC");
Stat sTotalAFTERQC("QC: TOTAL that were QC or bypassed");
Stat sQCRejected("QC: rejections");
Stat sQCNoDefect("QC: inspected but no defect found");
Stat sQCEscapes("QC: defects escaped (undetected)");


// --------------------------- STORES ---------------------------
Store Washer("Washer", 400);
Store Peeler("Peeler", 534);
Store Cutter("Cutter", 67);
Store Dryer("Dryer", 70000);      // 70 kg raw cut chips
Store Fryer("Fryer", 225000);     // 225 kg batch capacity
Store FryerA("FryerA", 112500);   // Backup fryer
Store FryerB("FryerB", 112500);   // Backup fryer
Store SeasonerA("Seasoner", 400);
Store SeasonerB("Seasoner", 400);
Store SeasonerC("Seasoner", 400);
Store SeasonerD("Seasoner", 400);

Store QCInspectorA("QC_A", 1);
Store QCInspectorB("QC_B", 1);
Store QCInspectorC("QC_C", 1);
Store QCInspectorD("QC_D", 1);

Store PackagerA("Packager", 400);
Store PackagerB("Packager", 400);
Store PackagerC("Packager", 400);
Store PackagerD("Packager", 400);

// ------------------------- STATISTICS -------------------------
Histogram hTotalTime("Total time per potato batch", 0, 1200, 24);
Stat sProcessed("Processed (good) potatoes");
Stat sSpoiled("Spoiled potatoes");
Stat sFinalMass("Final mass of processed potatoes");
Stat sPotatoesGenerated("Total potatoes generated");

// -------------------------- PROCESS ---------------------------

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

        // ---- Start Frying ----
        Enter(Fryer, BATCH_TARGET);   // occupy fryer space
        Wait(180);                    // 3 minutes
        Leave(Fryer, BATCH_TARGET);   // release capacity

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


// ------------------------------

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

        // Spoilage
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
        Leave(Dryer, weight);   // <–– FIXED (was 1)
        weight *= M_DRY;

        if (!IMPROVEMENT_FRYER) {
            // ---- Fryer batch accumulation ----
            FryerBatch::current_batch_mass += weight;

            // Enter waiting queue
            before_fry += 1;
            Into(FryerBatch::waiting);

            // If fryer not active & enough mass → wake it
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
                // Both inactive OR both active → balance by smaller queue
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
        after_fry += 1;
        
        // Seasoning
        double r = Random();
        double seasonTime = Uniform(1,4);
        if (r < 0.25) {
            Enter(SeasonerA, weight); Wait(seasonTime); Leave(SeasonerA, weight);
        } else if (r < 0.5) {
            Enter(SeasonerB, weight); Wait(seasonTime); Leave(SeasonerB, weight);
        } else if (r < 0.75) {
            Enter(SeasonerC, weight); Wait(seasonTime); Leave(SeasonerC, weight);
        } else {
            Enter(SeasonerD, weight); Wait(seasonTime); Leave(SeasonerD, weight);
        }

       // ================= QUALITY CONTROL =================
        sTotalBeforeQC(1);
        bool is_defect = (Random() < P_DEFECT_TRUE);

        if (!AUTOMATED_QC) {
            // ========= MANUAL HUMAN INSPECTION =========
            bool inspected = (Random() < MANUAL_SAMPLE_RATE);

            if (inspected) {
                sQCInspected(1);

                // Choose inspector
                if (r < 0.25) {
                    Enter(QCInspectorA, 1); Wait(Uniform(MANUAL_QC_TIME_MIN, MANUAL_QC_TIME_MAX)); Leave(QCInspectorA, 1);
                } else if (r < 0.5) {
                    Enter(QCInspectorB, 1); Wait(Uniform(MANUAL_QC_TIME_MIN, MANUAL_QC_TIME_MAX)); Leave(QCInspectorB, 1);
                } else if (r < 0.75) {
                    Enter(QCInspectorC, 1); Wait(Uniform(MANUAL_QC_TIME_MIN, MANUAL_QC_TIME_MAX)); Leave(QCInspectorC, 1);
                } else {
                    Enter(QCInspectorD, 1); Wait(Uniform(MANUAL_QC_TIME_MIN, MANUAL_QC_TIME_MAX)); Leave(QCInspectorD, 1);
                }

                bool detected = (Random() < MANUAL_ACCURACY);

                if (is_defect && detected) {
                    sQCRejected(1);
                    return;   // removed
                }

                if (!is_defect && detected) {
                    sQCNoDefect(1); // false reject
                }

            } else {
                if (is_defect) sQCEscapes(1); // defect passed because not inspected
            }

        } else {
            // ========= AUTOMATED MACHINE INSPECTION =========
            sQCInspected(1);

            // Machine vision delay (very small)
            Wait(AUTO_QC_TIME);

            bool detected = (Random() < AUTO_ACCURACY);

            if (is_defect && detected) {
                sQCRejected(1);
                return;   // removed by machine
            }

            if (!is_defect && detected == false) {
                sQCEscapes(1); // extremely rare miss
            }
        }

        sTotalAFTERQC(1);


        // Packaging
        double packTime = Uniform(2,3);
        if (r < 0.25) {
            Enter(PackagerA, weight); Wait(packTime); Leave(PackagerA, weight);
        } else if (r < 0.5) {
            Enter(PackagerB, weight); Wait(packTime); Leave(PackagerB, weight);
        } else if (r < 0.75) {
            Enter(PackagerC, weight); Wait(packTime); Leave(PackagerC, weight);
        } else {
            Enter(PackagerD, weight); Wait(packTime); Leave(PackagerD, weight);
        }

        // End
        sProcessed(1);
        hTotalTime(Time - startTime);
        finalmass += weight;
        sFinalMass(weight);
    }
};

// --------------------------- EVENT ----------------------------
class PotatoGenerator : public Event {
    void Behavior() override {
        double weight = Uniform(100, 200);
        (new Potato(false, weight))->Activate();
        sPotatoesGenerated(1);
        Activate(Time + Exponential(MEAN_INTERARRIVAL));
    }
};

// ---------------------------- MAIN ----------------------------
int main() {
    std::cout << "=== IMS Potato Chip Production Simulation ===\n";
    RandomSeed(time(nullptr)); // Seed random number generator

    Init(0, SIM_TIME);
    (new PotatoGenerator)->Activate();
    Run();

    std::cout << "\nSimulation complete.\n";
    std::cout << std::setprecision(17) << "Final mass: " << finalmass/1000 << " kg\n";
    if (!IMPROVEMENT_FRYER){    
        std::cout << FryerBatch::waiting.Length() << " potatoes waiting for frying.\n";}
    else {
        std::cout << "A: " << FryerBatchA::waiting.Length() << "B: " << FryerBatchB::waiting.Length() << " potatoes waiting for frying combined AB.\n";}
    FryerA.Output();
    FryerB.Output();
    
        
    sProcessed.Output();
    sSpoiled.Output();
    sPotatoesGenerated.Output();
    sTotalBeforeQC.Output();
    sTotalAFTERQC.Output();
    sQCInspected.Output();
    sQCRejected.Output();
    sQCNoDefect.Output();
    sQCEscapes.Output();

    /*Washer.Output(); Peeler.Output(); Cutter.Output();
    Dryer.Output(); 

    Fryer.Output(); FryerBatch::waiting.Output();

    SeasonerA.Output(); SeasonerB.Output(); SeasonerC.Output(); SeasonerD.Output();
    PackagerA.Output(); PackagerB.Output(); PackagerC.Output(); PackagerD.Output();*/
}
