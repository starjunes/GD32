fromelf .\object\cortex-m0.axf --bin --output .\object\cortex-m0.bin
copy .\object\cortex-m0.bin cortex-m0.bin
copy .\object\cortex-m0.hex cortex-m0.hex
binchk.exe

