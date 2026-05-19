# JetUESub-JSTG-TF03

Jet underlying-event (UE) subtraction studies for the sPHENIX JSTG TF03 task force.
This repository contains a local build of the `jetbackground` library and a macro suite
for running background-subtracted heavy-ion jet reconstruction on simulated data.

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Building `jetbackground`](#building-jetbackground)
3. [Running the Simulation Macro](#running-the-simulation-macro)
4. [Test Configurations](#test-configurations)
5. [Key `HIJetReco.C` Options](#key-hijetrecoc-options)

---

## Prerequisites

This package must be built inside the sPHENIX software environment.
All steps below assume you are on a machine that has the sPHENIX `opt` area mounted

---

## Building `jetbackground`

Run the provided setup script from the repository root **once** to configure the
environment and create the install directory, then build with autotools.

```bash
# 1. Source the environment and set up a local install prefix
source setup.sh

# 2. Enter the jetbackground source directory
cd jetbackground


# 3. Configure, build, and install
./configure --prefix=$INSTALLDIR
make -j4
make install
cd ..
```

After a successful install, the shared libraries `libjetbackground_io.so` and
`libjetbackground.so` will be located under `$INSTALLDIR/lib/` and the public
headers will be under `$INSTALLDIR/include/jetbackground/`.

> **Subsequent builds** – if you open a new shell you only need to re-source the
> environment before running `make`:
> ```bash
> source setup.sh
> cd jetbackground && make -j$(nproc) install && cd ..
> ```

---

## Running the Simulation Macro

### Input files

`Fun4All_Sim.C` expects DST files produced by the sPHENIX MDC2 simulation campaign.
For the default embedded-jet configuration (`jet_flag = 10`) the following four DST
types are required for each `(run_number, segment)` pair:

| DST type | Example filename |
|---|---|
| `CALO_CLUSTER` | `DST_CALO_CLUSTER_pythia8_Jet10_sHijing_0_20fm-0000000031-000000.root` |
| `MBD_EPD` | `DST_MBD_EPD_pythia8_Jet10_sHijing_0_20fm-0000000031-000000.root` |
| `GLOBAL` | `DST_GLOBAL_pythia8_Jet10_sHijing_0_20fm-0000000031-000000.root` |
| `TRUTH_JET` | `DST_TRUTH_JET_pythia8_Jet10_sHijing_0_20fm-0000000031-000000.root` |

Place these files in the directory from which you will run ROOT, or adjust the
`infile` path inside `Fun4All_Sim.C`.

### Running

```bash
# Source the environment (if not already done)
source setup.sh

cd macros/sim

# Basic run: 10 events, run 31, segment 0
root -b -q 'Fun4All_Sim.C(10, 31, 0, "testout.root")'
```

Function signature:

```cpp
void Fun4All_Sim(
    const int nEvents   = 10,    // number of events to process
    const int run_number = 31,   // MDC2 run number
    const int segment    = 0,    // file segment index
    const std::string & outfile  = "testout.root"
)
```

---

## Test Configurations

The variable `HIJET_TEST_NUMBER` inside `Fun4All_Sim.C` selects among several
pre-defined background-estimation configurations for closure testing.
Edit the line

```cpp
int HIJET_TEST_NUMBER = 0;
```

and re-run to switch configurations.

| Number | Label | Description |
|--------|-------|-------------|
| `0` | **Default** | AntiKt seed R=0.2, full-η strip exclusion enabled, no η renormalization, no positive-UE enforcement |
| `1` | **No full-strip exclusion** | Disables `exclude_full_eta_flow_strips`; enables `renormalize_eta_strip` to compensate |
| `2` | **kT seeds, R=0.4** | Switches seed algorithm to kT with R=0.4; uses tower-only exclusion (`DR = -1`) instead of full-DR exclusion |
| `3` | **Limit to 2 seeds** | Restricts the maximum number of omitted seed jets to 2 |
| `4` | **Positive-energy cuts** | Enforces `min_tower_energy = 0.0` and `do_double_check_on_UE = true` to ensure non-negative UE estimates |
| `5` | **Combined** | Applies all modifications from tests 2, 3, and 4 together ("dab on them") |

---

## Key `HIJetReco.C` Options

The `HIJETS` namespace in `macros/sim/HIJetReco.C` exposes all tunable parameters.
The most commonly adjusted settings are summarised below.

| Parameter | Default | Description |
|-----------|---------|-------------|
| `do_flow` | `3` | Flow-subtraction mode: `0` = none, `1` = ψ₂ from calorimeter, `2` = ψ₂ from HIJING truth, `3` = ψ₂ from sEPD (recommended for MDC2 sim) |
| `seed_algo` | `Jet::ANTIKT` | FastJet algorithm used for seed-jet finding |
| `seed_R` | `0.2` | Resolution parameter for seed jets |
| `n_omit_seeds` | `-1` (all) | Maximum number of seed jets omitted from background estimation; `-1` omits all |
| `DR` | `0.4` | Exclusion radius around seed jets; negative value limits exclusion to the jet cone only |
| `exclude_full_eta_flow_strips` | `true` | If `true`, exclude the entire φ ring in η strips that contain a seed jet |
| `renormalize_eta_strip` | `false` | Divide tower energies by the strip average to correct for η-dependent background |
| `min_tower_energy` | `-999` | Tower energy threshold for background estimation; disabled when negative |
| `do_double_check_on_UE` | `false` | Ensures the estimated UE is non-negative after flow modulation |
| `do_reweight` | `true` | Apply φ-reweighting to account for flow modulation in background estimation |

### Tower jet reconstruction path

`MakeHITowerJets()` (default) uses the legacy `DetermineTowerBackground` module.
`MakeHITowerJetsv2()` uses the newer `DetermineTowerBackgroundv1` module which
exposes finer flow-mode control and all of the `HIJETS` namespace parameters listed
above. To enable the v2 path, uncomment the relevant line in `HIJetReco()` at the
bottom of `HIJetReco.C` and comment out the v1 call:

```cpp
// if (Enable::HIJETS_TOWER && !Enable::HIJETS_ENABLE_TEST ) MakeHITowerJets();
if (Enable::HIJETS_TOWER && Enable::HIJETS_ENABLE_TEST ) MakeHITowerJetsv2();
```

and set `Enable::HIJETS_ENABLE_TEST = true` in `Fun4All_Sim.C`.
