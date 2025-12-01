# py/signature_check.py
import subprocess, hashlib, os, re

def _sha256_hex(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        h.update(f.read())
    return h.hexdigest()

def _extract_subject(cert_path):
    out = subprocess.check_output(["openssl", "x509", "-noout", "-subject", "-in", cert_path])
    out = out.decode('utf-8').strip()
    sub = out.replace("subject=", "").strip()
    sub = re.sub(r"\s+", " ", sub)
    return sub

def verify_signature(binary_path):
    sig_path = binary_path + ".sig"
    # assume cert next to binary named "<binary>-cert.pem" OR binary-cert.pem; try both
    cert_guess1 = binary_path + "-cert.pem"
    cert_guess2 = os.path.join(os.path.dirname(binary_path), "cert.pem")
    cert_path = cert_guess1 if os.path.exists(cert_guess1) else (cert_guess2 if os.path.exists(cert_guess2) else None)
    if not os.path.exists(sig_path) or not cert_path:
        return ("UNKNOWN", False)

    digest = _sha256_hex(binary_path)
    tmp = binary_path + ".sha256.tmp"
    with open(tmp, "w") as f:
        f.write(digest)

    proc = subprocess.run([
        "openssl", "smime", "-verify",
        "-in", sig_path, "-inform", "DER",
        "-content", tmp,
        "-certfile", cert_path,
        "-noverify"
    ], capture_output=True)

    os.remove(tmp)
    ok = (proc.returncode == 0)
    if ok:
        signer = _extract_subject(cert_path)
        return (signer, True)
    return ("UNKNOWN", False)
