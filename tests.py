from configparser import ConfigParser
from datetime import datetime
from io import TextIOWrapper
import os
from pathlib import Path
import random
from typing import KeysView, List, Set, Tuple
import unittest
from zipfile import ZipFile

from solution import ParentChild, Release, ReleaseMatcher

_time_format = "%Y-%m-%dT%H:%M:%S"


def _start_loading_from_file(name: str, archive: str) -> Tuple[ConfigParser, List[ParentChild]]:
    config = ConfigParser()
    path = f"{name}.ini"
    mp = Path(__file__)
    if (arch := mp.with_name(archive)).exists():
        with ZipFile(arch) as zip:
            config.read_file(TextIOWrapper(zip.open(path), encoding="ascii"), path)
    else:
        config.read(mp.with_name(path))
    dag = []
    for parent, children in config["dag"].items():
        for child in children.split(","):
            dag.append(ParentChild(parent=parent, child=child))
    random.seed(int(os.getenv("SEED", "777")))
    random.shuffle(dag)
    return config, dag


class CommitsOfTests(unittest.TestCase):
    @staticmethod
    def _load_test_from_file(name: str) -> Tuple[List[ParentChild],
                                                 List[Release],
                                                 List[Tuple[Release, Set[str]]]]:
        config, dag = _start_loading_from_file(name, "commits_of_tests.zip")
        releases = []
        for name, details in config["releases"].items():
            commit, ts = details.split(",")
            ts = datetime.strptime(ts, _time_format)
            releases.append(Release(name=name, commit=commit, published_at=ts))
        random.shuffle(releases)
        tasks = []
        for name, details in config["tasks"].items():
            commit, ts, *belongings = details.split(",")
            ts = datetime.strptime(ts, _time_format)
            tasks.append((Release(name=name, commit=commit, published_at=ts), set(belongings)))
        random.shuffle(tasks)
        return dag, releases, tasks

    def _test(self, name: str) -> None:
        dag, releases, tasks = self._load_test_from_file(name)
        matcher = ReleaseMatcher(dag, releases)
        for release, answer in tasks:
            yours = matcher.commits_of(release)
            self.assertIsInstance(yours, (set, frozenset, KeysView))
            self.assertEqual(yours, answer, msg=release.name)

    def test_example(self):
        self._test("example")

    def test_empty_everything(self):
        ReleaseMatcher([], [])

    def test_empty_initial(self):
        self._test("empty_initial")

    def test_same_commit(self):
        self._test("same_commit")

    def test_multiple_roots(self):
        self._test("multiple_roots")

    def test_disjoint(self):
        self._test("disjoint")

    def test_leak(self):
        self._test("leak")

    def test_go_git(self):
        self._test("go_git")

    def test_git(self):
        self._test("git")


class DiffTests(unittest.TestCase):
    @staticmethod
    def _load_test_from_file(name: str) -> Tuple[List[ParentChild],
                                                 List[Tuple[Release, Release, Set[str]]]]:
        config, dag = _start_loading_from_file(name, "diff_tests.zip")
        tasks = []
        for names, rest in config["tasks"].items():
            name1, name2 = names.split(",")
            commit1, ts1, commit2, ts2, *diff = rest.split(",")
            ts1 = datetime.strptime(ts1, _time_format)
            ts2 = datetime.strptime(ts2, _time_format)
            tasks.append((Release(name=name1, commit=commit1, published_at=ts1),
                          Release(name=name2, commit=commit2, published_at=ts2),
                          set(diff)))
        random.shuffle(tasks)
        return dag, tasks

    def _test(self, name: str) -> None:
        dag, tasks = self._load_test_from_file(name)
        matcher = ReleaseMatcher(dag, [])
        for release1, release2, answer in tasks:
            yours = matcher.diff(release1, release2)
            self.assertIsInstance(yours, (set, frozenset, KeysView))
            self.assertEqual(yours, answer, msg=f"{release1.name} <> {release2.name}")

    def test_example(self):
        self._test("example")

    def test_different_roots(self):
        self._test("different_roots")

    def test_disjoint_roots(self):
        self._test("disjoint_roots")

    def test_leak(self):
        self._test("leak")

    def test_torture(self):
        self._test("go_git")


if __name__ == "__main__":
    unittest.main()
