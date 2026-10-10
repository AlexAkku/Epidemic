import sqlite3
import tempfile
import unittest
from pathlib import Path

import test_core
from ep_auditor.core.storage import RunStorage


class StorageTransactionTests(unittest.TestCase):
    def test_sqlite_connection_closed_after_success_and_rollback(self):
        with tempfile.TemporaryDirectory() as directory:
            config, _ = test_core.AuditorCoreTests()._config(Path(directory))
            storage = RunStorage(config)
            for abort in (False, True):
                connection = None
                try:
                    try:
                        with storage._connect() as connection:
                            connection.execute("SELECT 1").fetchone()
                            if abort:
                                raise RuntimeError("controlled rollback")
                    except RuntimeError:
                        pass
                    with self.assertRaises(sqlite3.ProgrammingError):
                        connection.execute("SELECT 1")
                finally:
                    if connection is not None:
                        connection.close()


if __name__ == "__main__":
    unittest.main()
