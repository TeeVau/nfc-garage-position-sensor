const m = require('zigbee-herdsman-converters/lib/modernExtend');
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const reporting = require('zigbee-herdsman-converters/lib/reporting');

const ea = exposes.access;
const e = exposes.presets;

const directionFromMultistateInput = {
  cluster: 'genMultistateInput',
  type: ['attributeReport', 'readResponse'],
  convert: (model, msg) => {
    if (msg.endpoint.ID !== 11 || msg.data.presentValue === undefined) {
      return null;
    }

    const value = Number(msg.data.presentValue);
    const lookup = {
      0: 'unknown',
      1: 'opening',
      2: 'closing',
      3: 'stopped',
    };

    return {
      direction: lookup[value] ?? 'unknown',
      direction_code: value,
    };
  },
};

module.exports = {
  zigbeeModel: ['nfc-garage-position-sensor'],
  model: 'nfc-garage-position-sensor',
  vendor: 'TeeVau',
  description: 'NFC garage door position sensor with direction endpoint',
  fromZigbee: [directionFromMultistateInput],
  extend: [
    m.deviceEndpoints({
      endpoints: {default: 10, direction: 11},
      multiEndpointSkip: ['position', 'state', 'tilt', 'direction', 'direction_code'],
    }),
    m.windowCovering({controls: ['lift']}),
  ],
  exposes: [
    e.enum('direction', ea.STATE, ['unknown', 'opening', 'closing', 'stopped']).withDescription('Current garage door movement direction'),
    e.numeric('direction_code', ea.STATE).withDescription('Current garage door movement direction as numeric code'),
  ],
  meta: {multiEndpoint: true},
  configure: async (device, coordinatorEndpoint) => {
    const directionEndpoint = device.getEndpoint(11);

    if (!directionEndpoint) {
      return;
    }

    await reporting.bind(directionEndpoint, coordinatorEndpoint, ['genMultistateInput']);
    await directionEndpoint.configureReporting('genMultistateInput', [
      {
        attribute: 'presentValue',
        minimumReportInterval: 0,
        maximumReportInterval: 3600,
        reportableChange: 1,
      },
    ]);
  },
};
