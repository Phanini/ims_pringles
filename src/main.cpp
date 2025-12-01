#include <iostream>
#include <simlib.h>
#include "FryerBatch.hpp"

// --------------------- MASS & TIMING CONSTANTS ---------------------
const double M_PEEL = 0.90;
const double M_CUT  = 0.90;
const double M_DRY  = 0.80;
const double M_FRY  = 0.40;

const double MEAN_INTERARRIVAL = 0.075;
const double SIM_TIME = 3600.0;

double finalmass = 0.0;

// --------------------------- STORES ---------------------------
Store Washer("Washer", 400);
Store Peeler("Peeler", 534);
Store Cutter("Cutter", 67);
Store Dryer("Dryer", 70000);      // 70 kg raw cut chips
Store Fryer("Fryer", 225000);     // 225 kg batch capacity

Store SeasonerA("Seasoner", 400);
Store SeasonerB("Seasoner", 400);
Store SeasonerC("Seasoner", 400);
Store SeasonerD("Seasoner", 400);

Store PackagerA("Packager", 1000);
Store PackagerB("Packager", 1000);
Store PackagerC("Packager", 1000);
Store PackagerD("Packager", 1000);

// ------------------------- STATISTICS -------------------------
Histogram hTotalTime("Total time per potato batch", 0, 1200, 24);
Stat sProcessed("Processed (good) potatoes");
Stat sSpoiled("Spoiled potatoes");
Stat sFinalMass("Final mass of processed potatoes");
Stat sPotatoesGenerated("Total potatoes generated");

// -------------------------- PROCESS ---------------------------
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

        // ---- Fryer batch accumulation ----
        FryerBatch::current_batch_mass += weight;

        // Enter waiting queue
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

        // After frying
        weight *= M_FRY;

        // Seasoning
        double r = Random();
        double seasonTime = Uniform(1,5);
        if (r < 0.25) {
            Enter(SeasonerA, weight); Wait(seasonTime); Leave(SeasonerA, weight);
        } else if (r < 0.5) {
            Enter(SeasonerB, weight); Wait(seasonTime); Leave(SeasonerB, weight);
        } else if (r < 0.75) {
            Enter(SeasonerC, weight); Wait(seasonTime); Leave(SeasonerC, weight);
        } else {
            Enter(SeasonerD, weight); Wait(seasonTime); Leave(SeasonerD, weight);
        }

        // Packaging
        double packTime = Uniform(5,10);
        r = Random();
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

    Init(0, SIM_TIME);
    (new PotatoGenerator)->Activate();
    Run();

    std::cout << "\nSimulation complete.\n";

    sProcessed.Output();
    sSpoiled.Output();
    sPotatoesGenerated.Output();
    std::cout << "Final mass: " << finalmass << " g\n";

    Washer.Output(); Peeler.Output(); Cutter.Output();
    Dryer.Output(); Fryer.Output();
    SeasonerA.Output(); SeasonerB.Output(); SeasonerC.Output(); SeasonerD.Output();
    PackagerA.Output(); PackagerB.Output(); PackagerC.Output(); PackagerD.Output();
}
