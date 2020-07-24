Small app to convert Midi inputs into OSC and CSV 

### Install dependencies

```bash
sudo apt install cmake liblo-dev libyaml-cpp-dev
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
Devices are program using a YAML file, which is past as the only argument

```bash
midigyver config.yaml 
```

### Config
Each YAML file can contain the configuration of multiple devices. The configuration of a device is set under the node with it own name (**note**: empty spaces and other symbols are replaced with `_` ).

In that node you set up the `out` protocols (`osc` and/or 'csv') and the `events`.

Each event is compose by:
    * `name`: name of the event. This is use to construct the OSC path or the first column on the CSV output
    * `type`: could be: `button`, `toggle`, `states`, `scalar`, `vector` and `color`.
    * `map`: depend on the type of the event
    * `update`: update JS function 
    * `value`: 

```yaml
global:
    counter: 0

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
                off: undefine,DRAW_SHAPE

        43:
            name: backward
            type: button
            update: |
                function() {
                    global.counter--;
                    return global.counter;
                }
        44:
            name: forward
            type: button
            update: |
                function() {
                    global.counter++;
                    return global.counter;
                }
```

# Aknowladgements 

- Based on [MidiOSC](https://github.com/jstutters/MidiOSC/) by [Jon Stutters](https://github.com/jstutters) and [Christian Ashby](https://github.com/cscashby)
- Using [liblo]((http://liblo.sourceforge.net)) library from Steve Harris and Stephen Sinclair 
- Using [RtMidi](http://www.music.mcgill.ca/~gary/rtmidi) classes from Gary P. Scavone 