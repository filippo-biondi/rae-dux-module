# My ZMK Keyboard Configuration

## Repository Structure
This repository contains both:
- A **ZMK configuration** (to define bindings and enable features).
- A **ZMK external module** (for additional custom features not present in upstream ZMK).

### Structure
```text
.
├── CMakeLists.txt             
├── build.yaml                 # Build config for CI
├── config/                    # Global ZMK configurations
│   ├── features/              # Custom features: behaviors, combos, macros
│   ├── rae_dux.conf           # Base config for Rae Dux
│   ├── rae_dux.keymap         # Base keymap for Rae Dux
│   └── west.yml               # West manifest
├── boards/shields/            # Custom shield definitions
│   ├── rae_dux/               # Standard Rae Dux
│   └── rae_dux_24/            # Rae Dux 24-key variant
├── dts/                       # Custom Device Tree & bindings
├── src/                       # Custom C behaviors
├── zephyr/module.yaml         # Zephyr module configuration
├── flake.nix                  # Nix flake for builds
└── flake.lock                 # Nix flake lockfile
````

## Building

You can find pre-built firmware in the GitHub Actions artifacts.

If you use **Nix** (with flakes and nix-command experimental features enabled), build locally with:

```sh
nix build
```

The resulting firmware will be available in the `result` folder.

To build without Nix, refer to the [ZMK official documentation](https://zmk.dev/docs/development/local-toolchain/setup).

> The **containerized approach** is highly recommended.

## Flashing

To flash the firmware, simply copy the firmware file to the **Nice!Nano** while it's in bootloader mode.
