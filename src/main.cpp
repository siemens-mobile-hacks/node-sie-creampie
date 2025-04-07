#include <vector>
#include <cstdint>
#include <format>
#include <cstring>
#include <emscripten/bind.h>

#include "elf.h"

struct ELF {
	uint32_t base;
	uint32_t v_addr;
	uint32_t ram_size;
	uint32_t entry;
	std::vector<uint8_t> memory;
};

static const uint8_t ELF_MAGIC_HEADER[] = {
	0x7f, 0x45, 0x4c, 0x46, /* 0x7f, 'E', 'L', 'F' */
	0x01, /* Only 32-bit objects. */
	0x01, /* Only LSB data. */
	0x01, /* Only ELF version 1. */
};

template<typename T>
const T *getSafePtr(const std::vector<uint8_t> &buffer, size_t offset) {
	if (offset + sizeof(T) > buffer.size())
		throw std::runtime_error(std::format("Unexpected ELF file EOF at {}", offset + sizeof(T)));
	return reinterpret_cast<const T *>(&buffer[offset]);
}

template<typename T>
T *getSafePtr(std::vector<uint8_t> &buffer, size_t offset) {
	if (offset + sizeof(T) > buffer.size())
		throw std::runtime_error(std::format("Unexpected ELF file EOF at {}", offset + sizeof(T)));
	return reinterpret_cast<T *>(&buffer[offset]);
}

static ELF elfloader(uint32_t base, const std::vector<uint8_t> &elf);

static ELF loadELF(uint32_t base, uintptr_t ptr, int len) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
	std::vector<uint8_t> file(data, data + len);
	return elfloader(base, file);
}

EMSCRIPTEN_BINDINGS(CreamPie) {
	emscripten::register_vector<uint8_t>("VectorUint8");

	emscripten::value_object<ELF>("ELF")
		.field("base", &ELF::base)
		.field("virtualAddr", &ELF::v_addr)
		.field("ramSize", &ELF::ram_size)
		.field("entry", &ELF::entry)
		.field("memory", &ELF::memory);

	function("loadELF", &loadELF,
		emscripten::allow_raw_pointers(),
		emscripten::allow_raw_pointer<emscripten::arg<0>>()
	);
}

static void calcSizeAndVAddr(const std::vector<uint8_t> &file, ELF &elf) {
	uint32_t max_addr = 0;
	uint32_t max_ram_addr = 0;
	uint32_t v_addr = 0xFFFFFFFF;

	auto *ehdr = getSafePtr<Elf32_Ehdr>(file, 0);
	for (size_t i = 0; i < ehdr->e_phnum; i++) {
		auto *phdr = getSafePtr<Elf32_Phdr>(file, ehdr->e_phoff + sizeof(Elf32_Phdr) * i);
		if (phdr->p_type == PT_LOAD) {
			v_addr = std::min(v_addr, phdr->p_vaddr);
			if (phdr->p_filesz != 0)
				max_addr = std::max(max_addr, phdr->p_vaddr + phdr->p_filesz);
			max_ram_addr = std::max(max_ram_addr, phdr->p_vaddr + phdr->p_memsz);
		}
	}

	elf.ram_size = max_ram_addr;
	elf.memory.resize(max_addr - v_addr);
	elf.v_addr = v_addr;
}

static int doReloaction(ELF &elf, Elf32_Dyn *dynamic) {
	size_t i = 0;
	std::vector<uint32_t> dyn;
	dyn.resize(0xFF);
	while (dynamic[i].d_tag != DT_NULL) {
		if (dynamic[i].d_tag <= DT_FLAGS) {
			switch (dynamic[i].d_tag) {
				case DT_SYMBOLIC:
					dyn[dynamic[i].d_tag] = 1;
					break;
				case DT_DEBUG:
					dynamic[i].d_un.d_val = 0;
					break;
				case DT_NEEDED:
					// skip
					break;
				default:
					dyn[dynamic[i].d_tag] = dynamic[i].d_un.d_val;
					break;
			}
		}
		++i;
	}

	for (size_t i = 0; i < dyn[DT_RELSZ]; i += sizeof(Elf32_Rel)) {
		auto *rel = getSafePtr<Elf32_Rel>(elf.memory, dyn[DT_REL] - elf.v_addr + i);
		Elf32_Word r_type = ELF32_R_TYPE(rel->r_info);
		uint32_t *addr = getSafePtr<uint32_t>(elf.memory, rel->r_offset - elf.v_addr);

		switch (r_type) {
			case R_ARM_NONE:
				break;

			case R_ARM_RABS32:
				*addr += elf.base - elf.v_addr;
				break;

			case R_ARM_RELATIVE:
				*addr += elf.base - elf.v_addr;
				break;

			case R_ARM_THM_RPC22:
				break;

			default:
				throw std::runtime_error("Unknown relocation type!");
		}
	}

	if (dyn[DT_PLTRELSZ])
		throw std::runtime_error("DT_PLTRELSZ not supported!");

	return 0;
}

static ELF elfloader(uint32_t base, const std::vector<uint8_t> &file) {
	auto *ehdr = getSafePtr<Elf32_Ehdr>(file, 0);

	bool isValidElf = (
		ehdr->e_ehsize == sizeof(Elf32_Ehdr) &&
		ehdr->e_phentsize == sizeof(Elf32_Phdr) &&
		ehdr->e_shentsize == sizeof(Elf32_Shdr)
	);

	if (!isValidElf)
		throw std::runtime_error("Invalid ELF.");

	if (memcmp(ehdr->e_ident, ELF_MAGIC_HEADER, sizeof(ELF_MAGIC_HEADER)) != 0)
		throw std::runtime_error("Invalid ELF magic.");

	if (ehdr->e_machine != EM_ARM)
		throw std::runtime_error("Only ARM ELF's is supported.");

	ELF elf = {
		.base = base,
		.v_addr = 0,
		.memory = {},
	};

	calcSizeAndVAddr(file, elf);

	elf.entry = base + ehdr->e_entry - elf.v_addr;

	for (size_t i = 0; i < ehdr->e_phnum; i++) {
		auto *phdr = getSafePtr<Elf32_Phdr>(file, ehdr->e_phoff + ehdr->e_phentsize * i);
		if (phdr->p_filesz == 0) // skip empty sections
			continue;

		switch (phdr->p_type) {
			case PT_LOAD:
				std::copy(file.begin() + phdr->p_offset, file.begin() + phdr->p_offset + phdr->p_filesz, elf.memory.begin() + phdr->p_vaddr - elf.v_addr);
				break;

			case PT_DYNAMIC: {
				auto *dynamic = getSafePtr<Elf32_Dyn>(elf.memory, phdr->p_vaddr - elf.v_addr);
				doReloaction(elf, dynamic);
				break;
			}
		}
	}

	return elf;
}

/*
int main() {
	auto file = loadFile("/home/azq2/dev/sie/pmb887x-emu/bsp/examples/apoxi_open_boot/build/app.elf");

	auto elf = elfloader(0xB0FC0000, file);
	FILE *fp = fopen("/home/azq2/dev/sie/js/node-sie-serial/examples/data/apoxi-unlock.bin", "wb");
	fwrite(elf.memory.data(), elf.memory.size(), 1, fp);
	fclose(fp);

	return 0;
}

std::vector<uint8_t> loadFile(const std::filesystem::path &path) {
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file) {
		throw std::ios_base::failure("Failed to open file: " + path.string());
	}

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> buffer(size);
	if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
		throw std::ios_base::failure("Failed to read file: " + path.string());
	}

	return buffer;
}
*/
