#include "table_walk.hpp"

#include "log.hpp"
#include "paging/shadow/shadow.hpp"
#include <assert.hpp>
#include <bit.hpp>
#include <exception/types.hpp>
#include <paging/vpas.hpp>
#include <vcpu_ctx.hpp>
#include <lib/libc/stdio.hpp>

namespace paging::shadow {
	namespace {
		/** Read a 64-bit value from the given address, checking whether vEL2
		 * would be allowed to access it.
		 */
		uint64_t read_as_vel2(const uintptr_t addr) {
			// It is technically possible but unlikely vEL2 puts something
			// there. It is more likely we made a mistake
			assert(addr != 0);

			if (vpas::is_illegal(addr)) {
				die("Illegal address, inject external abort");
			}

			return *reinterpret_cast<const uint64_t*>(addr);
		}

		shadow::descriptor_shadow read_descriptor(const uintptr_t addr) {
			return shadow::descriptor_shadow { .v = read_as_vel2(addr) };
		}

		struct s2_ttw_params {
			uint8_t txsz = vcpu().vtcr_el2().t0sz();
		};

		constexpr bool ipa_is_out_of_range(const uintptr_t ipa, const s2_ttw_params& walkparams) {
			const uint8_t iasize = 64 - walkparams.txsz;

			if (iasize >= 56) {
				return false;
			}

			// Remove bottom [iasize-1 : 0] bits
			const uintptr_t relevant = ipa >> iasize;
			// We need to strip of the top [63 : 56] bits as well. Since we
			// shifted ipa down by iasize, we instead need to strip off
			// [63-iasize : 56-iasize]
			const uintptr_t mask = (1 << (56 - iasize)) - 1;
			return (relevant & mask) != 0;
		}

		enum class walk_descriptor_type {
			invalid,
			leaf,
			table,
		};

		constexpr int FINAL_LEVEL = 3;

		constexpr int s2_start_level(const s2_ttw_params& walkparams) {
			(void) walkparams;
			// Chosen by fair dice roll
			return 2;
		}

		constexpr uint8_t ia_size(const uint8_t txsz) {
			return 64 - txsz;
		}

		constexpr bool block_desc_supported(const granule_size granule_size, const int level) {
			switch (granule_size) {
				case granule_size::G4KiB:
					return level == 1 || level == 2;
				case granule_size::G16KiB:
					return level == 2;
				case granule_size::G64KiB:
					return level == 2;
			}

			return false;
		}

		uintptr_t s2_tt_base_address(
			const s2_ttw_params& walkparams,
			const vttbr_el2 ttbr
		) {
			const uint8_t iasize = ia_size(walkparams.txsz);
			const uint8_t granulebits = granule_bits(granule_size::G4KiB);
			const uint8_t descsize_log2 = 3;
			const uint8_t stride = granulebits - descsize_log2;
			const int startlevel = s2_start_level(walkparams);
			const int levels = FINAL_LEVEL - startlevel;

			// Base address is aligned to size of the initial translation table in bytes
			const size_t tsize = (iasize - (levels * stride + granulebits)) + descsize_log2;

			const uintptr_t tablebase = ttbr.base_address();
			return align(tablebase, 1 << tsize);
		}

		uintptr_t s2_sl_tt_entry_address(
			const s2_ttw_params& walkparams,
			const uintptr_t ipa,
			const uintptr_t tablebase
		) {
			const int startlevel = s2_start_level(walkparams);
			const uint8_t iasize = ia_size(walkparams.txsz);
			const uint8_t granulebits = granule_bits(granule_size::G4KiB);
			const uint8_t descsize_log2 = 3;
			const uint8_t stride = granulebits - descsize_log2;
			const int levels = FINAL_LEVEL - startlevel;

			const uint8_t lsb = levels * stride + granulebits;
			const uint8_t msb = iasize - 1;

			const uintptr_t index =  bit_extract(ipa, msb, lsb) << descsize_log2;

			return tablebase | index;
		}

		struct s2_ttw_state {
			s2_ttw_state(const s2_ttw_params& walkparams)
				: base_address(s2_tt_base_address(walkparams, vcpu().vttbr_el2()))
				, level(s2_start_level(walkparams)) {}

			uintptr_t base_address;
			int level;
		};

		constexpr walk_descriptor_type decode_descriptor_type(
			const shadow::descriptor_shadow descriptor,
			const granule_size granule_size,
			const int level
		) {
			if (!descriptor.valid()) {
				return walk_descriptor_type::invalid;
			}

			if (descriptor.descriptor_type() == descriptor_type::block) {
				return block_desc_supported(granule_size, level)
					? walk_descriptor_type::leaf
					: walk_descriptor_type::invalid;
			}

			return level == FINAL_LEVEL
				? walk_descriptor_type::leaf
				: walk_descriptor_type::table;
		}

		constexpr mmu_abort_status_code add_level(const mmu_abort_status_code base, const int level) {
			switch (base) {
				case mmu_abort_status_code::translation_l0:
					switch (level) {
						case -2:
							return mmu_abort_status_code::translation_lm2;
						case -1:
							return mmu_abort_status_code::translation_lm1;
						case 0:
							return mmu_abort_status_code::translation_l0;
						case 1:
							return mmu_abort_status_code::translation_l1;
						case 2:
							return mmu_abort_status_code::translation_l2;
						case 3:
							return mmu_abort_status_code::translation_l3;
						default:
							die("Invalid level");
					};
				default:
					todo("Invalid or implemented base status code");
			}
		}

		xlate_result s2_walk(const uintptr_t ipa, const s2_ttw_params& walkparams) {
			s2_ttw_state walkstate { walkparams };

			if (walkstate.level > FINAL_LEVEL) {
				todo("Inject illegal start level fault");
			}

			uintptr_t desc_addr = s2_sl_tt_entry_address(walkparams, ipa, walkstate.base_address);

			while (true) {
				const auto descriptor = read_descriptor(desc_addr);

				switch (decode_descriptor_type(descriptor, granule_size::G4KiB, walkstate.level)) {
					case walk_descriptor_type::leaf:
						return {
							.output_descriptor = descriptor,
							.level = walkstate.level,
						};
					case walk_descriptor_type::table:
                      {
                        LOG_INFO("Got table desc 0x%llx\n", descriptor);
						// TODO(transparency) Check VTCR_EL2.HAFT bit
                        descriptor_shadow desc_shad{descriptor};
                        uintptr_t s3_table = desc_shad.next_level_table_addr(walkstate.level);
                        uint64_t x = 12;
                        uintptr_t s3_baseaddr = bit_extract(s3_table, 48-1, x);
                        uintptr_t ia = bit_extract(ipa, x+8, 12);
                        LOG_INFO("Got s3 table desc 0x%llx\n", s3_table);
                        LOG_INFO("Got s3 table base addr 0x%llx\n", s3_baseaddr);
                        LOG_INFO("Got s3 table IDX 0x%llx\n", ia);
                        desc_addr = (s3_baseaddr << (9+3)) | (ia << 3);
                        LOG_INFO("Got s3 table desc addr 0x%llx\n", desc_addr);
                      	auto tmp_descriptor = read_descriptor(desc_addr);
                        LOG_INFO("Next blockd esc 0x%llx\n", tmp_descriptor);

                        walkstate.level++;
                        continue;
                      }
					case walk_descriptor_type::invalid:
						return {
							.has_mmu_abort = true,
							.status_code = add_level(mmu_abort_status_code::translation_l0, walkstate.level),
						};
				}
			}
		}
	}

	xlate_result s2_translate(const uintptr_t ipa) {
		s2_ttw_params walkparams;

		// TODO(transparency) The pseudocode checks for an illegal
		// start-level configuration here. We only allow a single valid
		// configuration but for completness sake we should check for it.
		// We could even cache it on initialization.

		if (ipa_is_out_of_range(ipa, walkparams)) {
			todo("IPA out of range");
		}

		// TODO(transparency) Modify and write back output descriptor based
		// on VTCR_EL2.HA
		return s2_walk(ipa, walkparams);
	}
}
