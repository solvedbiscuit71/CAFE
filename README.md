# CAFE: Context-Aware Forwarding Engine for Autonomous Vehicular Communication

⚠️ **Disclaimer:** This is a custom, unsupported fork of the [ndnSIM 2.x simulator](https://github.com/named-data-ndnSIM/ns-3-dev).

## Overview
This repository contains the codebase for **CAFE**, designed to support research in context-aware vehicular networking. We have open-sourced this project under the **GNU General Public License v2.0** to ensure the reproducibility of our published results and to encourage further academic exploration.

## Prerequisites
This project has been tested and verified on the following platforms:
* **Ubuntu 22.04** (amd64)
* **Ubuntu 24.04** (amd64)

## 1. SUMO Setup

### Step 1: Install Dependencies
```bash
sudo apt update
sudo apt-get install cmake python3 g++ libxerces-c-dev libfox-1.6-dev libgdal-dev libproj-dev libgl2ps-dev swig
```

### Step 2: Clone and Build SUMO
```bash
git clone --recursive https://github.com/eclipse/sumo
export SUMO_HOME="$HOME/sumo"
mkdir -p sumo/build_config/cmake-build
cd sumo/build_config/cmake-build
cmake ../..
make -j$(nproc)
```

### Step 3: Configure Environment
Add the following to your `~/.bashrc`:
```bash
export SUMO_HOME="$HOME/sumo"
export PATH=$PATH:"$SUMO_HOME/bin"
```
Apply the changes:
```bash
source ~/.bashrc
```

## 2. CAFE Setup

### Step 1: Install Dependencies
```bash
sudo apt install build-essential libsqlite3-dev libboost-all-dev libssl-dev git python3-setuptools castxml
```

### Step 2: Clone and Build CAFE
```bash
git clone https://github.com/solvedbiscuit71/CAFE.git
cd CAFE
CXXFLAGS="-std=c++17" ./waf configure --disable-python --disable-examples
./waf
```

## 3. NetAnim Setup (Optional)

### Step 1: Install Dependencies
```bash
sudo apt install -y mercurial qtcreator qtbase5-dev qtchooser qt5-qmake cmake qtbase5-dev-tools
```

### Step 2: Build NetAnim
```bash
cd netanim
make clean
qmake NetAnim.pro
make
```

## Reproducing Results

To recreate the results presented in the paper (specifically Table 4), run the provided scenario script:

```bash
python3 scripts/scene.py
```

This will generate a `result.csv` file. 

### Note on Broadcast Flooding
By default, the simulation includes context-aware logic. To replicate the **broadcast flooding** baseline, you must manually disable the geobroadcast logic:
1. Open `src/ndnSIM/NFD/daemon/fw/caf-forwarder.cpp`.
2. Comment out the relevant geobroadcast logic sections.
3. Re-run `./waf` and the simulation script.
