import Module from "#build/creampie.js";

const Creampie = await Module();

export type LoadedELF = {
	image: Buffer;
	base: number;
	virtualAddr: number;
	ramSize: number;
	entry: number;
};

export function loadELF(base: number, file: Buffer): LoadedELF {
	const ptr = Creampie._malloc(file.length);
	Creampie.HEAPU8.set(file, ptr);
	const elf = Creampie.loadELF(base, ptr, file.length);
	Creampie._free(ptr);

	if (!elf)
		throw new Error("Invalid ELF");

	const memory = Buffer.alloc(elf.memory.size());
	for (let i = 0; i < memory.length; i++)
		memory[i] = elf.memory.get(i) ?? 0;
	elf.memory.delete();

	const info: LoadedELF = {
		base: elf.base,
		virtualAddr: elf.virtualAddr,
		ramSize: elf.ramSize,
		entry: elf.entry,
		image: memory
	};

	return info;
}
