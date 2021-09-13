# MidiGyver [![Build Status](https://travis-ci.org/patriciogonzalezvivo/midiGyver.svg?branch=master)](https://travis-ci.org/patriciogonzalezvivo/midiGyver)

Flexible console app that allows you to:

* broadcas, filter, bridge MIDI events
* program any MIDI instrument/controler with some JavaScript
* convert to/from OSC
* save events to a MIDI or CSV files 

All this is posible through YAML configuration files.


### Install dependencies

In linux would be:

```bash
sudo apt install cmake
```

In MacOS would be:

```bash
brew install cmake git
```

### Compile and Install

```bash
git clone https://github.com/patriciogonzalezvivo/MidiGyver.git
cd MidiGyver

mkdir build
cd build
cmake ..
make
sudo make install
```

### Use
Devices are program using a YAML file, which is past as the only argument

```bash
midigyver config.yaml 
```

Try one of the examples of the `examples/` folder.

### Config
Each YAML file can contain the configuration of multiple devices. The configuration of a device is set under the node with it own name (**note**: empty spaces and other symbols are replaced with `_` ).

In that node you set up the `out` protocols ( `csv` or as many `osc` clients you want).

Each key event happens in the following order:
```
[ MIDI Key IN ] -> [ shaping function (JS) ] -> [ map ] -> [ send key values to OUT ]
```

Each event node is compose by:
    * `name`: name of the event. This is use to construct the OSC path or the first column on the CSV output
    * `type`: could be: `button`, `toggle`, `states`, `scalar`, `vector` and `color`.
    * `shape`: shaping function to modify the original key value (between `0` and `127` from the key) to any other number. After the mapping the range still will be between `0 ~ 127`. If the result is a `false` it doesn't map or send the key value.
    * `map`: depend on the type of the event it can map:
            - bottom or toggle booleans to **strings** (`on: <something>` and `off: <something>`) to string
            - states linearly from any **array of strings** (ex; `[low, med, high]` )
            - scalars linearly from any **array of numbers** (ex: `[0, 1, 100, 2, -10]`)  
            - vectors linearly from any **array of vectors** (ex: `[ [0, 0], [0.5, 1.0]]`)
            - colors linearly from any **array of colors** (ex: `[ [1, 0, 0], [0, 0, 1]]`)
    * `value`: here is where the final values are store so next time this YAML is reload it can send all the previous states.
    * `out`: you can specify special out puts that will over write the default one.

```yaml
global:
    track: 0

out:
    -   csv
    -   osc://localhost:8000
in:
    nanoKONTROL2*:
        0:
            name: fader00
            type: scalar
            value: 1
            out:
                -   osc://localhost:8001
-
        16:
            name: knob00
            type: scalar
            map: [-3.1415, 3.1415]
            value: 3.1415

        17:
            name: knob01
            type: color
            map: [[1, 0, 0], [0, 1, 0], [0, 0, 1], [1, 1, 1]]
            value: [1, 1, 1, 1]

        18:
            name: knob02
            type: vector
            map: [[0.0, 0.0], [-0.5, 0.0], [-0.5, -0.5], [0.5, -0.5], [0.5, 0.5], [0.0, 0.5], [0.0, 0.0]]
            value: [0, 0, 0]

        19:
            name: knob03
            type: states
            map: [low, med, high, ultra_high]
            value: ultra_high

        32:
            name: sBtns0
            type: toggle
            value: true
            map:
                on: define,DRAW_SHAPE

        58:
            name: track_back
            type: scalar
            shape: |
                function() {
                    if (global.track == 0)
                        return false;

                    if (value == 127)
                        return global.track--;
                    
                    return false;
                }

        59:
            name: track_fwd
            type: scalar
            shape: |
                function() {
                    if (value == 127)
                        return ++global.track;
                    
                    return false;
                }
```

# Acknowledgements 

- Based on [MidiOSC](https://github.com/jstutters/MidiOSC/) by [Jon Stutters](https://github.com/jstutters) and [Christian Ashby](https://github.com/cscashby)
- Using [liblo]((http://liblo.sourceforge.net)) library from Steve Harris and Stephen Sinclair 
- Using [RtMidi](http://www.music.mcgill.ca/~gary/rtmidi) classes from Gary P. Scavone 
- The JS scripts (using [Duktape](https://duktape.org/)) inside YAML file was mostly inspired by [tangram-es](https://github.com/tangrams/tangram-es) implemented by [Matt Blair](https://github.com/matteblair)
