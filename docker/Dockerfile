# This docker file is to build a zero training environment
FROM nvidia/cuda:12.4.1-cudnn-devel-ubuntu22.04

ARG DEBIAN_FRONTEND=noninteractive
ARG PYTORCH_VERSION=2.4.1
ARG PYTORCH_CUDA=cu124

ENV TZ=Asia/Taipei

RUN rm /etc/apt/sources.list.d/cuda.list || true && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    vim \
    gdb \
    git \
    less \
    htop \
    wget \
    cmake \
    ffmpeg \
    clang-format \
    python3-pip \
    python3-dev \
    python3-opencv \
    python3-pybind11 \
    libopencv-dev \
    libarchive-tools \
    libboost-all-dev && \
    ln -s /usr/bin/python3 /usr/bin/python && \
    python3 -m pip install -U pip \
    gdown \
    cmake==3.25.2 \
    numpy \
    pandas \
    jupyter \
    matplotlib && \
    python3 -m pip cache purge && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# install pytorch & libtorch
RUN python3 -m pip install torch==${PYTORCH_VERSION} --index-url https://download.pytorch.org/whl/${PYTORCH_CUDA} && \
    wget -qO- https://download.pytorch.org/libtorch/${PYTORCH_CUDA}/libtorch-cxx11-abi-shared-with-deps-${PYTORCH_VERSION}%2B${PYTORCH_CUDA}.zip | bsdtar -xvf- -C /usr/local && \
    find /usr/local/libtorch/lib -maxdepth 1 -name 'libcudnn*.so*' -delete
ENV CMAKE_PREFIX_PATH=/usr/local/libtorch/share/cmake/Torch

# install gym/atari & Arcade Learning Environment (ALE v0.8.0)
# You can download Atari game ROMs from the following link: https://www.atarimania.com/rom_collection_archive_atari_2600_roms.html
RUN python3 -m pip install gym[atari] && \
    python3 -m pip cache purge && \
    mkdir -p /opt/atari57 && \
    if [ -n "$(ls -A /opt/atari57 2>/dev/null)" ]; then ale-import-roms /opt/atari57; else echo "Skipping Atari ROM import; add ROMs to /opt/atari57 later if needed."; fi && \
    cd /opt/ && \
    git clone https://github.com/mgbellemare/Arcade-Learning-Environment.git && \
    cd Arcade-Learning-Environment && \
    git checkout d59d00688b58c5c14dff5fc79db5c22e86987f5d && \
    mkdir build && cd build && \
    cmake ../ -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 -DBUILD_PYTHON_LIB=OFF && \
    cmake --build . --target install -j && \
    cd /opt/ && rm -rf Arcade-Learning-Environment

# install gogui
RUN apt-get update && \
    apt-get install -y unzip gconf2 default-jre && \
    gdown https://drive.google.com/uc?id=1wF65yyD5BjL0MkZRFCWLvmqWYx8TDgk0 && \
    unzip gogui-1.4.9.zip && \
    echo "exit 0" >> gogui-1.4.9/install.sh && \
    cd gogui-1.4.9 && \
    ./install.sh && \
    cd .. && \
    rm -rf gogui-1.4.9 gogui-1.4.9.zip && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

RUN mkdir /workspace
WORKDIR /workspace
