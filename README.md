# MidiGlue

Small app to convert Midi inputs into OSC and CSV 

### Install dependencies

```bash
sudo apt install cmake liblo-dev 
```

### Compile and Install

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

### Use

```bash
midiglue config.yaml 
```