import importlib.util
import json
from pathlib import Path
import unittest
spec = importlib.util.spec_from_file_location("watch", Path(__file__).resolve().parents[1] / "check_dependencies.py")
w = importlib.util.module_from_spec(spec)
spec.loader.exec_module(w)
MOD = "69A4D664A6284E06"
def issue(labels=(), state="closed"):
    return {"number": 1, "state": state, "closed_at": "2026-01-01T00:00:00Z",
        "body": w.PREFIX + json.dumps({"schema": 1, "modId": MOD, "version": "1.0"}),
        "labels": [{"name": x} for x in labels]}
class Tests(unittest.TestCase):
    def test_duplicate_closed(self):
        self.assertTrue(w.tracked([issue()], MOD, "1.0"))
    def test_new_version(self):
        self.assertFalse(w.tracked([issue()], MOD, "2.0"))
    def test_closed_is_not_pass(self):
        self.assertIsNone(w.baseline([issue()], {"modId": MOD}))
    def test_pass(self):
        self.assertEqual(w.baseline([issue(["compatibility:passed"])], {"modId": MOD}), "1.0")
    def test_conflicting_result(self):
        self.assertIsNone(w.baseline([issue(["compatibility:passed", "compatibility:failed"])], {"modId": MOD}))
    def test_pending_result(self):
        self.assertIsNone(w.baseline([issue(["compatibility:passed", "compatibility:pending"])], {"modId": MOD}))
    def test_open_pass(self):
        self.assertIsNone(w.baseline([issue(["compatibility:passed"], "open")], {"modId": MOD}))
    def test_initial_baseline(self):
        self.assertEqual(w.baseline([], {"modId": MOD, "initialWorkingVersion": "0.9"}), "0.9")
    def test_schema(self):
        self.assertEqual(w.version_of({"status": "success", "mod": {"id": MOD, "version": "1.0"}}, MOD), "1.0")
    def test_missing_version(self):
        with self.assertRaises(ValueError):
            w.version_of({"status": "success", "mod": {"id": MOD}}, MOD)
    def test_wrong_id(self):
        with self.assertRaises(ValueError):
            w.version_of({"status": "success", "mod": {"id": "OTHER", "version": "1.0"}}, MOD)
    def test_bad_metadata(self):
        self.assertEqual(w.meta({"body": w.PREFIX + "bad json"}), {})
