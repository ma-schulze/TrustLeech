#include <registers.hpp>
#include <sysops.hpp>
#include <types.hpp>

namespace dbg
{

    constexpr uint64_t MASK_47_TO_12 = 0x0000FFFFFFFFF000ULL;
    constexpr uint64_t MASK_11_TO_0 = 0x0000000000000FFFULL;

    template <typename AT> uint64_t translate(uint64_t address, AT at)
    {
        auto aligned_address = address & ~(static_cast<uint64_t>(4096 - 1));

        at(aligned_address);
        isb();
        uint64_t phys_address = read_par_el1().v;
        if (phys_address & 0b1) {
            return 0;
        }

        return (MASK_47_TO_12 & phys_address) + (MASK_11_TO_0 & address);
    }

    inline uint64_t translate_el0(uint64_t address) {
      return translate(address, at_s12e0r);
    }

    inline uint64_t translate_el1(uint64_t address) {
      return translate(address, at_s12e1r);
    }

    inline uint64_t translate_el1s1(uint64_t address) {
      return translate(address, at_s1e1r);
    }

    inline uint64_t translate_el2s1(uint64_t address) {
      return translate(address, at_s1e2r);
    }


}
