import importlib.util, json, unittest
from pathlib import Path
spec = importlib.util.spec_from_file_location("watch", Path(__file__).resolve().parents[1] / "check_releases.py")
w = importlib.util.module_from_spec(spec); spec.loader.exec_module(w)
C = {"id":"experimental","name":"Experimental","steamAppId":"1890860","initialWorkingBuildId":None}
def issue(build="123", labels=(), state="closed", closed="2026-01-01T00:00:00Z"):
    m={"schema":1,"channel":"experimental","steamAppId":"1890860","buildId":build}
    return {"number":1,"state":state,"closed_at":closed,"body":w.PREFIX+json.dumps(m),"labels":[{"name":x} for x in labels]}
S='''"1874880"\n{\n"depots"\n{\n"branches"\n{\n"public"\n{\n"buildid" "24681357"\n}\n}\n}\n}'''
class Tests(unittest.TestCase):
    def test_parse(self): self.assertEqual(w.parse_public_build_id(S), "24681357")
    def test_missing(self):
        with self.assertRaises(ValueError): w.parse_public_build_id('"branches"\n{\n}')
    def test_tracked(self): self.assertTrue(w.tracked([issue()], "experimental", "123"))
    def test_new(self): self.assertFalse(w.tracked([issue()], "experimental", "999"))
    def test_closed_not_pass(self): self.assertIsNone(w.baseline([issue()], C))
    def test_pass(self): self.assertEqual(w.baseline([issue(labels=["compatibility:passed"])], C), "123")
    def test_pending_pass_invalid(self): self.assertIsNone(w.baseline([issue(labels=["compatibility:passed","compatibility:pending"])], C))
    def test_failed_pass_invalid(self): self.assertIsNone(w.baseline([issue(labels=["compatibility:passed","compatibility:failed"])], C))
    def test_open_pass_invalid(self): self.assertIsNone(w.baseline([issue(labels=["compatibility:passed"], state="open")], C))
    def test_latest_pass(self):
        a=issue("111",["compatibility:passed"],closed="2026-01-01T00:00:00Z"); b=issue("222",["compatibility:passed"],closed="2026-02-01T00:00:00Z"); b["number"]=2
        self.assertEqual(w.baseline([a,b], C), "222")
    def test_initial(self): self.assertEqual(w.baseline([], dict(C,initialWorkingBuildId="777")), "777")
    def test_config(self): self.assertEqual(w.validate_config({"channels":[dict(C)]})[0]["steamAppId"], "1890860")
    def test_duplicate(self):
        with self.assertRaises(ValueError): w.validate_config({"channels":[dict(C),dict(C,id="other")]})
    def test_body(self): self.assertIn('"buildId":"123"', w.body_for(C,"123","100"))
if __name__ == "__main__": unittest.main()
