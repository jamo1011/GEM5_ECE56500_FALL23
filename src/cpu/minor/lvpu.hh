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

class LVPU : public Named 
{
  public:
    enum Classification
    {
      Unpredictable,
      Predictable,
      Constant
    };

    struct LVPUStats : public statistics::Group
    {
      LVPUStats(MinorCPU *cpu);
      /** Stats */
      statistics::Scalar num_predictions;
      statistics::Scalar correct_predictions;
      statistics::Scalar incorrect_predictions;
      statistics::Scalar memory_bypasses;
    } stats;

    struct PredictionResults {
      bool misprediction;
      bool entry_upgraded;
    };

  protected:
    struct lvpt_entry {
      Addr pc;
      bool valid; // 0 when the entry is added.  Change to 1 when the value is returned from memory
      RegVal value;
    };

    struct lct_entry {
      Addr pc;
      int classification;  // n bit saturating counter used to classify table entries
    };

    struct cvt_entry {
      Addr pc;
      Addr mem_addr;
    };

    /** Load value prediction table */
    std::vector<lvpt_entry> lvpt_table;

    /** Load classification table */
    std::vector<lct_entry> lc_table;

    /** Constant verification table */
    std::vector<cvt_entry> cv_table;

    /** Parameters for LCT */
    int num_lct_entries;
    int bits_per_entry;  // Number of bits for saturating counter used for load classification

    /** Parameters for LVPT */
    int num_lvpt_entries;

    /** Parameters for CVT */
    int num_cvt_entries;

    std::string hacks;

  public:
    LVPU(std::string name_,
      MinorCPU &cpu_,
      int num_lvpt_entries_,
      int num_lct_entries_,
      int num_cvt_entries_,
      int bits_per_entry_,
      std::string hacks_);


    /**  LVPT functions */
    // Looks for entry with matching pc in LVPT table.  Returns index of the entry or -1 if not found
    int find_entry(Addr pc);

    // Adds entry to LVPT table. Valid bit is set to false.  
    void add_entry(Addr pc);

    // Updates LVPT entry value.  Sets valid bit to true.
    void update_entry(Addr pc, RegVal value);

    // Checks if entry exists and is valid
    bool valid_entry(Addr pc);

    // Get the value of the given entry if it exists
    RegVal read_entry(Addr pc);

    // Increase saturating counter by one if entry.value matches value.  Decrease counter otherwise
    // Update entry with new value
    // Records results of prediction in stats
    // Return true on correct prediction
    PredictionResults prediction_results(Addr pc, RegVal value);

    /** LCT functions */
    int find_lct_entry(Addr pc);

    // Decreases saturating counter by one
    void decrement_counter(Addr pc);

    // Increases saturating counter by one
    void increment_counter(Addr pc);

    // Add a new entry if the number of entries is less than num_entries.  Entries start at Unpredictable
    //TODO: Need to come up with logic to replace entries when the table is full.  Replace Unpredictable?
    void add_lct_entry(Addr pc);

    // Returns true if entry is valid and is classified as 'Predictable' or 'Constant'
    bool is_predictable(Addr pc);

    // Returns true if the given pc has a valid LVPT entry, classified as 'Constant' in LCT, and has an entry in CVT
    bool is_constant(Addr pc, Addr mem_addr);

    Classification get_classification(Addr pc);

    /** CVT functions */
    void add_cvt_entry(Addr pc, Addr mem_addr);

    // Returns true if memory address and mem_addr matches an entry in CVT
    bool verify_constant(Addr pc, Addr mem_addr);

    // Checks for entries with address.
    // If any entries are found, remove them and downgrade LCT entry to 'predictable'
    // Returns list of PCs of entries that were removed
    std::vector<Addr> update_store_addr(Addr address);
};

} // namespace minor
} // namespace gem5

#endif /* __LOAD_VALUE_PREDICTOR_UNIT_HH__*/
