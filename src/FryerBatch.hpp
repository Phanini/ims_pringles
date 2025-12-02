#ifndef FRYERBATCH_HPP
#define FRYERBATCH_HPP

#include <simlib.h>

// Forward declaration
extern Store Fryer;

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

#endif
