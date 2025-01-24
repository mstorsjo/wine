FROM ubuntu:24.04 as build

RUN dpkg --add-architecture armhf && \
    apt-get update && \
    apt-get install -y build-essential git g++-arm-linux-gnueabihf libc6:armhf libstdc++6:armhf flex bison curl \
        pkg-config libfreetype-dev libx11-dev libfreetype-dev:armhf libx11-dev:armhf
# clang lld

# 20241030 is the last version before llvm-mingw switched to Clang config files;
# older Wine before 9.21 (0872e3c1ff242dc17f18749efb1e285c1155399b) fail to
# build with llvm-mingw that uses Clang config files.
RUN curl -LO https://github.com/mstorsjo/llvm-mingw/releases/download/20241030/llvm-mingw-20241030-ucrt-ubuntu-20.04-aarch64.tar.xz && \
    tar -Jxf llvm-mingw*.tar.xz && \
    rm llvm-mingw*.tar.xz && \
    mv llvm-mingw* /opt/llvm-mingw
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
    ../wine/configure --prefix=/opt/wine --disable-tests --enable-win64 && \
    make -j$(nproc) && \
    make -j$(nproc) install-lib

FROM ubuntu:24.04 as runtime

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
