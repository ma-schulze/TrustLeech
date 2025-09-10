#include "shadow.hpp"

#include "log.hpp"
#include "paging/types.hpp"
#include <assert.hpp>
#include <bit.hpp>
#include <exception/inject.hpp>
#include <exception/mmu.hpp>
#include <exception/types.hpp>
#include <exec_vel2.hpp>
#include <paging/frame_allocator.hpp>
#include <paging/shadow/descriptor.hpp>
#include <paging/shadow/table_walk.hpp>
#include <registers.hpp>
#include <spinlock.hpp>

namespace paging::shadow {
	namespace {
		class table_cache {
			public:
				template<typename Allocate>
				bool compute_if_absent(const stage_2::ctl_regs key, stage_2::ctl_regs& result, Allocate allocate) {

                    spin_lock(&lock);
					for (size_t i = 0; i < next_free; i++) {
						if (entries[i].key == key) {
							result = entries[i].value;
                            spin_unlock(&lock);
							return true;
						}
					}

					if (next_free == CACHE_SIZE) {
                        spin_unlock(&lock);
						return false;
					}

					entries[next_free].key = key;
					entries[next_free].value = allocate(key);
					result = entries[next_free].value;
					next_free++;

                    spin_unlock(&lock);
					return true;
				}

				void clear() {
					// Friendship ended with "The Operating System", now
					// "Closing QEMU" is my best friend
					//
					// - garbage collection
					next_free = 0;
				}

			private:
				struct entry {
					stage_2::ctl_regs key;
					stage_2::ctl_regs value;
				};

				static constexpr size_t CACHE_SIZE = 32;

				// Use simple arena allocator with fixed size for now
				size_t next_free = 0;
				entry entries[CACHE_SIZE];
                spinlock_t lock;
		};

		// TODO(multicore) Either make CPU-local or synchronize. In the first
		// case, the VTTBR_EL2.CnP bit may require special handling (instead of
		// just using the guest's value without modification), since we would
		// use different physical tables in this case.
		table_cache cache;

		shadow::descriptor_shadow* allocate_table(const granule_size granule_size) {
			void* ptr = paging::allocate_frame(granule_size);
			assert(ptr != nullptr);
			// TODO Maybe wrap in struct with bounds checking
			return reinterpret_cast<shadow::descriptor_shadow*>(ptr);
		}

		shadow::descriptor_shadow* physical_table() {
			const auto vttbr = read_vttbr_el2();
			const uintptr_t tablebase = vttbr.base_address();
			return reinterpret_cast<shadow::descriptor_shadow*>(tablebase);
		}

		void replicate_descriptor(const uintptr_t ipa, const shadow::descriptor_shadow desc, int level) {
			assert(2 <= level && level <= 3);

			if (level == 2) {
				const auto index = bit_extract(ipa, 31, 21);
				physical_table()[index] = desc;
			} else {
				const auto s2_index = bit_extract(ipa, 31, 21);
                uint64_t *s3_table;
                if(!physical_table()[s2_index].valid()) {
			        void* new_table = paging::allocate_frame(granule_size::G4KiB);
                    auto s2_descr = (uint64_t)new_table;
                    s2_descr &= ~((uint64_t)(1 << 13) - 1);
                    s2_descr |= 3;
				    physical_table()[s2_index] = {s2_descr};
                    s3_table = (uint64_t *)new_table;
                } else {
                    shadow::descriptor_shadow s2_desc = {physical_table()[s2_index]};
                    s3_table = (uint64_t *)s2_desc.next_level_table_addr(2);
                }
				const auto index = bit_extract(ipa, 20, 12);
			    s3_table[index] = desc.v;
				s3_table[index] = desc.v;
			}
		}

		void handle_translation_fault(const exception_syndrome esr, const exception::fault_info& fault_info) {
			const uint64_t ipa = fault_info.faulting_address();
			const auto xlate_result = s2_translate(ipa);

			if (xlate_result.has_mmu_abort) {
				auto used_esr = esr;
				used_esr.mmu_abort.set_status_code(xlate_result.status_code);
				exception::aarch64::take_sync_exception(used_esr, read_far_el2(), fault_info.hpfar);
			} else {
				replicate_descriptor(ipa, xlate_result.output_descriptor, xlate_result.level);
			}
		}
	}

	stage_2::ctl_regs get_ctl_regs(const struct vttbr_el2 vel2_vttbr, const struct vtcr_el2 vel2_vtcr) {
		// Changing these assumptions requires improving the shadow table walk
		if (vel2_vtcr.t0sz() != 32
				|| vel2_vtcr.granule_size() != granule_size::G4KiB
				|| vel2_vtcr.sl0() != stage_2_starting_level::level_2_4k
				|| vel2_vtcr.ds()
				|| vel2_vtcr.sl2()
				|| vel2_vtcr.physical_address_size() != memory::physical_address_size::PAS_48_BITS
				// This check is required since we'd need to flush the TLB more
				// often if vEL2 used this VMID for a VM
				|| vel2_vttbr.vmid() == VEL2_VMID) {
			todo("Unsupported stage 2 configuration");
		}

		stage_2::ctl_regs result;

		const bool success = cache.compute_if_absent(
			{ vel2_vtcr, vel2_vttbr },
			result,
			[](const stage_2::ctl_regs key) -> stage_2::ctl_regs {
				(void) key;
				// For the supported VTCR configuration, the initial table has
				// a size of 16 KiB. We just overshoot by using 64 KiB for now.
				//
				// TODO Implement 16 KiB tables
				// TODO(transparency) Calculate the actual table size here
				auto* table = allocate_table(granule_size::G64KiB);

				const auto vtcr = key.vtcr_el2;
				// Copy over the VMID
				const auto vttbr = key.vttbr_el2
					.with_base_address(reinterpret_cast<uintptr_t>(table))
					// TODO(multicore) Check whether it would be possible to
					// use CnP
					.with_common_not_private(false);

				return {
					vtcr,
					vttbr,
				};
			}
		);

		if (!success) {
			die("Failed to allocate cache entry for shadow paging control registers");
		}

		return result;
	}

	void handle_mmu_abort(const exception_syndrome esr, const exception::fault_info& fault_info) {
		assert(exception::is_lower_el_mmu_abort(esr.exception_class()));

		// if (!exception::is_translation_fault(esr.mmu_abort.status_code())) {
		// 	todo("Unknown MMU abort status code - either forward or handle");
		// }

		handle_translation_fault(esr, fault_info);
	}

	void clear_cache() {
		cache.clear();
	}
}
