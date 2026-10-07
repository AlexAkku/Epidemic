"""Controlled analyzer V2: additionally derive whether each observation is positive."""
import json

from ep_auditor.core.lineage import normalized_record

VERSION = "fixture-analyzer-2"


def analyze(raw_bytes, raw_reference):
    observations = [json.loads(line) for line in raw_bytes.splitlines()]
    return [normalized_record({"schemaVersion": 1, "eventType": "fixture.observation", "required": True,
                               "provenance": "external", "confidence": "observed", "value": item["value"],
                               "derivedPositive": {"value": item["value"] > 0, "confidence": "derived"}},
                              {"fixture.observation"}, raw_reference, index)
            for index, item in enumerate(observations)]
