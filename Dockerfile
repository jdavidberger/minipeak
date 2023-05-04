FROM images.constructiverealities.io/open-source/dockers/base as build-machine

RUN --mount=type=cache,target=/var/cache/apt,id=apt-cache-$TARGETARCH --mount=type=cache,target=/var/lib/apt,id=apt-lib-$TARGETARCH \
    curl -L http://packages.lunarg.com/lunarg-signing-key-pub.asc -o /lunarg-signing-key-pub.asc && \
    curl -L http://packages.lunarg.com/vulkan/lunarg-vulkan-focal.list -o /etc/apt/sources.list.d/lunarg-vulkan-focal.list &&  \
    apt-key add /lunarg-signing-key-pub.asc && \
    apt update && apt-get install --no-install-recommends -y coreutils retry cmake libopencv-dev openssh-client ccache vulkan-headers libglfw3-dev libgl-dev libegl-dev libglew-dev libopengl-dev libvulkan-dev libgles-dev libglm-dev libfuse2 && \
    ssh-keygen -f /root/.ssh/id_rsa && \
    eval $(ssh-agent -s) && \
    mkdir -p ~/.ssh && \
    chmod 700 ~/.ssh && \
    echo -e "Host *\n\tStrictHostKeyChecking no\n\n" > ~/.ssh/config && \
    pip install virtualenv cmake

ENV GIT_SSH_COMMAND='ssh -o StrictHostKeyChecking=no'

FROM build-machine as build

ENV CMAKE_PATH=cmake

#  --mount=type=cache,target=/build-repo/depthaikernels,id=dai-kernels-build-$TARGETARCH \
#RUN --mount=type=bind,source=.,target=/source,rw cd /source && cibuildwheel --platform linux .
ADD . /source
RUN mkdir -p /build-repo/minipeak && \
    mkdir -p /install && \
    mkdir -p /minipeak/install && \
    pip install cmake && \
    cd /build-repo/minipeak && $CMAKE_PATH -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/minipeak/install /source && make -j10 install

FROM build-machine
COPY --from=build /minipeak/install /minipeak/install
COPY --from=build /minipeak/install/* /
