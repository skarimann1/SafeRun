File Structure

saferun-project/
├─ policy.yaml
├─ scripts/
│  ├─ generate_signer.sh
│  └─ sign_and_bundle.sh
├─ saferun            # python, executable
├─ controller         # python, executable
├─ py/
│  └─ signature_check.py
├─ shim/
│  ├─ preload.c
│  └─ Makefile
├─ examples/
└─ demo/
