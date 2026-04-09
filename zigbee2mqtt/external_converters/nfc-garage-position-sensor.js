const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const reporting = require('zigbee-herdsman-converters/lib/reporting');

const e = exposes.presets;
const ea = exposes.access;

const ZIGBEE_COVERING_ENDPOINT = 10;

const definition = {
  fingerprint: [
    {
      modelID: 'nfc-garage-position-sensor',
      manufacturerName: 'TeeVau',
    },
  ],
  model: 'nfc-garage-position-sensor',
  vendor: 'TeeVau',
  description: 'Garage door position sensor based on PN532 NFC tags',
  fromZigbee: [fz.cover_position_tilt],
  toZigbee: [],
  exposes: [
    e
      .numeric('position', ea.STATE)
      .withUnit('%')
      .withValueMin(0)
      .withValueMax(100)
      .withDescription('Garage door opening percentage with 0 = closed and 100 = open'),
    e
      .enum('state', ea.STATE, ['OPEN', 'CLOSE', 'STOP', 'OPENING', 'CLOSING'])
      .withDescription('Garage door state derived from the window covering cluster'),
  ],
  endpoint: () => {
    return {default: ZIGBEE_COVERING_ENDPOINT};
  },
  configure: async (device, coordinatorEndpoint) => {
    const endpoint = device.getEndpoint(ZIGBEE_COVERING_ENDPOINT);

    await reporting.bind(endpoint, coordinatorEndpoint, ['closuresWindowCovering']);
    await reporting.currentPositionLiftPercentage(endpoint);

    if (!device.powerSource || device.powerSource === 'Unknown') {
      device.powerSource = 'Mains (single phase)';
      device.save();
    }
  },
};

module.exports = definition;
