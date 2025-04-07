# CREAMPIE — Code RElocation And Memory PIE extractor
CREAMPIE is a WASM library for extracting runtime memory images and relocated entry points from ELF files compiled as PIE (Position Independent Executables). It allows you to convert an ELF into a binary memory blob and an actual entry address, suitable for direct execution on hardware.

ARM Little Endian only.

# USAGE
```ts
import { loadELF } from "@sie-js/creampie";
import fs from "node:fs";

const elf = loadELF(0xA0000000, fs.readFileSync("app.elf"));
console.log("Base:", "0x" + elf.base.toString(16));
console.log("Entry point:", "0x" + elf.entry.toString(16));
console.log("Ram size:", "0x" + elf.ramSize.toString(16));
console.log("Image size:", "0x" + elf.image.length.toString(16));
console.log("Image data:", elf.image);
```
