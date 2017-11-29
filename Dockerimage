FROM akuhak/ps2toolchain:new_gcc_test

ENV PS2SDK $PS2DEV/ps2sdk
ENV PATH   $PATH:$PS2SDK/bin

ENV DEBIAN_FRONTEND noninteractive

RUN git clone https://github.com/AKuHAK/ps2sdk /ps2sdk \
    && cd /ps2sdk \
    && make install
