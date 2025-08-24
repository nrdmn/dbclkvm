# `dbcl` on Loongarch in KVM

This is a reproducer that crashes the CPU core that it is run on. It does so
by calling the `dbcl` instruction inside KVM. For example dmesg output, see
the file dmesg.txt.

## How to build and run

```
meson setup build
meson compile -C build
./build/dbclkvm
```
