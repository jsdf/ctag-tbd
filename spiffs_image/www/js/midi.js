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
                window.midiCtrl.actMidiInDev.onmidimessage = midiCtrl.midiProcess;
            }
            bindMidiMethods();
        }
        , (err) => {
            console.log(err);
        });
} else {
    console.warn("No MIDI support in your browser");
}

// midi methods bound to window.midiCtrl object
function bindMidiMethods() {
    midiCtrl.selectInputDevice = (dev) =>{
        // remove listener
        if(midiCtrl.actMidiInDev){
            midiCtrl.actMidiInDev.onmidimessage = null;
        }
        // deselect in device
        midiCtrl.actMidiInDevID = 0;
        midiCtrl.actMidiInDev = undefined;
        // select new device
        for(let i of midiCtrl.devices.inputs.values()){
            if(i.id == dev){
                midiCtrl.actMidiInDevID = dev;
                midiCtrl.actMidiInDev = i;
                midiCtrl.actMidiInDev.onmidimessage = midiCtrl.midiProcess;
                break;
            }
        }
        // persist selection on cookie
        setCookie('midi-in', midiCtrl.actMidiInDevID.toString(), 365);
    };
    // activates midi learn
    midiCtrl.midiLearn = (ch, el, target) => {
        if(!window.midiCtrl.actMidiInDevID) return; // no active dev
        target.style.color = 'red';
        midiCtrl.learn = {ch: ch, el: el, t: target};
        window.midiCtrl.actMidiInDev.onmidimessage = midiCtrl.midiLearnOnMsg;
    };
    // callback for midi message if in midi learn mode
    midiCtrl.midiLearnOnMsg = (e) => {
        let m = midiCtrl.parseMidiEvent(e);
        if (m) { // valid automatization ?
            midiCtrl.learn.t.style.color = 'green';
            midiCtrl.midiCtrlMappings.push({filter: m, param: {ch: midiCtrl.learn.ch, el: midiCtrl.learn.el}}); // do not confuse plugin ch with midi filter ch!
        } else {
            midiCtrl.learn.t.style.color = 'inherit';
        }
        window.midiCtrl.actMidiInDev.onmidimessage = midiCtrl.midiProcess;
        midiCtrl.learn = undefined;
    };
    // callback for midi message, general
    midiCtrl.midiProcess = (e) => {
        let m = midiCtrl.parseMidiEvent(e);
        if (!m) return; // no valid automation event!
        // check if anything needs to be controlled and do control
        midiCtrl.midiCtrlMappings.forEach(el => {
            if(m.type == el.filter.type && m.ch == el.filter.ch && m.key == el.filter.key){ // found mapping for this midi event!
                let val = Math.trunc(m.val * (el.param.el.max - el.param.el.min) + el.param.el.min); // use int part
                if(val>el.param.el.max) val = el.param.el.max;
                if(val<el.param.el.min) val = el.param.el.min;
                if(val != el.param.el.current){ // if parameter has really changed from current value, TODO check which value slider has!
                    // set current parameter in automatization array
                    el.param.el.current = val;
                    // update ui if edit view is active
                    if(midiCtrl.uiUpdateCallback) midiCtrl.uiUpdateCallback(el.param);
                    // perform REST request to set parameter
                    $.getq('myq',
                        'api/v1/setPluginParam/' + el.param.ch,
                        {
                            id: el.param.el.id,
                            current: val
                        }
                    );
                }
            }
        });
    };
    // remove specified midi mapping
    midiCtrl.removeMapping = (ch, el, target) => {
        target.style.color = 'inherit';
        midiCtrl.learn = undefined;
        midiCtrl.midiCtrlMappings = midiCtrl.midiCtrlMappings.filter( el2 => el2.param.ch != ch || el2.param.el.id != el.id);
    };
    // indicates mapping, returns true if mapping exists
    midiCtrl.hasMidiMapping = (ch, el) => {
        // midi inactive ?
        if(!midiCtrl.actMidiInDevID) return false;
        // check if mapping exists
        if(midiCtrl.midiCtrlMappings.filter( el2 => el2.param.ch == ch && el2.param.el.id == el.id).length > 0){
            return true;
        };
        return false;
    };
    // ui callback, called when param value changes
    midiCtrl.uiUpdateCallback = undefined;
    // update current parameter value in midi mapping, if automation for that parameter exists
    midiCtrl.updateAutoValue = (ch, el, val) =>{
        midiCtrl.midiCtrlMappings.forEach(el2 => {
            if(el2.param.ch != ch) return;
            if(el2.param.el.id != el.id) return;
            el2.param.el.current = val;
        });
    };
    // array of current mapping objects, each object consists of filter and params
    midiCtrl.midiCtrlMappings = [];
    // contains current parameter to be learned if in learn mode, otherwise undefined
    midiCtrl.learn = undefined;
    // parse midi event, returns object including all parsed data
    midiCtrl.parseMidiEvent = (e) => {
        // return value is undefined if no valid midi event for automation
        // otherwise {type, midiCh, key, value (always 14bit scaled)}
        let midiChannel = e.data[0] & 0x0F;
        switch (e.data[0] & 0xf0) {
            case 0x90:
                if (e.data[2] != 0) {  // if velocity != 0, this is a note-on message
                    //console.log('Note on');
                    return {type: 'n', ch: midiChannel, key: e.data[1], val: e.data[2]/127};
                }
            // if velocity == 0, fall thru: it's a note-off.  MIDI's weird, y'all.
            case 0x80:
                //console.log('Note off');
                return {type: 'n', ch: midiChannel, key: e.data[1], val: e.data[2]/127};
            case 0xa0:
                //.log('Poly Key Pressure value: ');
                break;
            case 0xB0:
                //.log('Controller: ' + e.data[1] + ' value: ' + e.data[2]);
                return {type: 'c', ch: midiChannel, key: e.data[1], val: e.data[2]/127};
            case 0xC0:
                //.log('Program change');
                break;
            case 0xD0:
                //console.log('Channel pressure');
                break;
            case 0xE0:
                //console.log('Pitch bend value: ' + (e.data[1] + (e.data[2] << 7)));
                return {type: 'p', ch: midiChannel, key: 0, val: (e.data[1] + (e.data[2] << 7))/16384};
            case 0xF0:
                //console.log('Sysx');
                break;
            default:
                break;
        }
        return undefined;
    }
}