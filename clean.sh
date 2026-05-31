if [ -d "build/cache" ]; then
    rm -rf build/cache
fi
if [ -d "build/vendor" ]; then
    rm -rf build/vendor
fi
if [ -f "build/bin.exe" ]; then
    rm build/bin.exe
fi
rm -f protocols/*-protocol.c protocols/*-protocol.h protocols/*-client-protocol.h
