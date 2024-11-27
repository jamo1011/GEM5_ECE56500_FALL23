#include "cpu/minor/lvpu.hh"

#include <iomanip>
#include <sstream>

#include "base/compiler.hh"
#include "base/logging.hh"
#include "base/trace.hh"
#include "cpu/minor/exec_context.hh"
#include "cpu/minor/cpu.hh"
#include "cpu/minor/execute.hh"
#include "cpu/minor/pipeline.hh"
#include "cpu/utils.hh"
#include "debug/TEST.hh"
#include "debug/LVPU.hh"

namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(Minor, minor);
namespace minor
{

LVPU::LVPU(std::string name_,
    MinorCPU &cpu_,
    int num_lvpt_entries_,
    int num_lct_entries_,
    int num_cvt_entries_,
    int bits_per_entry_,
    std::string hacks_) :
    Named(name_),
    num_lvpt_entries(num_lvpt_entries_),
    num_lct_entries(num_lct_entries_),
    num_cvt_entries(num_cvt_entries_),
    bits_per_entry(bits_per_entry_),
    hacks(hacks_),
    stats(&cpu_)
{
    DPRINTF(LVPU, "LVPT created. num_lvpt_entries: %s, num_lct_entries: %s, num_cvt_entries: %s, lct_bits_per_entry: %s\n",
        num_lvpt_entries,
        num_lct_entries,
        num_cvt_entries,
        bits_per_entry);
}

int LVPU::find_entry(Addr pc)
{
    for (int i = 0; i < lvpt_table.size(); i++) {
        if (pc == lvpt_table[i].pc) {
            DPRINTF(LVPU,
                "find_entry: Found entry PC: %s, valid: %s, value: %s\n",
                pc,
                lvpt_table[i].valid,
                lvpt_table[i].value);
            return i;
        }
    }
    DPRINTF(LVPU,
        "find_entry: No entry found.  PC: %s\n",
        pc);
    return -1;
}

void LVPU::add_entry(Addr pc) {
    //TODO: Limit number of entries to num_entries
    //TODO: Figure out when to remove entries.  Based on classification?
    if (find_entry(pc) == -1) {
        struct lvpt_entry e = {pc, false, 0};
        lvpt_table.push_back(e);
        DPRINTF(LVPU,
            "add_entry: Adding entry to LVPT. PC: %s, Number of entries: %s\n",
            pc,
            lvpt_table.size());
        add_lct_entry(pc);
    } else {
        DPRINTF(LVPU,
            "add_entry: Entry already found in LVPT. PC: %s, Number of entries: %s\n",
            pc,
            lvpt_table.size());
    }
    return;
}

void LVPU::update_entry(Addr pc, RegVal value) {
    int entry_index = find_entry(pc);
    if (entry_index != -1) {
        lvpt_table[entry_index].value = value;
        lvpt_table[entry_index].valid = true;
        DPRINTF(LVPU,
            "update_entry: Updated entry. PC: %s, Value: %s\n",
            pc,
            value);
    } else {
        DPRINTF(LVPU,
            "update_entry: Tried to update an entry that doesn't exist. PC: %s\n",
            pc);
    }
}

bool LVPU::valid_entry(Addr pc) {
    bool valid = false;
    int entry_index = find_entry(pc);
    if (entry_index != -1) {
        valid = lvpt_table[entry_index].valid;
        DPRINTF(LVPU,
            "valid_entry: entry found. pc: %s, valid: %s\n",
            pc,
            valid);
    } else {
        DPRINTF(LVPU,
            "valid_entry: entry not found. pc: %s\n",
            pc);
    }
    return valid;
}

RegVal LVPU::read_entry(Addr pc) {
    int entry_index = find_entry(pc);
    RegVal value;
    if (entry_index != -1 and valid_entry(pc)) {
        value = lvpt_table[entry_index].value;
        DPRINTF(LVPU,
            "read_entry: pc: %s, value: %s\n",
            pc,
            value);
    }
    return value;
}

LVPU::LVPUStats::LVPUStats(MinorCPU *cpu)
    : statistics::Group(cpu, "lvpu"),
    ADD_STAT(num_predictions, statistics::units::Count::get(),
             "Number of LVPU predictions"),
    ADD_STAT(correct_predictions, statistics::units::Count::get(),
             "Number of correct LVPU predictions"),
    ADD_STAT(incorrect_predictions, statistics::units::Count::get(),
             "Number of incorrect LVPU predictions"),
    ADD_STAT(memory_bypasses, statistics::units::Count::get(),
             "Number of memory bypasses from constant loads")
{
    num_predictions
        .flags(statistics::total);
    correct_predictions
        .flags(statistics::total);
    incorrect_predictions
        .flags(statistics::total);
    memory_bypasses
        .flags(statistics::total);
}

LVPU::PredictionResults LVPU::prediction_results(Addr pc, RegVal value) {
    bool misprediction = false;
    bool entry_upgraded = false;
    if (valid_entry(pc)) {
        // Compare predicted value with value returned from memory
        RegVal predicted_value = read_entry(pc);
        if (predicted_value == value) {
            DPRINTF(LVPU, "prediction_results: Correct prediction\n");
            // Get previous classification to determine if entry was upgraded
            Classification previous_classification = get_classification(pc);
            increment_counter(pc);
            Classification new_classification = get_classification(pc);
            if (previous_classification != Constant && new_classification == Constant) {
                DPRINTF(LVPU, "prediction_results: Entry upgraded to constant. pc: %s\n", pc);
                entry_upgraded = true;
            }
        } else {
            DPRINTF(LVPU, "prediction_results: Incorrect prediction\n");
            decrement_counter(pc);
            misprediction = true;
        }
    } else {
        DPRINTF(LVPU, "check_prediction: Valid LVPT entry not found. \n");
    }
    update_entry(pc, value);
    PredictionResults results = {misprediction, entry_upgraded};
    return results;
}

void LVPU::decrement_counter(Addr pc) {
    int min_value = 0; // Minimum value for the counter
    int index = find_lct_entry(pc);
    if (index != -1) {
        if (lc_table[index].classification > min_value) {
            lc_table[index].classification--;
            DPRINTF(LVPU, "d_counter: decremented classification to %d\n",
                lc_table[index].classification);
        } else {
            DPRINTF(LVPU, "d_counter: classification already at minimum (%d)\n", min_value);
        }
    }
    return;
}

void LVPU::increment_counter(Addr pc) {
    int max_value = (1 << bits_per_entry) - 1; // maximum value for the counter
    int index = find_lct_entry(pc);
    if (index != -1) {
        if (lc_table[index].classification < max_value) {
            lc_table[index].classification++;
            DPRINTF(LVPU, "incre_counter: Incremented classification to %d\n",
                lc_table[index].classification);
        } else {
            DPRINTF(LVPU, "incre_counter: classification already at maximum  (%d)\n", max_value);
        }
    }
    return;
}

int LVPU::find_lct_entry(long unsigned int pc) {
    for (size_t i = 0; i < lc_table.size(); i++) {
        if (lc_table[i].pc == pc) {
            return i; // Entry found, return index
        }
    }
    return -1; // Entry not found
}

void LVPU::add_lct_entry(Addr pc) {
    //TODO
    // Default classification to 0

    // check if  PC already exists in the LCT
    if (find_lct_entry(pc) == -1) {
        if (lc_table.size() >= num_lct_entries) {       //   LCT does not exceed  maximum number of entries
            // Replace an existing entry based on classification
            // remove the first entry classified as 'Unpredictable'

            auto it = std::find_if(lc_table.begin(), lc_table.end(), [this](const lct_entry &entry) {
                return this->get_classification(entry.pc) == Unpredictable;
            });

            if (it != lc_table.end()) {
                DPRINTF(LVPU, "add_lct_entry: Replacing entry with PC: %lx\n", it->pc);
                lc_table.erase(it);
            } else {
                DPRINTF(LVPU, "add_lct_entry: Unable to replace any entry in LCT.\n");
                return;
            }
        }

        //  a new LCT entry and add it to the table
        struct lct_entry new_entry = {pc, 0}; // Default classification is Unpredictable
        lc_table.push_back(new_entry);

        DPRINTF(LVPU,
            "add_lct_entry: Added entry to LCT. PC: %s, Total entries: %s\n",
            pc, lc_table.size());
    } else {
        DPRINTF(LVPU,
            "add_lct_entry: Entry already exists in LCT. PC: %s, Total entries: %s\n",
            pc, lc_table.size());
    }
}

bool LVPU::is_predictable(Addr pc) {
    //TODO
    // Predictable when lvpt has a valid entry and lct classified as predictable
    // for 1 bit counter Predictable when classification == 1, unpredictable if classification == 0
    // for 2 bit counter predictable when classification == 2 or 3, unpredictable if classification == 0 or 1

    // Disables load value prediction.  Registers should not be written to and lvpu should not issue a branch
    if (hacks == "never_predictable" or hacks == "constant_only") {
        return false;
    }
    //TODO
    // Predictable when lvpt has a valid entry and lct classified as predictable
    // for 1 bit counter Predictable when classification == 1, unpredictable if classification == 0
    // for 2 bit counter predictable when classification == 2 or 3, unpredictable if classification == 0 or 1

    // ensure the entry exists and valid in LVPT
    if (!valid_entry(pc)) {
        return false; //
    }

    // Check LCT classification
    int lct_index = find_lct_entry(pc);
    if (lct_index == -1) {
        return false; // not predictable if no LCT entry exist
    }

    Classification classification = get_classification(pc);
    if (classification == Unpredictable) {
        return false;
    }
    return true;
}

bool LVPU::is_constant(Addr pc, Addr mem_addr) {
    // Return true if classified as a constant and has an entry in cvt with matching pc
    if (hacks == "never_predictable") {
        return false;
    }
    
    // Check for valid entry
    if (!valid_entry(pc)) {
        return false;
    }

    int lct_index = find_lct_entry(pc);
    if (lct_index == -1) {
        return false;
    }

    // Chack LCT for constant
    LVPU::Classification classification = get_classification(pc);
    if (!(classification == Constant)) {
        return false;
    } 
    
    if (!verify_constant(pc, mem_addr)) {
        return false;
    }

    return true;
}

LVPU::Classification LVPU::get_classification(Addr pc) {
    int index = find_lct_entry(pc);
    if (index != -1) {
        int classification = lc_table[index].classification;
        switch (bits_per_entry) {
            case 1:
                switch (classification) {
                    case 0:
                        return Unpredictable;
                    case 1:
                        return Constant;
                }
            case 2:
                switch (classification) {
                    case 0:
                        return Unpredictable;
                    case 1:
                        return Unpredictable;
                    case 2:
                        return Predictable;
                    case 3:
                        return Constant;
                }
        }
    }
    return Unpredictable;
}

bool LVPU::verify_constant(Addr pc, Addr mem_addr) {
    if (hacks == "no_constants") {
        return false;
    }
    for (auto entry : cv_table) {
        if (entry.pc==pc && entry.mem_addr==mem_addr) {
            return true;
        }
    }
    return false;
}

void LVPU::add_cvt_entry(Addr pc, Addr mem_addr) {
    //TODO
    // Loads are indexed by both the load address and the memory address
    if (cv_table.size() <= num_cvt_entries) {
        DPRINTF(LVPU, "Adding entry to CVT. pc: %s, mem_addr: %s\n", pc, mem_addr);
        struct cvt_entry entry = {pc, mem_addr};
        cv_table.push_back(entry);
    } else {
        DPRINTF(LVPU, "CVT full.  Not adding entry. pc: %s, mem_addr: %s\n", pc, mem_addr);
    }
    return;
}

std::vector<Addr> LVPU::update_store_addr(Addr mem_addr) {
    std::vector<Addr> entries_removed;
    for (int i = cv_table.size()-1; i >= 0; i--) {
        if (cv_table[i].mem_addr == mem_addr) {
            DPRINTF(LVPU, "update_store_addr.  matching entry found for mem_addr: %s, downgrading entry: %s, num cvt entries: %s\n", mem_addr, cv_table[i].pc, cv_table.size());
            entries_removed.push_back(cv_table[i].pc);
            decrement_counter(cv_table[i].pc);
            cv_table.erase(cv_table.begin() + i);
        }
    }
    return entries_removed;
}

} // namespace minor
} // namespace gem5
