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

LVPT::LVPT(std::string name_,
    MinorCPU &cpu_,
    int num_entries_) :
    Named(name_),
    num_entries(num_entries_)
{
    DPRINTF(LVPU,
        "LVPT created.  num_entries: %s\n",
        num_entries);
}

LVPT::~LVPT()
{
    DPRINTF(LVPU,
        "Deconstructing LVPT. Number of entries: %s",
        lvpt_table.size());
}

int LVPT::find_entry(long unsigned int pc)
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

void LVPT::add_entry(long unsigned int pc) {
    //TODO: Limit number of entries to num_entries
    //TODO: Figure out when to remove entries.  Based on classification?
    if (find_entry(pc) == -1) {
        struct entry e = {pc, false, 0};
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

void LVPT::update_entry(long unsigned int pc, RegVal value) {
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

bool LVPT::valid_entry(long unsigned int pc) {
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

RegVal LVPT::read_entry(long unsigned int pc) {
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

LCT::LCTStats::LCTStats(MinorCPU *cpu)
    : statistics::Group(cpu, "lct"),
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

LCT::LCT(std::string name_,
    MinorCPU &cpu_,
    int num_entries_) :
    Named(name_),
    num_entries(num_entries_),
    stats(&cpu_)
{
    DPRINTF(LVPU,
        "LCT created.  num_entries: %s\n",
        num_entries);
}

    // Increase saturating counter by one
void LCT::record_prediction(long unsigned int pc) {
    DPRINTF(LVPU, "record_prediction\n");
    stats.num_predictions++;
    stats.num_correct++;
}

    // Decrease saturating counter by one
void LCT::record_misprediction(long unsigned int pc) {
    DPRINTF(LVPU, "record_misprediction\n");
    stats.num_predictions++;
    stats.num_incorrect++;
}


} // namespace minor
} // namespace gem5
