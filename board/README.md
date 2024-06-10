# Reference board design

This directory contains [atopile](https://atopile.io/) & [KiCad](https://www.kicad.org/) project files for the reference board design.

If you just need PCB gerber files, see `elec/layout/default/production/board.zip`.

## Folder & File structure

- `ato.yaml`: atopile project setting file. see [atopile documentation](https://atopile.io/config/).
- `elec/`: main project files related with atopile and KiCad
  - `footprints/`: KiCad footprints & 3D shape datas.
  - `layout/`: KiCad project files.
  - `src/`: atopile module files.

## To modify & build your own gerber files

1. Modify `elec/src/board.ato` as you like.
2. [Install atopile](https://atopile.io/getting-started/) and run `ato install & ato build` in this folder, then you got netlist file at `build/default.net`.
3. Open `elec/layout/board.kicad_pro` in KiCad and open PCB file (or just open `elec/layout/board.kicad_pcb`).
4. From `File` menu, select `Import -> Netlist...`, then select the generated netlist file.
5. Now netlist updated as you write in `elec/src/board.ato`. Change board layout as you like!

Subsequent steps is the same as the usual board design in KiCad. Refer to  KiCad documentation.