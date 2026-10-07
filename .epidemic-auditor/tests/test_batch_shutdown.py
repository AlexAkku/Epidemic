import os
from pathlib import Path
import tempfile
import unittest
import uuid

import test_batch_orchestration as fixture


class BatchShutdownTests(unittest.TestCase):
    def test_shutdown_between_children_preserves_actual_cancellation_reason(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("batch_shutdown_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            fixture.BatchOrchestrationTests.execute_case(self, base, "application_shutdown")
