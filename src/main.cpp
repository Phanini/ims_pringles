/**
 * @file    main.cpp
 * @author  Anastasiia Chumachenko - xchuma00
 * @author  Jakub Phan - xphanj00
 * @brief   Main file controling the simulation.
 * @date    2025-11-01
 * 
 * This file contains the main function and initialization logic for the program.
 */

 /* TODO LIST:
    - Implement potato mass lost during the process of manufacturing based on documentation (the constants are defined lower)
    - Implement timed transitions based on Petri's net diagram
    - DEBATE: Do we still wanna go for different type of moisture?
 */

#include <iostream>
#include <simlib.h>

//CONSTANTS FOR MASS LOST DURING PROCEDURE
double M_PEEL = 0.9;
double M_CUT = 0.9;
double M_DRY = 0.8;
double M_FRY = 0.6;

Store Washer("Washer", 1);
Store Peeler("Peeler", 1);
Store Blancher("Blancher", 1);
Store Cutter("Cutter", 1);
Store Dryer("Dryer", 1);
Store Fryer("Fryer", 1);

Store SeasonerA("Seasoner", 1);
Store SeasonerB("Seasoner", 1);
Store SeasonerC("Seasoner", 1);
Store SeasonerD("Seasoner", 1);

Store PackagerA("Packager", 1);
Store PackagerB("Packager", 1);
Store PackagerC("Packager", 1);
Store PackagerD("Packager", 1);

// STATISTICS
Histogram hTotalTime("Total time per potato batch", 0, 50, 25);
Stat sProcessed("Processed potatoes");
Stat sSpoiled("Spoiled potatoes");


class Potato : public Process {
    private:
        double weight;
        bool isSpoiled;
        double moisture; // Moisture content of potatoes will vary the drying time

    public:
        Potato(double w, bool spoiled, double m) : weight(w), isSpoiled(spoiled), moisture(m) {}
    
        void Behavior() override {
             double startTime = Time;  // Start timing the batch

            // Track spoiled potatoes
            if (isSpoiled) sSpoiled(1);

            // Initial processing steps
            Enter(Washer, 1);      Wait(Uniform(0.5, 1.0));      Leave(Washer, 1);
            Enter(Peeler, 1);      Wait(Uniform(0.5, 1.0));      Leave(Peeler, 1);
            Enter(Blancher, 1);    Wait(Uniform(0.5, 1.0));      Leave(Blancher, 1);
            Enter(Cooker, 1);      Wait(Uniform(1.0, 2.0));      Leave(Cooker, 1);

            // Drying (time varies based on moisture content)
            Enter(Dryer, 1);
            double dryingTime = 5.0 + (moisture * 2.0); // Random formula for drying time TODO
            Wait(dryingTime);
            Leave(Dryer, 1); 

            Enter(Fryer, 1);       Wait(Uniform(1.0, 2.0));      Leave(Fryer, 1);


            // Seasoning (random selection)
            double r = Random();
            if (r < 0.25) { Enter(SeasonerA, 1); Wait(2.0); Leave(SeasonerA, 1); }
            else if (r < 0.5) { Enter(SeasonerB, 1); Wait(2.0); Leave(SeasonerB, 1); }
            else if (r < 0.75) { Enter(SeasonerC, 1); Wait(2.0); Leave(SeasonerC, 1); }
            else { Enter(SeasonerD, 1); Wait(2.0); Leave(SeasonerD, 1); }

            // Packaging (random selection)
            r = Random();
            if (r < 0.25) { Enter(PackagerA, 1); Wait(3.0); Leave(PackagerA, 1); }
            else if (r < 0.5) { Enter(PackagerB, 1); Wait(3.0); Leave(PackagerB, 1); }
            else if (r < 0.75) { Enter(PackagerC, 1); Wait(3.0); Leave(PackagerC, 1); }
            else { Enter(PackagerD, 1); Wait(3.0); Leave(PackagerD, 1); }

            // Update statistics
            hTotalTime(Time - startTime);
            sProcessed(1);
        }
};

class PotatoGenerator : public Event {
    void Behavior() override {
        double weight = 100 + Random() * 200; // Weight between 100g and 300g
        bool isSpoiled = (Random() < 0.1); // 10% chance of being spoiled
        double moisture = 0.1 + Random() * 0.4; // Moisture content between 10% and 50%

        Potato* potato = new Potato(weight, isSpoiled, moisture);
        potato->Activate();

        // Schedule next potato arrival
        Activate(Time + Exponential(10.0)); // Average inter-arrival time of 10 time units
    }
};


int main() {
    std::cout << "=== IMS Potato Chip Production Simulation ===" << std::endl;

    // Initialize
    Init(0, 1000); // simulate 1000 time units

    // Start first generator
    (new PotatoGenerator)->Activate();

    // Run simulation
    Run();

    // Print results
    std::cout << "\nSimulation complete.\n";

    // Output statistics
    std::cout << "\n--- Summary ---\n";
    std::cout << "Processed potatoes:\n "; sProcessed.Output();
    std::cout << "Spoiled potatoes: \n"; sSpoiled.Output();
    std::cout << "Processing time distribution: \n"; hTotalTime.Output();

    // Utilization of all stores
    Washer.Output(); Peeler.Output(); Blancher.Output(); Cutter.Output();
    Cooker.Output(); Masher.Output(); Dryer.Output(); Miller.Output();
    Mixer.Output(); Roller.Output(); DoughCutter.Output(); Fryer.Output();
    SeasonerA.Output(); SeasonerB.Output(); SeasonerC.Output(); SeasonerD.Output();
    PackagerA.Output(); PackagerB.Output(); PackagerC.Output(); PackagerD.Output();

    return 0;
}