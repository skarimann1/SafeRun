# Build shim
cd shim && make && cd ..

# Create signers
./scripts/generate_signer.sh "CN=Oracle Software/O=Oracle" oracle

# Build and Sign for an example (make sure the examples folder exists)
./scripts/sign_and_bundle.sh oracle_tool.cpp oracle

# Go into bundle and run (for oracle example)
cd demo/bundle_oracle
../../saferun ./oracle_tool

You should see saferun verify the signature, start controller, then the controller will print allow/block messages as the shim forwards events.

NOTES:

- If running on macOS use DYLD_INSERT_LIBRARIES instead of LD_PRELOAD. 