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
Devices are program using a YAML file, which is past as the only argument

```bash
midiglue config.yaml 
```

### Config
Each YAML file can contain the configuration of multiple devices. The configuration of a device is set under the node with it own name (**note**: empty spaces and other symbols are replaced with `_` ).
In that node you set up the `out` protocols (`osc` and/or 'csv') and the `events`.
Each event is compose by:
    * `name`: name of the event. This is use to construct the OSC path or the first column on the CSV output
    * `type`: could be: `button`, `switch`, `scalar`, `sequence`
    * `map`: depend on the type of the event
    * `value`:

```yaml
nanoKONTROL2_20_0:

    out:
        osc:
            address: localhost
            port: 8000

        csv: on

    events:
        -   name: fader00
            type: scalar
            map: [0, 1]

        -   name: fader01
            type: scalar
            map: [0, 1]

        -   name: fader02
            type: scalar
            map: [0, 1]

        -   name: fader03
            type: scalar
            map: [0, 1]

        -   name: fader04
            type: scalar
            map: [0, 1]

        -   name: fader05
            type: scalar
            map: [0, 1]

        -   name: fader06
            type: scalar
            map: [0, 1]

        -   name: fader07
            type: scalar
            map: [0, 1]

        -   type: none
        -   type: none
        -   type: none
        -   type: none
        -   type: none
        -   type: none
        -   type: none
        -   type: none

        -   name: knob00
            type: scalar
            map: [0, 1]

        -   name: knob01
            type: scalar
            map: [0, 1]

        -   name: knob02
            type: scalar
            map: [0, 1]

        -   name: knob03
            type: scalar
            map: [0, 1]

        -   name: knob04
            type: scalar
            map: [0, 1]

        -   name: knob05
            type: scalar
            map: [0, 1]

        -   name: knob06
            type: scalar
            map: [0, 1]

        -   name: knob07
            type: scalar
            map: [0, 1]
            
        -   type: none
        -   type: none
        -   type: none
        -   type: none
        -   type: none
        -   type: none
        -   type: none
        -   type: none

        -   name: sBtns0
            type: switch
            value: off
            map:
                on: define,TEST
                off: undefine,TEST

        -   name: sBtns1
            type: switch
            value: off

        -   name: sBtns2
            type: switch
            value: off

        -   name: sBtns3
            type: switch
            value: off

        -   name: sBtns4
            type: switch
            value: off

        -   name: sBtns5
            type: switch
            value: off

        -   name: sBtns6
            type: switch
            value: off

        -   name: sBtns7
            type: switch
            value: off

        -   type: none

        -   name: play
            type: button

        -   name: stop
            type: button

        -   name: backward
            type: button

        -   name: forward
            type: button

        -   name: rec
            type: button

        -   name: cycle
            type: button

        -   type: none

        -   name: mBtns0
            type: switch
            value: off

        -   name: mBtns1
            type: switch
            value: off

        -   name: mBtns2
            type: switch
            value: off

        -   name: mBtns3
            type: switch
            value: off

        -   name: mBtns4
            type: switch
            value: off

        -   name: mBtns5
            type: switch
            value: off

        -   name: mBtns6
            type: switch
            value: off

        -   name: mBtns7
            type: switch
            value: off
            
        -   type: none
        -   type: none

        -   name: trackPrev
            type: button

        -   name: trackNext
            type: button

        -   name: markerSet
            map:
                on: debug,on
                off: debug,off

        -   name: markerPrev
            type: button

        -   name: markerNext
            type: button
            
        -   type: none

        -   name: rBtns0
            type: switch
            value: off

        -   name: rBtns1
            type: switch
            value: off

        -   name: rBtns2
            type: switch
            value: off

        -   name: rBtns3
            type: switch
            value: off

        -   name: rBtns4
            type: switch
            value: off

        -   name: rBtns5
            type: switch
            value: off

        -   name: rBtns6
            type: switch
            value: off

        -   name: rBtns7
            type: switch
            value: off

        -   type: none
        -   type: none
        -   type: none
        -   type: none
        -   type: none
        -   type: none
        -   type: none
        -   type: none
```

# Aknowladgements 

- Based on [MidiOSC](https://github.com/jstutters/MidiOSC/) by [Jon Stutters](https://github.com/jstutters) and [Christian Ashby](https://github.com/cscashby)
- Using [liblo]((http://liblo.sourceforge.net)) library from Steve Harris and Stephen Sinclair 
- Using [RtMidi](http://www.music.mcgill.ca/~gary/rtmidi) classes from Gary P. Scavone 