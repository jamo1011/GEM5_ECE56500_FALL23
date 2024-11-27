#ifndef __LOAD_VALUE_PREDICTOR_UNIT_HH__
#define __LOAD_VALUE_PREDICTOR_UNIT_HH__

#include <vector>

#include "base/named.hh"
#include "base/types.hh"
#include "cpu/minor/buffers.hh"
#include "cpu/minor/cpu.hh"
#include "cpu/minor/pipe_data.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/BaseMinorCPU.hh"
#include "cpu/reg_class.hh"

namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(Minor, minor);
namespace minor
{

/** Load Value Prediction Table */
class LVPT : public Named
{
  protected:
    struct entry {
      long unsigned int pc;
      bool valid; // 0 when the entry is added.  Change to 1 when the value is returned from memory
      RegVal value;
    };

    /** Table of entries in load value prediction table */
    std::vector<entry> lvpt_table;

    /** Max number of entries for LVPT */
    int num_entries;

  public:
    LVPT(std::string name_,
      MinorCPU &cpu_,
      int num_entries_);

    // Looks for entry with matching pc in LVPT table.  Returns index of the entry or -1 if not found
    int find_entry(long unsigned int pc);

    // Adds entry to LVPT table. Valid bit is set to false.  
    void add_entry(long unsigned int pc);

    // Updates entry value.  Sets valid bit to true.
    void update_entry(long unsigned int pc, RegVal value);

    // Checks if entry exists and is valid
    bool valid_entry(long unsigned int pc);

    // Get the value of the given entry if it exists
    RegVal read_entry(long unsigned int pc);

    virtual ~LVPT();
};

/** Load Classification Table */
class LCT: public Named
{
  public:
    enum Classification
    {
      Unpredictable,
      Predictable,
      Constant
    };

  protected:
    struct LCTStats : public statistics::Group
    {
        LCTStats(MinorCPU *cpu);
        /** Stats */
        statistics::Scalar num_predictions;
        statistics::Scalar num_correct;
        statistics::Scalar num_incorrect;
    } stats;

    struct entry {
      long unsigned int pc;
      int classification;  // n bit saturating counter used to classify table entries
    };

    /** Table of entries in load value prediction table */
    std::vector<entry> lc_table;

    /** Max number of entries for LVPT */
    int num_entries;
    int bits_per_entry;

  public:
    LCT(std::string name_,
      MinorCPU &cpu_,
      int num_entries_);

    // Increase saturating counter by one
    void record_prediction(long unsigned int pc);

    // Decrease saturating counter by one
    void record_misprediction(long unsigned int pc);

    // Add a new entry if the number of entries is less than num_entries.  Entries start at Unpredictable
    //TODO: Need to come up with logic to replace entries when the table is full.  Replace Unpredictable?
    void add_entry(long unsigned int pc);

    // Returns the classification of the given entry
    // If bits/entry is 1: 0->Unpredictable, 1->Predictable
    // If bits/entry is 2: 0,1->Unpredictable, 2->Predictable, 3->Constant
    enum Classification get_entry(long unsigned int pc);

};

class CVT: public Named
{
  protected:
    struct entry {
      long unsigned int pc;
      Addr address;
    };

    std::vector<entry> cv_table;

  public:
    CVT(std::string name_,
      MinorCPU &cpu_,
      int num_entries_);

    void add_entry(long unsigned int pc, Addr address);

    // Checks for entries with address. Removes any entries that are found
    // Returns true if any entry with address are found.
    bool check_for_entry_and_remove(Addr address);
};

} // namespace minor
} // namespace gem5

#endif /* __LOAD_VALUE_PREDICTOR_UNIT_HH__*/
