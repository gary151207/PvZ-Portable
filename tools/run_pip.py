"""Run pip inside a process where tempfile.mkdtemp creates 0777 dirs.

The DSH Windows sandbox denies writes inside directories created with mode 0700,
which is what tempfile.mkdtemp uses by default, so pip's unpack/target temp dirs
end up unwritable.
"""
import os
import sys
import tempfile


def mkdtemp_777(suffix=None, prefix=None, dir=None):
    suffix = suffix or ""
    prefix = prefix or "tmp"
    dir = dir or tempfile.gettempdir()
    for _ in range(10000):
        path = os.path.join(dir, prefix + os.urandom(6).hex() + suffix)
        try:
            os.mkdir(path, 0o777)
            return path
        except FileExistsError:
            continue
    raise FileExistsError("could not create temp dir")


tempfile.mkdtemp = mkdtemp_777

from pip._internal.cli.main import main  # noqa: E402

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
