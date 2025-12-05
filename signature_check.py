# py/signature_check.py
import subprocess
import hashlib
import os
import re

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
    sub = re.sub(r"\s*=\s*", "=", sub)
    sub = re.sub(r",\s*", ",", sub)
    return sub

def _find_cert(binary_path):
    directory = os.path.dirname(binary_path) or "."
    
    cert1 = binary_path + "-cert.pem"
    if os.path.exists(cert1):
        return cert1
    
    cert2 = os.path.join(directory, "cert.pem")
    if os.path.exists(cert2):
        return cert2
    
    for f in os.listdir(directory):
        if f.endswith("-cert.pem"):
            return os.path.join(directory, f)
    
    return None

def verify_signature(binary_path):
    sig_path = binary_path + ".sig"
    cert_path = _find_cert(binary_path)
    
    if not os.path.exists(sig_path):
        return ("UNKNOWN", False)
    
    if not cert_path:
        return ("UNKNOWN", False)

    digest = _sha256_hex(binary_path)
    tmp = binary_path + ".sha256.tmp"
    with open(tmp, "w") as f:
        f.write(digest + "\n")

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