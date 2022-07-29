rm -f fda

xcrun -sdk iphoneos clang -arch arm64 -Wall -O3 -o fda main.c find_kernel_base_under_checkra1n.c

codesign -s - --entitlements monkeydev.entitlements fda


