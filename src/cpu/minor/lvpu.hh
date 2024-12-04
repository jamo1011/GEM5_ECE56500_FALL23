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

  protected:
    struct LVPUStats : public statistics::Group
    {
      LVPUStats(MinorCPU *cpu);
      /** Stats */
      statistics::Scalar num_predictions;
      statistics::Scalar num_correct;
      statistics::Scalar num_incorrect;
    } stats;

    struct lvpt_entry {
      long unsigned int pc;
      bool valid; // 0 when the entry is added.  Change to 1 when the value is returned from memory
      RegVal value;
    };

    struct lct_entry {
      long unsigned int pc;
      int classification;  // n bit saturating counter used to classify table entries
    };

    struct cvt_entry {
      long unsigned int pc;
      Addr address;
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
    int find_entry(long unsigned int pc);

    // Adds entry to LVPT table. Valid bit is set to false.  
    void add_entry(long unsigned int pc);

    // Updates LVPT entry value.  Sets valid bit to true.
    void update_entry(long unsigned int pc, RegVal value);

    // Checks if entry exists and is valid
    bool valid_entry(long unsigned int pc);

    // Get the value of the given entry if it exists
    RegVal read_entry(long unsigned int pc);

    // Increase saturating counter by one if entry.value matches value.  Decrease counter otherwise
    // Update entry with new value
    // Records results of prediction in stats
    // Return true on correct prediction
    bool prediction_results(long unsigned int pc, RegVal value);

    /** LCT functions */
    // Decreases saturating counter by one
    void decrement_counter(long unsigned int pc);

    // Increases saturating counter by one
    void increment_counter(long unsigned int pc);

    // Add a new entry if the number of entries is less than num_entries.  Entries start at Unpredictable
    //TODO: Need to come up with logic to replace entries when the table is full.  Replace Unpredictable?
    void add_lct_entry(long unsigned int pc);

    // Returns true if entry is valid and is classified as 'Predictable' or 'Constant'
    bool is_predictable(long unsigned int pc);

    // Returns true if the given pc has a valid LVPT entry, classified as 'Constant' in LCT, and has an entry in CVT
    bool is_constant(long unsigned int pc);

    /** CVT functions */
    void add_cvt_entry(long unsigned int pc, Addr address);

    // Checks for entries with address. Removes any entries that are found
    // Downgrades LCT entry to 'predictable'
    void update_store_addr(Addr address);
};

} // namespace minor
} // namespace gem5

#endif /* __LOAD_VALUE_PREDICTOR_UNIT_HH__*/
