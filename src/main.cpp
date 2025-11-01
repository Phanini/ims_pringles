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
    - ALL the parameters are just made up numbers for Store, Enter and Leaves -> do research and find +- realistic values
 */

#include <iostream>
#include <simlib.h>

Store Washer("Washer", 100);
Store Peeler("Peeler", 100);
Store Blancher("Blancher", 100);
Store Cutter("Cutter", 100);
Store Cooker("Cooker", 100);
Store Masher("Masher", 100);
Store Dryer("Dryer", 100);
Store Miller("Miller", 100);
Store Mixer("Mixer", 100);
Store Roller("Roller", 100);
Store DoughCutter("DoughCutter", 100);
Store Fryer("Fryer", 100);

Store SeasonerA("Seasoner", 100);
Store SeasonerB("Seasoner", 100);
Store SeasonerC("Seasoner", 100);
Store SeasonerD("Seasoner", 100);

Store PackagerA("Packager", 100);
Store PackagerB("Packager", 100);
Store PackagerC("Packager", 100);
Store PackagerD("Packager", 100);

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
            Enter(Cutter, 1);      Wait(Uniform(0.3, 0.7));      Leave(Cutter, 1);
            Enter(Cooker, 1);      Wait(Uniform(1.0, 2.0));      Leave(Cooker, 1);
            Enter(Masher, 1);      Wait(Uniform(0.5, 1.5));      Leave(Masher, 1);

            // Drying (time varies based on moisture content)
            Enter(Dryer, 1);
            double dryingTime = 5.0 + (moisture * 2.0); // Random formula for drying time TODO
            Wait(dryingTime);
            Leave(Dryer, 1); 

            Enter(Miller, 1);      Wait(Uniform(0.5, 1.0));      Leave(Miller, 1);
            Enter(Mixer, 1);       Wait(Uniform(0.5, 1.0));      Leave(Mixer, 1);
            Enter(Roller, 1);      Wait(Uniform(0.5, 1.0));      Leave(Roller, 1);
            Enter(DoughCutter, 1); Wait(Uniform(0.5, 1.0));      Leave(DoughCutter, 1);
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