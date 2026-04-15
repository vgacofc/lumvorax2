#!/usr/bin/env python3
import base64
import hashlib
import json
import os
import subprocess
import time
import urllib.request
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
FILES = [
    ("hubbard_standard_names", ROOT / "src/advanced_calculations/quantum_problem_hubbard_hts/STANDARD_NAMES.md"),
    ("root_standard_names", ROOT / "STANDARD_NAMES.md"),
    ("prompt", ROOT / "prompt.txt"),
]


def read_file(path):
    content = path.read_text(encoding="utf-8")
    return content, hashlib.sha256(content.encode("utf-8")).hexdigest()


def request_json(url, method, headers, payload=None, timeout=25):
    data = None
    if payload is not None:
        data = json.dumps(payload).encode()
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    with urllib.request.urlopen(req, timeout=timeout) as response:
        body = response.read().decode()
        return json.loads(body) if body else {"status": response.status}


def sync_supabase(entries):
    supa_url = (os.environ.get("SUPABASE_URL") or os.environ.get("SUPABASE8_API_URL") or "").rstrip("/")
    key = os.environ.get("SUPABASE_SERVICE_ROLE_KEY", "")
    if not supa_url or not key:
        return {"ok": False, "reason": "SUPABASE_URL/SUPABASE_SERVICE_ROLE_KEY absents"}
    headers = {
        "apikey": key,
        "Authorization": f"Bearer {key}",
        "Content-Type": "application/json",
        "Prefer": "resolution=merge-duplicates,return=minimal",
    }
    payload = []
    for name, path, sha, content in entries:
        payload.append({
            "section": "LUMVORAX_C43_ARTIFACT_SYNC",
            "key_name": f"{name}_sha256",
            "value_desc": f"{path.relative_to(ROOT)} sha256={sha} bytes={len(content.encode('utf-8'))}",
            "file_origin": str(path.relative_to(ROOT)),
            "version": "4.3",
            "cycle": "C43",
            "is_canonical": True,
        })
    request_json(f"{supa_url}/rest/v1/standard_names_registry?on_conflict=section,key_name", "POST", headers, payload)
    return {"ok": True, "count": len(payload)}


def sync_vercel(entries):
    token = os.environ.get("VERCEL_TOKEN") or os.environ.get("VERCEL_API_KEY") or ""
    if not token:
        return {"ok": False, "reason": "VERCEL_TOKEN/VERCEL_API_KEY absent"}
    uploaded = []
    for name, path, sha, content in entries:
        headers = {
            "Authorization": f"Bearer {token}",
            "Content-Type": "text/plain; charset=utf-8",
            "x-vercel-filename": path.name,
        }
        req = urllib.request.Request("https://api.vercel.com/v2/blob/upload", data=content.encode("utf-8"), headers=headers, method="PUT")
        with urllib.request.urlopen(req, timeout=30) as response:
            body = json.loads(response.read().decode())
        uploaded.append({"name": name, "sha256": sha, "url": body.get("url", "")})
    return {"ok": True, "uploaded": uploaded}


def sync_supermemory(entries):
    key = os.environ.get("SUPERMEMORY_API_KEY", "")
    if not key:
        return {"ok": False, "reason": "SUPERMEMORY_API_KEY absent"}
    uploaded = []
    for name, path, sha, content in entries:
        payload = {
            "content": content,
            "metadata": {
                "source": "LumVorax",
                "event": "c43_artifact_sync",
                "artifact": name,
                "path": str(path.relative_to(ROOT)),
                "sha256": sha,
                "cycle": "C43",
                "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            },
            "containerTags": ["lumvorax_canonical_artifacts"],
        }
        headers = {"Authorization": f"Bearer {key}", "Content-Type": "application/json"}
        request_json("https://api.supermemory.ai/v3/documents", "POST", headers, payload)
        uploaded.append({"name": name, "sha256": sha})
    return {"ok": True, "uploaded": uploaded}


def sync_doppler(entries):
    token = os.environ.get("DOPPLER_TOKEN", "")
    if not token:
        return {"ok": False, "reason": "DOPPLER_TOKEN absent"}
    if subprocess.run(["bash", "-lc", "command -v doppler >/dev/null"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode != 0:
        return {"ok": False, "reason": "CLI doppler absent"}
    env_pairs = {}
    for name, path, sha, content in entries:
        env_pairs[f"LUMVORAX_{name.upper()}_SHA256"] = sha
    cmd = ["doppler", "secrets", "set", "--token", token, "--project", "lumvorax", "--config", "dev_lumvorax"]
    cmd.extend([f"{key}={value}" for key, value in env_pairs.items()])
    proc = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=45)
    return {"ok": proc.returncode == 0, "returncode": proc.returncode, "output": proc.stdout[-1000:]}


def main():
    entries = []
    for name, path in FILES:
        content, sha = read_file(path)
        entries.append((name, path, sha, content))
    result = {
        "schema": "lumvorax_c43_artifact_sync_result_v1",
        "created_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "artifacts": [{"name": name, "path": str(path.relative_to(ROOT)), "sha256": sha, "bytes": len(content.encode("utf-8"))} for name, path, sha, content in entries],
    }
    for label, func in [("supabase", sync_supabase), ("vercel", sync_vercel), ("supermemory", sync_supermemory), ("doppler", sync_doppler)]:
        try:
            result[label] = func(entries)
        except Exception as exc:
            result[label] = {"ok": False, "reason": str(exc)}
    out = ROOT / "logs/forensic/lumvorax_c43_artifact_sync_result.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if any(result.get(k, {}).get("ok") for k in ("supabase", "vercel", "supermemory", "doppler")) else 1


if __name__ == "__main__":
    raise SystemExit(main())