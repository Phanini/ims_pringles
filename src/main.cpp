    /**
     * @file    main.cpp
     * @author  Anastasiia Chumachenko - xchuma00
     * @author  Jakub Phan - xphanj00
     * @brief   Main file controling the simulation.
     * @date    2025-11-01
     */

    #include <iostream>
    #include <simlib.h>
/*
    todo:
    - Add spoiled check after seasoning?
    - adjust Store capacities to grams
*/


    // --------------------- MASS & TIMING CONSTANTS ---------------------


    // Škrábání - 10 %, Krájení - 10 %, Sušení - 20 %, Smažení - 60 %
    const double M_PEEL = 0.90;
    const double M_CUT  = 0.90;
    const double M_DRY  = 0.80;
    const double M_FRY  = 0.40;

    // λ = 1/200 s^-1, на Fryer ρ ≈ 0.9
    const double MEAN_INTERARRIVAL = 0.075;

    const double SIM_TIME = 3600.0;

    double finalmass = 0.0;

    // --------------------------- STORES ---------------------------

    Store Washer("Washer", 400);
    Store Peeler("Peeler", 534);
    Store Cutter("Cutter", 67);
    Store Dryer("Dryer", 600);
    Store Fryer("Fryer", 2400);

    Store SeasonerA("Seasoner", 40);
    Store SeasonerB("Seasoner", 40);
    Store SeasonerC("Seasoner", 40);
    Store SeasonerD("Seasoner", 40);

    Store PackagerA("Packager", 100);
    Store PackagerB("Packager", 100);
    Store PackagerC("Packager", 100);
    Store PackagerD("Packager", 100);

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
        : weight(weight),
            isSpoiled(spoiled) {}

        void Behavior() override {
            double startTime = Time;

            // --- Washing: 20–40 s ---
            Enter(Washer, 1);
            Wait(Uniform(20.0, 40.0));
            Leave(Washer, 1);

            // --- Peeling: 20–60 s ---
            Enter(Peeler, 1);
            Wait(Uniform(20.0, 60.0));
            Leave(Peeler, 1);
            weight *= M_PEEL;

            // WTF HELP CHAT
            double r = Random();
            if (r < 0.05) {
                isSpoiled = true;
                sSpoiled(1);       // record spoiled
                return;            // remove from line
            }

            // --- Cutting: 5 s, -10 % weight ---
            Enter(Cutter, 1);
            Wait(5.0);
            Leave(Cutter, 1);
            weight *= M_CUT;

            // --- Drying: 30–60 s, -20 % weight ---
            Enter(Dryer, 1);
            double dryingTime = Uniform(30.0, 60.0);
            Wait(dryingTime);
            Leave(Dryer, 1);
            weight *= M_DRY;

            // --- Frying: 3 min = 180 s, -60 % weight ---
            Enter(Fryer, 1);
            Wait(180.0);
            Leave(Fryer, 1);
            weight *= M_FRY;

            // --- Seasoning: 1–5 s ---
            r = Random();
            double seasonTime = Uniform(1.0, 5.0);
            if (r < 0.25) {
                Enter(SeasonerA, 1); Wait(seasonTime); Leave(SeasonerA, 1);
            } else if (r < 0.5) {
                Enter(SeasonerB, 1); Wait(seasonTime); Leave(SeasonerB, 1);
            } else if (r < 0.75) {
                Enter(SeasonerC, 1); Wait(seasonTime); Leave(SeasonerC, 1);
            } else {
                Enter(SeasonerD, 1); Wait(seasonTime); Leave(SeasonerD, 1);
            }

            // --- Packaging: 5–10 s ---
            r = Random();
            double packTime = Uniform(5.0, 10.0);
            if (r < 0.25) {
                Enter(PackagerA, 1); Wait(packTime); Leave(PackagerA, 1);
            } else if (r < 0.5) {
                Enter(PackagerB, 1); Wait(packTime); Leave(PackagerB, 1);
            } else if (r < 0.75) {
                Enter(PackagerC, 1); Wait(packTime); Leave(PackagerC, 1);
            } else {
                Enter(PackagerD, 1); Wait(packTime); Leave(PackagerD, 1);
            }

            sProcessed(1);
            hTotalTime(Time - startTime);
            finalmass += weight;
            sFinalMass(weight);
        }
    };

    // --------------------------- EVENT ----------------------------

    class PotatoGenerator : public Event {
        void Behavior() override {
            bool isSpoiled = false;
            double weight = Uniform(100.0, 200.0);
            (new Potato(isSpoiled, weight))->Activate();
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

        std::cout << "\n--- Summary ---\n";
        std::cout << "Processed (good) potatoes:\n";
        sProcessed.Output();
        std::cout << "Spoiled potatoes:\n";
        sSpoiled.Output();
        std::cout << "Total potatoes generated:\n";
        sPotatoesGenerated.Output();
        
        std::cout << "Final mass of processed potatoes (kg):\n";
        //sFinalMass.Output();
        std::cout << finalmass << " kg\n";

        //std::cout << "Processing time distribution (good batches only):\n";
        //hTotalTime.Output();

        Washer.Output(); Peeler.Output(); 
        Cutter.Output();/*
        Dryer.Output(); Fryer.Output();
        SeasonerA.Output(); SeasonerB.Output(); SeasonerC.Output(); SeasonerD.Output();
        PackagerA.Output(); PackagerB.Output(); PackagerC.Output(); PackagerD.Output();
        */
        return 0;
    }
