#!/bin/bash

CUR_DIR=$PWD  #now it build dir

cd $PWD/../../../external/ffmpeg/linux

echo -e ${UNDERLINE}\"Configure ffmpeg\"${BREAK}

PATH="$CUR_DIR/bin:$PATH" PKG_CONFIG_PATH="$CUR_DIR/lib/pkgconfig" ./configure \
        --prefix="$CUR_DIR" \
        --pkg-config-flags="--static" \
        --extra-cflags="-I$CUR_DIR/include" \
        --extra-ldflags="-L$CUR_DIR/lib" \
        --extra-libs="-lpthread -lm" \
        --ld="g++" \
        --bindir="$CUR_DIR/bin" \
        --disable-doc \
        --disable-shared \
        --enable-static \
        --arch="amd64" \
        --enable-gpl \
        --enable-gnutls \
        --enable-libaom \
        --enable-libass \
        --enable-libfreetype \
        --enable-libmp3lame \
        --enable-libopus \
        --enable-libfontconfig \
        --enable-vaapi \
        --enable-libvorbis \
        --enable-libvpx \
        --enable-libx264 \
        --enable-libx265 \
        --enable-libdrm \
        --enable-opengl \
        --enable-libfribidi \
        --enable-libpulse \
        --enable-pic \
        --disable-libdav1d \
        --enable-nonfree

# --enable-libdc1394 \ removed yet
# --enable-libfdk-aac \
# --enable-sdl2 \
# --enable-libdav1d \


echo -e ${UNDERLINE}\"Configure ffmpeg done. Now build and install\"${BREAK}

PATH="$CUR_DIR/bin:$PATH" make && \
make install && \
hash -r

echo -e ${UNDERLINE}\"Build and install ffmpeg done\"${BREAK}

cd $PWD/../../../build
