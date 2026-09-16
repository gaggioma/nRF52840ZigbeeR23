import * as m from 'zigbee-herdsman-converters/lib/modernExtend';

export default {
    zigbeeModel: ['nRF'],
    model: 'nRF',
    vendor: 'Marco Corp.',
    description: 'Automatically generated definition',
    extend: [
        m.deviceEndpoints({"endpoints":{"1": 1, "2": 2, "3": 3, "4": 4, "5": 5}}),
        m.battery({voltage: true, voltageReporting: true, percentageReportingConfig:{"min": "1_HOUR", "max": "MAX", "change": 0}, voltageReportingConfig: {"min": "1_HOUR", "max": "MAX", "change": 0} }),
        m.binary(
        {"name":"alarm_activation", "label":"alarm_activation","cluster":"genBinaryInput","attribute":"presentValue","reporting":{"min":"MIN","max":"MAX","change":1},"valueOn":["ON",1],"valueOff":["OFF",0],"description":"Binary Input alarm on endpoint 2","access":"STATE_GET","endpointName":"2"}
        ),
        m.binary(
        {"name":"alarm_int", "label":"alarm_int","cluster":"genBinaryInput","attribute":"presentValue","reporting":{"min":"MIN","max":"MAX","change":1},"valueOn":["ON",1],"valueOff":["OFF",0],"description":"Binary Input alarm on endpoint 3","access":"STATE_GET","endpointName":"3"}
        ),
        m.binary(
        {"name":"alarm_ext_sud", "label":"alarm_ext_sud","cluster":"genBinaryInput","attribute":"presentValue","reporting":{"min":"MIN","max":"MAX","change":1},"valueOn":["ON",1],"valueOff":["OFF",0],"description":"Binary Input alarm on endpoint 4","access":"STATE_GET","endpointName":"4"}
        ),
        m.binary(
        {"name":"alarm_ext_nord", "label":"alarm_ext_nord","cluster":"genBinaryInput","attribute":"presentValue","reporting":{"min":"MIN","max":"MAX","change":1},"valueOn":["ON",1],"valueOff":["OFF",0],"description":"Binary Input alarm on endpoint 5","access":"STATE_GET","endpointName":"5"}
        )
        ],
};