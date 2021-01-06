// Midi stuff
// init midi devices, get access
if (navigator.requestMIDIAccess) {
    navigator.requestMIDIAccess({
        sysex: false
    }).then(m => {
            // get input devices and store in global object
            window.midiCtrl = {};
            window.midiCtrl.devices = m;
            // check if cookie for settings exist and select current item if device exists
            let actMidiInDevID = getCookie('midi-in');
            window.midiCtrl.actMidiInDevID = 0;
            if (actMidiInDevID) {
                for (let i of midiCtrl.devices.inputs.values()) {
                    if (i.id == actMidiInDevID) {
                        window.midiCtrl.actMidiInDevID = actMidiInDevID;
                        window.midiCtrl.actMidiInDev = i;
                        break;
                    }
                }
            }
            // not found?
            if (window.midiCtrl.actMidiInDevID == 0) {
                setCookie('midi-in', '0', 365);
            } else {// otherwise register handler
                bindMidiMethods();
                window.midiCtrl.actMidiInDev.onmidimessage = processMidiEvent;
            }
        }
        , (err) => {
            console.log(err);
        });
} else {
    console.warn("No MIDI support in your browser");
}

// midi methods
function bindMidiMethods() {
    midiCtrl.midiLearn = (ch, el, target) => {
        console.log('Midi learn');
        console.log(ch);
        console.log(el);
        midiCtrl.learn = {ch: ch, el: el, t: target};
        window.midiCtrl.actMidiInDev.onmidimessage = midiCtrl.midiLearnOnMsg;
    };
    midiCtrl.midiLearnOnMsg = (e) => {
        let m = midiCtrl.parseMidiEvent(e);
        if (m) { // valid automatization ?
            console.log(midiCtrl.learn);
            midiCtrl.learn.t.style.color = 'green';
            midiCtrl.midiCtrlMappings.push({filter: m, param: {ch: midiCtrl.learn.ch, el: midiCtrl.learn.el}}); // do not confuse plugin ch with midi filter ch!
            console.log(midiCtrl.midiCtrlMappings);
        } else {
            midiCtrl.learn.t.style.color = 'inherit';
        }
        window.midiCtrl.actMidiInDev.onmidimessage = midiCtrl.midiProcess;
        midiCtrl.learn = undefined;
    };
    midiCtrl.midiProcess = (e) => {
        let m = midiCtrl.parseMidiEvent(e);
        if (!m) return; // no valid automation event!

        // check if anything needs to be controlled and do control
        midiCtrl.midiCtrlMappings.forEach(el => {
            if(m.type == el.filter.type && m.ch == el.filter.ch && m.key == el.filter.key){ // found mapping for this midi event!
                console.log('Found mapping, executing REST call!');
                let val = Math.trunc(m.val/16384 * (el.param.el.max - el.param.el.min) + el.param.el.min); // always 14 bit scaled midi
                console.log('value: ' + val);
                $.getq('myq',
                    'api/v1/setPluginParam/' + el.param.ch,
                    {
                        id: el.param.el.id,
                        current: val
                    }
                );
            }
        });
    };
    midiCtrl.midiCtrlMappings = [];
    midiCtrl.learn = undefined; // this will contain the parameter to be learned if learning mode is active
    midiCtrl.parseMidiEvent = (e) => {
        // return value is undefined if no valid midi event for automation
        // otherwise {type, midiCh, key, value (always 14bit scaled)}
        let midiChannel = e.data[0] & 0x0F;
        switch (e.data[0] & 0xf0) {
            case 0x90:
                if (e.data[2] != 0) {  // if velocity != 0, this is a note-on message
                    console.log('Note on');
                    break;
                }
            // if velocity == 0, fall thru: it's a note-off.  MIDI's weird, y'all.
            case 0x80:
                console.log('Note off');
                break;
            case 0xa0:
                console.log('Poly Key Pressure value: ');
                break;
            case 0xB0:
                console.log('Controller: ' + e.data[1] + ' value: ' + e.data[2]);
                return {type: 'c', ch: midiChannel, key: e.data[1], val: e.data[2]<<7};
            case 0xC0:
                console.log('Program change');
                break;
            case 0xD0:
                console.log('Channel pressure');
                break;
            case 0xE0:
                console.log('Pitch bend value: ' + (e.data[1] + (e.data[2] << 7)));
                return {type: 'p', ch: midiChannel, key: 0, val: (e.data[1] + (e.data[2] << 7))};
            case 0xF0:
                console.log('Sysx');
                break;
            default:
                break;
        }
        return undefined;
    }
}


// process and list midi events
function processMidiEvent(event) {
    // parse event
    let midiChannel = event.data[0] & 0x0F;
    console.log('Midi Channel' + midiChannel);
    switch (event.data[0] & 0xf0) {
        case 0x90:
            if (event.data[2] != 0) {  // if velocity != 0, this is a note-on message
                console.log('Note on');
                break;
            }
        // if velocity == 0, fall thru: it's a note-off.  MIDI's weird, y'all.
        case 0x80:
            console.log('Note off');
            break;
        case 0xa0:
            console.log('Poly Key Pressure value: ');
            break;
        case 0xB0:
            console.log('Controller: ' + event.data[1] + ' value: ' + event.data[2]);
            break;
        case 0xC0:
            console.log('Program change');
            break;
        case 0xD0:
            console.log('Channel pressure');
            break;
        case 0xE0:
            console.log('Pitch bend value: ' + (event.data[1] + (event.data[2] << 7)));
            break;
        case 0xF0:
            console.log('Sysx');
            break;
        default:
            break;
    }
}