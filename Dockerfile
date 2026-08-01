FROM ubuntu:24.04 AS build

RUN dpkg --add-architecture armhf && \
    apt-get update && \
    apt-get install -y build-essential git g++-arm-linux-gnueabihf libc6:armhf libstdc++6:armhf flex bison curl \
        pkg-config libfreetype-dev libx11-dev libfreetype-dev:armhf libx11-dev:armhf \
        cmake ninja-build python3 python3-packaging

RUN cd /opt && \
    curl -LO https://github.com/mstorsjo/llvm-mingw/releases/download/20260602/llvm-mingw-20260602-ucrt-ubuntu-22.04-aarch64.tar.xz && \
    tar -Jxvf llvm-mingw-*.tar.xz && \
    rm llvm-mingw-*.tar.xz && \
    mv llvm-mingw-* llvm-mingw

ENV PATH=/opt/llvm-mingw/bin:$PATH

WORKDIR /build
COPY . wine

RUN mkdir wine-build32 && \
    cd wine-build32 && \
    ../wine/configure --prefix=/opt/wine --disable-tests --host=arm-linux-gnueabihf && \
    make -j$(nproc) && \
    make -j$(nproc) install-lib
RUN mkdir wine-build64 && \
    cd wine-build64 && \
    ../wine/configure --prefix=/opt/wine --disable-tests --enable-archs=aarch64,arm64ec,i386 && \
    make -j$(nproc) && \
    make -j$(nproc) install-lib

RUN git clone --recurse-submodules https://github.com/FEX-emu/FEX && \
    cd FEX && \
    git checkout --recurse-submodules FEX-2607

RUN cd FEX && \
    mkdir build-unix && \
    cd build-unix && \
    cmake ../Source/Windows/UnixLib \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/fex \
        -DCMAKE_INSTALL_LIBDIR=/opt/wine/lib/wine/aarch64-unix && \
    ninja && \
    ninja install

RUN cd FEX && \
    for arch in aarch64 arm64ec; do \
        mkdir build-$arch && \
        cd build-$arch && \
        cmake .. \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_TOOLCHAIN_FILE=../Data/CMake/toolchain_mingw.cmake \
            -DENABLE_LTO=False \
            -DMINGW_TRIPLE=$arch-w64-mingw32 \
            -DCMAKE_SYSTEM_PROCESSOR=$arch \
            -DFMT_MODULE=FALSE \
            -DBUILD_TESTING=False \
            -DENABLE_JEMALLOC_GLIBC_ALLOC=False \
            -DCMAKE_INSTALL_PREFIX=/opt/fex \
            -DCMAKE_INSTALL_LIBDIR=/opt/wine/lib/wine/aarch64-windows && \
        ninja && \
        ninja install && \
        cd .. || exit 1; \
    done && \
    cd /opt/wine/lib/wine/aarch64-windows && \
    mv libarm64ecfex.dll xtajit64.dll && \
    mv libwow64fex.dll xtajit.dll

FROM ubuntu:24.04 AS runtime

RUN dpkg --add-architecture armhf && \
    apt-get update && \
    apt-get install -y libc6:armhf libstdc++6:armhf \
        libx11-6 libfreetype6 libx11-6:armhf libfreetype6:armhf \
        build-essential git curl rsync sudo

# Don't require "-y" to apt-get, to match regular github action runner
# environments.
RUN echo 'APT::Get::Assume-Yes "true";' > /etc/apt/apt.conf.d/90assumeyes

COPY --from=build /opt/wine /opt/wine
ENV PATH=/opt/wine/bin:$PATH

# Set a fixed WINEPREFIX, regardless of $HOME; github action runners run the
# container with a custom $HOME with a different uid, causing "wine:
# '/github/home' is not owned by you, refusing to create a configuration
# directory there".
ENV WINEPREFIX=/root/.wineprefix
