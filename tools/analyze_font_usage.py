"""Count which embedded font each text component in the game bundle actually uses.

IL2CPP release bundles ship without MonoBehaviour type trees, so:
  * the owning script class comes from the m_Script PPtr -> MonoScript assets (plain text);
  * the owning GameObject comes from the m_GameObject PPtr -> GameObject.m_Name;
  * font usage is counted by finding the font asset's PPtr pathID bytes in the component data.

MonoBehaviour header layout observed in this bundle (typeless object):
  0  int32  m_GameObject.fileID
  4  int64  m_GameObject.pathID
  12 int32  m_Script.fileID
  16 int32  padding
  20 int64  m_Script.pathID
  28 int32  m_Name length (UTF-8)
"""
from __future__ import annotations

import collections
import json
import os
import struct
import sys

BUNDLE = r"G:\Gaga\植物大战僵尸融合版3.9\植物大战僵尸融合版3.9\PlantsVsZombiesRH_Data\data.unity3d"
WS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(WS, "analysis")
os.makedirs(OUT, exist_ok=True)


def main() -> int:
    import UnityPy

    print("loading bundle ...", flush=True)
    env = UnityPy.load(BUNDLE)

    scripts: dict[str, dict[int, str]] = collections.defaultdict(dict)
    scripts_anywhere: dict[int, list[str]] = collections.defaultdict(list)
    gameobjects: dict[str, dict[int, str]] = collections.defaultdict(dict)
    go_anywhere: dict[int, list[str]] = collections.defaultdict(list)
    fonts: dict[str, dict[int, str]] = collections.defaultdict(dict)
    mbs: list[tuple[str, int, bytes]] = []
    ext_map: dict[str, dict[int, str]] = {}

    n = 0
    for obj in env.objects:
        n += 1
        if n % 50000 == 0:
            print(f"  scanned {n}", flush=True)
        tname = obj.type.name
        af = obj.assets_file
        fname = af.name
        if fname not in ext_map:
            ext_map[fname] = {
                i + 1: os.path.basename(str(fi.path).replace("\\", "/"))
                for i, fi in enumerate(getattr(af, "externals", []) or [])
            }
        try:
            if tname == "MonoScript":
                d = obj.read()
                cls = getattr(d, "m_ClassName", "") or ""
                ns = getattr(d, "m_Namespace", "") or ""
                full = f"{ns}.{cls}" if ns else cls
                scripts[fname][obj.path_id] = full
                scripts_anywhere[obj.path_id].append(full)
            elif tname == "GameObject":
                d = obj.read()
                gameobjects[fname][obj.path_id] = getattr(d, "m_Name", "") or ""
                go_anywhere[obj.path_id].append(gameobjects[fname][obj.path_id])
            elif tname == "Font":
                d = obj.read()
                fonts[fname][obj.path_id] = getattr(d, "m_Name", "") or f"font_{obj.path_id}"
            elif tname == "MonoBehaviour":
                mbs.append((fname, obj.path_id, bytes(obj.get_raw_data())))
        except Exception as exc:  # noqa: BLE001
            print(f"  !! {tname} {obj.path_id}: {type(exc).__name__}: {exc}", flush=True)

    print(f"scanned {n}; monoscripts={sum(len(v) for v in scripts.values())} "
          f"gameobjects={sum(len(v) for v in gameobjects.values())} "
          f"fonts={ {k: v for k, v in fonts.items()} } mbs={len(mbs)}", flush=True)

    def resolve(mb_file: str, file_id: int, path_id: int, table, anywhere):
        if file_id == 0:
            hit = table.get(mb_file, {}).get(path_id)
            if hit is not None:
                return hit
        target = ext_map.get(mb_file, {}).get(file_id)
        if target and target in table:
            hit = table[target].get(path_id)
            if hit is not None:
                return hit
        if file_id == 0 and path_id in anywhere and len(anywhere[path_id]) == 1:
            return anywhere[path_id][0]
        return None

    # ---- classify every MonoBehaviour -------------------------------------------
    records = []
    class_counts = collections.Counter()
    for fname, pid, raw in mbs:
        if len(raw) < 32:
            continue
        go_fid, go_pid = struct.unpack_from("<i", raw, 0)[0], struct.unpack_from("<q", raw, 4)[0]
        sc_fid, sc_pid = struct.unpack_from("<i", raw, 12)[0], struct.unpack_from("<q", raw, 20)[0]
        cls = resolve(fname, sc_fid, sc_pid, scripts, scripts_anywhere) or f"<unknown:{sc_pid}>"
        class_counts[cls] += 1
        records.append((fname, pid, raw, cls, go_fid, go_pid))

    # ---- TMP font assets and their source TTF ------------------------------------
    font_path_ids: dict[int, str] = {}
    for f, table in fonts.items():
        for pid, name in table.items():
            font_path_ids[pid] = f"{name} [{f}]"

    tmp_assets: dict[int, tuple[str, list[str]]] = {}
    for fname, pid, raw, cls, _, _ in records:
        if cls.endswith(("TMP_FontAsset", "FontAsset")):
            name = raw[32:32 + struct.unpack_from("<i", raw, 28)[0]].decode("utf-8", "replace")
            src = [n for p, n in font_path_ids.items() if struct.pack("<q", p) in raw[32:]]
            tmp_assets[pid] = (name or f"tmp_{pid}", src)

    # ---- which font does each text component use ---------------------------------
    text_classes = sorted({c for c in class_counts
                           if c.endswith(("TextMeshProUGUI", "TextMeshPro", "Text", "InputField",
                                          "UIText", "TextEx", "Label", "RichText"))})
    print("\ntext-ish classes:", text_classes, flush=True)

    candidates: list[tuple[int, str, str]] = []  # (path_id, label, kind)
    for pid, label in font_path_ids.items():
        candidates.append((pid, label, "legacy Font"))
    for pid, (name, src) in tmp_assets.items():
        candidates.append((pid, f"{name} (TMP asset, src={src})", "TMP asset"))

    usage = collections.Counter()
    go_names = collections.defaultdict(collections.Counter)
    comp_counts = collections.Counter()
    for fname, pid, raw, cls, go_fid, go_pid in records:
        if cls not in text_classes:
            continue
        comp_counts[cls] += 1
        body = raw[32:]
        hit = None
        for cand_pid, label, kind in candidates:
            if struct.pack("<q", cand_pid) in body:
                hit = label
                break
        if hit is None:
            usage["<none found in serialized data>"] += 1
            continue
        usage[hit] += 1
        go_name = resolve(fname, go_fid, go_pid, gameobjects, go_anywhere) or "?"
        go_names[hit][go_name] += 1

    result = {
        "bundle": BUNDLE,
        "objects_scanned": n,
        "text_components_by_class": dict(comp_counts),
        "monobehaviour_classes_top60": class_counts.most_common(60),
        "legacy_fonts": font_path_ids,
        "tmp_font_assets": {str(k): {"name": v[0], "source_ttf": v[1]} for k, v in tmp_assets.items()},
        "font_usage": dict(usage),
        "gameobject_names_per_font": {k: v.most_common(20) for k, v in go_names.items()},
    }
    with open(os.path.join(OUT, "font-usage.json"), "w", encoding="utf-8") as fh:
        json.dump(result, fh, ensure_ascii=False, indent=2)

    print("\n=== MonoBehaviour classes (top 60) ===")
    for c, v in class_counts.most_common(60):
        print(f"{v:7d}  {c}")
    print("\n=== legacy Font assets ===")
    for pid, name in sorted(font_path_ids.items()):
        print(f"  {pid}: {name}")
    print("\n=== TMP font assets ===")
    for pid, (name, src) in sorted(tmp_assets.items()):
        print(f"  {pid}: {name} <- {src}")
    print("\n=== font usage among text components ===")
    for label, v in usage.most_common():
        print(f"{v:7d}  {label}")
    print("\nwrote", os.path.join(OUT, "font-usage.json"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
