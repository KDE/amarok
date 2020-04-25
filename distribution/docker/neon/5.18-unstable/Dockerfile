FROM kdeneon/plasma:unstable
USER root
WORKDIR /root
ENV container docker
ENV DEBIAN_FRONTEND=noninteractive
ENV DISPLAY=:0
RUN apt-get update; apt-get upgrade -y;
# amarok build dependencies
RUN apt-get install -y cmake extra-cmake-modules g++ git libaio-dev libmariadb-dev libmariadbd-dev libmariadbclient-dev-compat pkg-config  \
    qtdeclarative5-dev qtscript5-dev libqt5svg5-dev qtquickcontrols2-5-dev qtwebengine5-dev libkf5archive-dev libkf5attica-dev \
    libkf5codecs-dev libkf5config-dev libkf5configwidgets-dev libkf5coreaddons-dev libkf5crash-dev libkf5dbusaddons-dev \
    libkf5declarative-dev libkf5dnssd-dev libkf5globalaccel-dev libkf5guiaddons-dev libkf5i18n-dev libkf5iconthemes-dev \
    libkf5kcmutils-dev libkf5kio-dev libkf5newstuff-dev libkf5notifications-dev libkf5notifyconfig-dev libkf5package-dev libkf5solid-dev \
    libkf5texteditor-dev libkf5threadweaver-dev libkf5widgetsaddons-dev libkf5windowsystem-dev libkf5wallet-dev kirigami2-dev \
    gettext libtag1-dev libtag-extras-dev libavcodec-dev libavformat-dev libavdevice-dev libavutil-dev libswscale-dev libpostproc-dev \
    libgpod-dev libgdk-pixbuf2.0-dev libmtp-dev libcurl4-openssl-dev libxml2-dev libssl-dev libgcrypt20-dev \
    libfftw3-dev libloudmouth1-dev libofa0-dev liblastfm5-dev libkf5auth-dev libkf5parts-dev libkf5service-dev \
    libkf5webengineviewer-dev libqt5webchannel5-dev libkf5bookmarks-dev libkf5completion-dev libkf5itemviews-dev \
    libkf5jobwidgets-dev libkf5xmlgui-dev libkf5textwidgets-dev libkf5sonnet-dev
# fetch code
RUN git clone https://github.com/pcgomes/amarok.git
# configure build with cmake
RUN mkdir -p amarok/build
RUN cmake -DCMAKE_BUILD_TYPE=Debug -B amarok/build -S amarok
# build it
RUN make install
CMD ["/bin/bash"]
