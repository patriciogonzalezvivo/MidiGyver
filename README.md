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

# Aknowladgements 

- Based on [MidiOSC](https://github.com/jstutters/MidiOSC/) by [Jon Stutters](https://github.com/jstutters) and [Christian Ashby](https://github.com/cscashby)
- Using [liblo]((http://liblo.sourceforge.net)) library from Steve Harris and Stephen Sinclair 
- Using [RtMidi](http://www.music.mcgill.ca/~gary/rtmidi) classes from Gary P. Scavone 