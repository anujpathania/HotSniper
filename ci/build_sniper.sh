set -e

# PIN
wget https://software.intel.com/content/dam/develop/external/us/en/protected/pinplay-drdebug-3.2-pin-3.2-81205-gcc-linux.tar.gz
tar -xvzf pinplay-drdebug-3.2-pin-3.2-81205-gcc-linux.tar.gz
mv pinplay-drdebug-3.2-pin-3.2-81205-gcc-linux pin_kit

# Sniper
make
