#pragma once

namespace plat {
	constexpr bool supports(feat feature) {
		switch (feature) {
			case feat::aa32bf16:
			case feat::aa32hpd:
			case feat::aa32i8mm:
			case feat::aes:
			case feat::bbm_level_2:
			case feat::bf16:
			case feat::bti:
			case feat::crc32:
			case feat::csv2:
			case feat::csv2_1p1:
			case feat::csv2_1p2:
			case feat::csv2_2:
			case feat::csv3:
			case feat::dgh:
			case feat::dit:
			case feat::dpb:
			case feat::debugv8p2:
			case feat::debugv8p4:
			case feat::dotprod:
			case feat::doublefault:
			case feat::e0pd:
			case feat::ecv:
			case feat::epac:
			case feat::ets:
			case feat::evt:
			case feat::fcma:
			case feat::fgt:
			case feat::fhm:
			case feat::fp16:
			case feat::fpac:
			case feat::fpaccombine:
			case feat::frintts:
			case feat::flagm:
			case feat::flagm2:
			case feat::gtg:
			case feat::hafdbs:
			case feat::hbc:
			case feat::hcx:
			case feat::hpds:
			case feat::hpds2:
			case feat::hpmn0:
			case feat::i8mm:
			case feat::idst:
			case feat::iesb:
			case feat::jscvt:
			case feat::lor:
			case feat::lpa:
			case feat::lpa2:
			case feat::lrcpc:
			case feat::lrcpc2:
			case feat::lse:
			case feat::lse2:
			case feat::lva:
			case feat::mops:
			case feat::mte:
			case feat::mte2:
			case feat::mte3:
			case feat::nv:
			case feat::nv2:
			case feat::pacimp:
			case feat::pacqarma3:
			case feat::pacqarma5:
			case feat::pan:
			case feat::pan2:
			case feat::pan3:
			case feat::pauth:
			case feat::pauth2:
			case feat::pmull:
			case feat::pmuv3p1:
			case feat::pmuv3p4:
			case feat::pmuv3p5:
			case feat::ras:
			case feat::rasv1p1:
			case feat::rdm:
			case feat::rme:
			case feat::rng:
			case feat::s2fwb:
			case feat::sb:
			case feat::sel2:
			case feat::sha1:
			case feat::sha256:
			case feat::sha3:
			case feat::sha512:
			case feat::sm3:
			case feat::sm4:
			case feat::specres:
			case feat::ssbs:
			case feat::tidcp1:
			case feat::tlbios:
			case feat::tlbirange:
			case feat::ttcnp:
			case feat::ttl:
			case feat::ttst:
			case feat::uao:
			case feat::vhe:
			case feat::vmid16:
			case feat::xnx:
				return true;
			// QEMU supports these features in principle, but we disable them
			// on the command line
			case feat::sve:
			case feat::sve2:
			case feat::sme:
			case feat::sme_fa64:
			case feat::sme_f64f64:
			case feat::sme_i16i64:
			default:
				return false;
		}

		return false;
	}
}

constexpr uint8_t NUM_CORES = 8;
