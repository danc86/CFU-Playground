# CFU Playground

Want a faster ML processor?   Do it yourself!

This project provides a framework that an engineer, intern, or student can use to design and evaluate **enhancements** to an FPGA-based “soft” processor, specifically to increase the performance of machine learning (ML) tasks.   The goal is to abstract away most infrastructure details so that the user can get up to speed quickly and focus solely on adding new processor instructions, exploiting them in the computation, and measuring the results.

This project enables rapid iteration on processor improvements -- multiple iterations per day.

This is how it works:
* Choose a TensorFlow Lite model; a quantized person detection model is provided, or bring your own.
* Execute the inference on the Arty FPGA board to get cycle counts per layer.
* Choose an TFLite operator to accelerate, and dig into that code.
* Design new instruction(s) that can replace multiple basic operations.
* Build a custom function unit (a small amount of hardware) that performs the new instruction(s).
* Modify the TFLite/Micro library kernel to use the new instruction(s), which are available as intrinsics with function call syntax.
* Rebuild the FPGA Soc, recompile the TFLM library, and rerun to measure improvement.

The focus here is performance, not demos.  The inputs to the ML inference are canned/faked, and the only output is cycle counts.  It would be possible to export the improvements made here to an actual demo, but currently no pathway is set up for doing so.

With the exception of Vivado, everything used by this project is open source.

**Disclaimer: This is not an officially supported Google project.   Support and/or new releases may be limited.**

_This is an early prototype of a ML exploration framework; expect a lack of documentation and occasional breakage. If you want to collaborate on building out this framework, reach out to tcal@google.com!   See "Contribution guidelines" below._


### Required hardware/OS

* Currently, the only supported target is the Arty 35T board from Digilent.
* The only supported host OS is Linux (Debian / Ubuntu).

If you want to try things out using [Renode](https://renode.io) simulation,
then you don't need either the Arty board or Vivado software.
You can also perform Verilog-level cycle-accurate simulation with Verilator,
but this is much slower.

### Assumed software

* [Vivado](https://www.xilinx.com/support/download.html) must be manually installed.

Other required packages will be checked for and, if on a Debian-based system,
automatically installed by the setup script below.


### Setup

Clone this repo, `cd` into it, then get run:
```sh
scripts/setup
```
### Use

Build the SoC and load the bitstream onto Arty:
```sh
cd proj/proj_template
make prog
```

This builds the SoC with the default CFU from `proj/proj_template`.   Later you'll copy this and modify it to make your own project.


Build a RISC-V program and execute it on the SoC that you just loaded onto the Arty:
```sh
make load
```

To use Renode to execute on a simulator on the host machine (no Vivado or Arty board required), execute:

```sh
make renode
```

To use Verilator to execute on a cycle-accurate RTL-level simulator (no Vivado or Arty board required), execute:

```sh
make PLATFORM=sim load
```

### Underlying open-source technology

* [LiteX](https://github.com/enjoy-digital/litex): Open-source framework for assembling the SoC (CPU + peripherals)
* [VexRiscv](https://github.com/SpinalHDL/VexRiscv): Open-source RISC-V soft CPU optimized for FPGAs
* [nMigen](https://github.com/nmigen/nmigen): Python toolbox for building digital hardware


### Licensed under Apache-2.0 license

See the file [LICENSE](LICENSE).

### Contribution guidelines

**If you want to contribute to CFU Playground, be sure to review the
[contribution guidelines](CONTRIBUTING.md).  This project adheres to Google's
[code of conduct](CODE_OF_CONDUCT.md).   By participating, you are expected to
uphold this code.**
