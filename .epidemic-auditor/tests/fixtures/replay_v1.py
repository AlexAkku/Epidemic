"""Controlled analyzer V1: retain externally captured integer observations."""
import json

from ep_auditor.core.lineage import normalized_record

VERSION = "fixture-analyzer-1"


def analyze(raw_bytes, raw_reference):
    observations = [json.loads(line) for line in raw_bytes.splitlines()]
    return [normalized_record({"schemaVersion": 1, "eventType": "fixture.observation", "required": True,
                               "provenance": "external", "confidence": "observed", "value": item["value"]},
                              {"fixture.observation"}, raw_reference, index)
            for index, item in enumerate(observations)]
