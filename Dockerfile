FROM akuhak/ps2toolchain:new_gcc_test

ENV PS2SDK $PS2DEV/ps2sdk
ENV PATH   $PATH:$PS2SDK/bin

COPY . /ps2sdk
RUN ls -l /ps2sdk

#RUN git clone https://github.com/AKuHAK/ps2sdk -b new_gcc_test /ps2sdk \
RUN make install -C /ps2sdk \
    && cd /ps2sdk \
    && make install \
    && rm -rf \
        /ps2sdk \
        /ps2dev/ps2sdk/test.tmp
