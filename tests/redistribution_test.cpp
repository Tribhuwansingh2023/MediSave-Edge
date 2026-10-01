#include <iostream>
#include <cassert>
#include <string>
#include <vector>

#include "RedistributionEngine.h"

int main() {
    std::cout << "========================================\n";
    std::cout << "     REDISTRIBUTION TEST\n";
    std::cout << "========================================\n\n";

    // 1. No facilities
    {
        RedistributionEngine engine;
        auto recs = engine.generateRecommendations();
        assert(recs.empty());
        assert(engine.getFacilityCount() == 0);
    }

    // 2. One facility (surplus only)
    {
        RedistributionEngine engine;
        FacilityStock f1{"Facility-A", "MED001", "Paracetamol", "P2026A", 150, 50, 100, "2027-01-01"};
        engine.addFacilityStock(f1);

        assert(engine.getSurplusCount() == 1);
        assert(engine.getShortageCount() == 0);

        auto recs = engine.generateRecommendations();
        assert(recs.empty()); // No destination facility
    }
    std::cout << "[PASS] Surplus detection\n";

    // 3. Shortage only
    {
        RedistributionEngine engine;
        FacilityStock f2{"Facility-B", "MED001", "Paracetamol", "P2026A", 20, 50, 100, "2027-01-01"};
        engine.addFacilityStock(f2);

        assert(engine.getSurplusCount() == 0);
        assert(engine.getShortageCount() == 1);

        auto recs = engine.generateRecommendations();
        assert(recs.empty()); // No source facility
    }
    std::cout << "[PASS] Shortage detection\n";

    // 4. Matching surplus + shortage
    {
        RedistributionEngine engine;
        FacilityStock f1{"Facility-A", "MED001", "Paracetamol", "P2026A", 150, 50, 100, "2027-01-01"}; // Surplus = 100
        FacilityStock f2{"Facility-B", "MED001", "Paracetamol", "P2026A", 20, 50, 100, "2027-01-01"};  // Shortage = 30
        engine.addFacilityStock(f1);
        engine.addFacilityStock(f2);

        auto recs = engine.generateRecommendations();
        assert(recs.size() == 1);
        assert(recs[0].sourceFacility == "Facility-A");
        assert(recs[0].destinationFacility == "Facility-B");
        assert(recs[0].suggestedTransfer == 30); // min(100, 30) = 30
        assert(recs[0].sourceSurplus == 100);
        assert(recs[0].destinationShortage == 30);
    }
    std::cout << "[PASS] Facility matching\n";

    // 5. Transfer quantity bounds: never negative, zero, or greater than surplus
    {
        RedistributionEngine engine;
        // Source has surplus of 15, Destination needs 50
        FacilityStock f1{"Facility-A", "MED002", "Amoxicillin", "AMX10", 65, 50, 100, "2026-12-01"}; // Surplus = 15
        FacilityStock f2{"Facility-B", "MED002", "Amoxicillin", "AMX10", 0, 50, 100, "2026-12-01"};  // Shortage = 50
        engine.addFacilityStock(f1);
        engine.addFacilityStock(f2);

        auto recs = engine.generateRecommendations();
        assert(recs.size() == 1);
        assert(recs[0].suggestedTransfer == 15); // Cannot exceed surplus of 15
        assert(recs[0].suggestedTransfer > 0);
    }
    std::cout << "[PASS] Transfer quantity\n";

    // 6. Multiple facilities & multiple medicines
    {
        RedistributionEngine engine;
        // Facility-A has Paracetamol surplus = 100
        FacilityStock a_para{"Facility-A", "MED001", "Paracetamol", "P2026A", 150, 50, 100, "2027-01-01"};
        // Facility-B has Paracetamol shortage = 30
        FacilityStock b_para{"Facility-B", "MED001", "Paracetamol", "P2026A", 20, 50, 100, "2027-01-01"};
        // Facility-C has Paracetamol shortage = 40
        FacilityStock c_para{"Facility-C", "MED001", "Paracetamol", "P2026A", 10, 50, 100, "2027-01-01"};

        // Facility-B has Ibuprofen surplus = 50
        FacilityStock b_ibu{"Facility-B", "MED003", "Ibuprofen", "IBU01", 100, 50, 100, "2026-11-15"};
        // Facility-A has Ibuprofen shortage = 25
        FacilityStock a_ibu{"Facility-A", "MED003", "Ibuprofen", "IBU01", 25, 50, 100, "2026-11-15"};

        engine.addFacilityStock(a_para);
        engine.addFacilityStock(b_para);
        engine.addFacilityStock(c_para);
        engine.addFacilityStock(b_ibu);
        engine.addFacilityStock(a_ibu);

        auto recs = engine.generateRecommendations();
        assert(recs.size() >= 3);
        // Total transfer matches needs without over-allocating
        for (const auto& r : recs) {
            assert(r.suggestedTransfer > 0);
            assert(r.sourceFacility != r.destinationFacility);
        }
    }
    std::cout << "[PASS] Multiple facility handling\n";

    // 7. Deterministic recommendation ordering: larger shortage first
    {
        RedistributionEngine engine;
        FacilityStock src{"Facility-Source", "MED001", "Paracetamol", "P1", 200, 50, 100, "2027-01-01"}; // Surplus = 150
        FacilityStock destSmall{"Facility-Small", "MED001", "Paracetamol", "P1", 40, 50, 100, "2027-01-01"}; // Shortage = 10
        FacilityStock destLarge{"Facility-Large", "MED001", "Paracetamol", "P1", 10, 50, 100, "2027-01-01"}; // Shortage = 40

        engine.addFacilityStock(src);
        engine.addFacilityStock(destSmall);
        engine.addFacilityStock(destLarge);

        auto recs = engine.generateRecommendations();
        assert(recs.size() == 2);
        // Larger shortage (Facility-Large with shortage 40) must be matched first!
        assert(recs[0].destinationFacility == "Facility-Large");
        assert(recs[0].destinationShortage == 40);
        assert(recs[1].destinationFacility == "Facility-Small");
        assert(recs[1].destinationShortage == 10);
    }
    std::cout << "[PASS] Recommendation ordering\n";

    // 8. Invalid transfer prevention: no transfer when source == destination, no negative
    {
        RedistributionEngine engine;
        // Same facility cannot transfer to itself
        FacilityStock fSelf{"Facility-Local", "MED001", "Paracetamol", "P1", 50, 50, 100, "2027-01-01"}; // Normal
        engine.addFacilityStock(fSelf);

        auto recs = engine.generateRecommendations();
        assert(recs.empty());
    }
    std::cout << "[PASS] Invalid transfer prevention\n";

    // 9. TCP message ingestion
    {
        RedistributionEngine engine;
        FacilityMessage msg1{"Facility-North", "Insulin", "INS01", 120, "SURPLUS"};
        FacilityMessage msg2{"Facility-South", "Insulin", "INS01", 15, "SHORTAGE"};

        engine.ingestFacilityMessage(msg1);
        engine.ingestFacilityMessage(msg2);

        auto recs = engine.generateRecommendations();
        assert(recs.size() == 1);
        assert(recs[0].sourceFacility == "Facility-North");
        assert(recs[0].destinationFacility == "Facility-South");
        assert(recs[0].suggestedTransfer > 0);
    }

    std::cout << "\nAll redistribution tests passed.\n";
    std::cout << "========================================\n";
    return 0;
}
