"""Behavior tests for optional binary aliases; accepts a component scripts path."""
import copy
import importlib.util
import os
from pathlib import Path
import sys
import unittest


SCRIPTS = Path(os.environ.get("GAMEDATA_SCRIPTS_DIR", Path(__file__).resolve().parents[1]))


def load(name, filename):
    spec = importlib.util.spec_from_file_location(name, SCRIPTS / filename)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


sync = load("alias_sync", "sync-gamedata.py")
validate = load("alias_validate", "validate-gamedata.py")


class BinaryAliasTests(unittest.TestCase):
    def setUp(self):
        self.document = {
            "schemaVersion": 5,
            "source": {"snapshotSchemaVersion": 8, "analysisOutputContractVersion": 3,
                       "gameVersion": "test-1"},
            "binaries": {"client": {"windows": {"crc64": "1234567890abcdef"}}},
            "records": [{"platform": "windows", "module": "client", "symbolName": "value",
                         "kind": "function", "payload": {"func_rva": "0x1000", "func_size": "0x10"}}],
        }
        self.manifest = sync.ConsumerManifest(
            name="test", index_url=None, game_versions=("test-1",),
            symbols={"client": {"value": "function"}}, optional_symbols={},
            numbered_patch_sets=(), strip_payload_fields={}, strip_record_fields=(),
            strip_top_level_fields=(), symbol_exemptions={}, alternative_groups=(), conditional_groups=())

    def test_alias_roundtrip(self):
        for aliases in ([], ["client_orig.dll", "client_original.dll", "client_org.dll"]):
            with self.subTest(aliases=aliases):
                self.document["binaries"]["client"]["windows"]["alias"] = aliases
                result = sync.prune_snapshot(self.document, self.manifest, "test-1")
                self.assertEqual(aliases, result["binaries"]["client"]["windows"]["alias"])
                self.assertEqual([], validate.validate_snapshot(result, "test-1")[0])

    def test_absent_alias_preserves_old_contract(self):
        result = sync.prune_snapshot(self.document, self.manifest, "test-1")
        self.assertNotIn("alias", result["binaries"]["client"]["windows"])
        self.assertEqual([], validate.validate_snapshot(result, "test-1")[0])

    def test_reject_invalid_aliases(self):
        for invalid in (None, "client_orig.dll", [1], [""], [".."], ["."],
                        ["../client.dll"], [r"..\client.dll"], [r"C:\client.dll"],
                        ["a:b.dll"], ["a\0.dll"], ["a*.dll"], ["client.dll."], ["client.dll "]):
            with self.subTest(invalid=invalid):
                doc = copy.deepcopy(self.document)
                doc["binaries"]["client"]["windows"]["alias"] = invalid
                self.assertTrue(validate.validate_snapshot(doc, "test-1")[0])


if __name__ == "__main__":
    unittest.main()
