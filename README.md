# SineKit

[![status – pre‑alpha](https://img.shields.io/badge/status-pre--alpha-yellow)](https://shields.io)
[![language – C++20](https://img.shields.io/badge/language-C%2B%2B20-blue)](https://isocpp.org)
[![license – PolyForm NC 1.0.0](https://img.shields.io/badge/license-PolyForm%20NC%201.0.0-lightgrey)](#-license)

> **SineKit** is an **open‑source, cross‑platform, cross‑format** audio‑conversion **library** written in **C++20**.  
> It targets *bit‑perfect* conversion between PCM and DSD and will later power both a GUI converter and a licensable SDK.

---

## 📂 Repository Layout

/src          → [library core](https://github.com/H3ct0r55/SineKit/tree/main/src)  
/useful_docs  → [format specs & style guides](https://github.com/H3ct0r55/SineKit/tree/main/useful_docs)  

---

## 🕹️ Using the Library

```cpp
#include "SineKit.h"

int main() {
    sk::SineKit someFunnyAudioFile;

    someFunnyAudioFile.read("example.wav");
    
    someFunnyAudioFile.write("output.wav");
}
```

> ⚠ **API Extremely Unstable** – headers will stabilise before `v0.1.0`.

---

## 🛣️ Roadmap

| Milestone                        | Target window       | Status |
|----------------------------------|---------------------|--------|
| Skeleton library & basic WAV I/O | Early–Mid June 2025 | ✅      |
| All uncompressed PCM & DSD I/O   | Mid–Late June 2025  | 🚧     |
| Bit‑depth conversion suite       | Early–Mid July 2025 | ⏳      |
| Sample‑rate conversion engine    | Mid–Late July 2025  | ⏳      |
| More To Come                     | To Be Planned       | 📅     |

---

## 🧪 Testing & CI

A GoogleTest suite and GitHub Actions pipeline will be introduced once the I/O layer is feature‑complete.

---

## 🤝 Contributing

External contributions are **on hold** during the pre‑alpha phase while the core API crystallises.  
Bug reports and feature requests are welcome via GitHub Issues; pull requests will be accepted once a `CONTRIBUTING.md` is published.

---

## 📄 License

**SineKit is released under the [PolyForm Noncommercial 1.0.0](https://polyformproject.org/licenses/noncommercial/1.0.0/) license.**  
Commercial use requires a separate agreement with **Sine Industry**.

---

## 🙋 Authors & Credits

**Sine Industry** — original author & copyright holder  

### Core developers
- **Hector van der Aa** — CEO of Sine Industry, lead developer · [@H3ct0r55](https://github.com/H3ct0r55)

---

> _SineKit is in rapid development; expect sharp edges and breaking changes!_  
> Thank you for trying it out.