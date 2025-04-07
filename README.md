# CREAMPIE — Code RElocation And Memory PIE extractor
CREAMPIE is a WASM library for extracting runtime memory images and relocated entry points from ELF files compiled as PIE (Position Independent Executables). It allows you to convert an ELF into a binary memory blob and an actual entry address, suitable for direct execution on hardware.

ARM Little Endian only.

# USAGE
```ts
import { loadeELF } from "@sie-js/creampie";
import fs from "node:fs";

console.log(loadELF(fs.readFileSync("app.elf")));
```
