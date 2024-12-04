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

int LVPU::find_entry(long unsigned int pc)
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

void LVPU::add_entry(long unsigned int pc) {
    //TODO: Limit number of entries to num_entries
    //TODO: Figure out when to remove entries.  Based on classification?
    if (find_entry(pc) == -1) {
        struct lvpt_entry e = {pc, false, 0};
        lvpt_table.push_back(e);
        DPRINTF(LVPU,
            "add_entry: Adding entry to LVPT. PC: %s, Number of entries: %s\n",
            pc,
            lvpt_table.size());
    } else {
        DPRINTF(LVPU,
            "add_entry: Entry already found in LVPT. PC: %s, Number of entries: %s\n",
            pc,
            lvpt_table.size());
    }
    return;
}

void LVPU::update_entry(long unsigned int pc, RegVal value) {
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

bool LVPU::valid_entry(long unsigned int pc) {
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

RegVal LVPU::read_entry(long unsigned int pc) {
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
    ADD_STAT(num_correct, statistics::units::Count::get(),
             "Number of correct LVPU predictions"),
    ADD_STAT(num_incorrect, statistics::units::Count::get(),
             "Number of incorrect LVPU predictions")
{
    num_predictions
        .flags(statistics::total);
    num_correct
        .flags(statistics::total);
    num_incorrect
        .flags(statistics::total);
}

bool LVPU::prediction_results(long unsigned int pc, RegVal value) {
    bool misprediction = false;
    if (valid_entry(pc)) {
        // Compare predicted value with value returned from memory
        RegVal predicted_value = read_entry(pc);
        if (predicted_value == value) {
            DPRINTF(LVPU, "check_prediction: Correct prediction\n");
            increment_counter(pc);
            stats.num_predictions++;
            stats.num_correct++;
        } else {
            DPRINTF(LVPU, "check_prediction: Incorrect prediction\n");
            decrement_counter(pc);
            stats.num_predictions++;
            stats.num_incorrect++;
            misprediction = true;
        }
    } else {
        DPRINTF(LVPU, "check_prediction: Valid LVPT entry not found. \n");
    }
    update_entry(pc, value);
    return misprediction;
}

void LVPU::decrement_counter(long unsigned int pc) {
    //TODO
    return;
}

void LVPU::increment_counter(long unsigned int pc) {
    //TODO
    return;
}

void LVPU::add_lct_entry(long unsigned int pc) {
    //TODO
    // Default classification to 0
    return;
}

bool LVPU::is_predictable(long unsigned int pc) {
    //TODO
    // Predictable when lvpt has a valid entry and lct classified as predictable
    // for 1 bit counter Predictable when classification == 1, unpredictable if classification == 0
    // for 2 bit counter predictable when classification == 2 or 3, unpredictable if classification == 0 or 1

    if (hacks == 'never_predictable') {
        return false
    }
    return valid_entry(pc);
}

bool LVPU::is_constant(long unsigned int pc) {
    //TODO
    // for 1 bit counter, constant if classification == 1
    // for 2 bit counter, constant if classification == 3
    return false;
}

void LVPU::add_cvt_entry(long unsigned int pc, Addr address){
    //TODO
    return;
}

void LVPU::update_store_addr(Addr address) {
    //TODO
    return;
}

} // namespace minor
} // namespace gem5
