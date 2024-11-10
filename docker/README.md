# Docker test builds for XORP

Make easy building verfication with docker.

To test XORP building on common Linux systems run from main repo's
directory commands:

```bash
# Fedora builds
docker build --build-arg FEDORA_VERSION=39 -t xorp:fedora-39 -f docker/Dockerfile.fedora .
docker build --build-arg FEDORA_VERSION=40 -t xorp:fedora-40 -f docker/Dockerfile.fedora .
docker build --build-arg FEDORA_VERSION=41 -t xorp:fedora-41 -f docker/Dockerfile.fedora .
# Ubuntu builds
docker build --build-arg UBUNTU_VERSION=20.04 -t xorp:ubuntu-20 -f docker/Dockerfile.ubuntu .
docker build --build-arg UBUNTU_VERSION=22.04 -t xorp:ubuntu-20 -f docker/Dockerfile.ubuntu .
docker build --build-arg UBUNTU_VERSION=24.04 -t xorp:ubuntu-20 -f docker/Dockerfile.ubuntu .
# Debian builds
docker build --build-arg DEBIAN_VERSION=11 -t xorp:debian-11 -f docker/Dockerfile.debian .
docker build --build-arg DEBIAN_VERSION=12 -t xorp:debian-12 -f docker/Dockerfile.debian .
```

