FROM ubuntu:24.04 as build

RUN dpkg --add-architecture armhf && \
    apt-get update && \
    apt-get install -y build-essential git g++-arm-linux-gnueabihf libc6:armhf libstdc++6:armhf flex bison curl clang lld \
        pkg-config libfreetype-dev libx11-dev libfreetype-dev:armhf libx11-dev:armhf

WORKDIR /build
COPY . wine

RUN mkdir wine-build32 && \
    cd wine-build32 && \
    ../wine/configure --prefix=/opt/wine --disable-tests --host=arm-linux-gnueabihf && \
    make -j$(nproc) && \
    make -j$(nproc) install-lib
RUN mkdir wine-build64 && \
    cd wine-build64 && \
    ../wine/configure --prefix=/opt/wine --disable-tests && \
    make -j$(nproc) && \
    make -j$(nproc) install-lib

FROM ubuntu:24.04 as runtime

RUN dpkg --add-architecture armhf && \
    apt-get update && \
    apt-get install -y libc6:armhf libstdc++6:armhf \
        libx11-6 libfreetype6 libx11-6:armhf libfreetype6:armhf \
        build-essential git curl

COPY --from=build /opt/wine /opt/wine
ENV PATH=/opt/wine/bin:$PATH
