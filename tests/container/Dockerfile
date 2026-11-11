ARG PYTHON_VERSION=3.12
FROM python:${PYTHON_VERSION}
RUN apt-get update && apt-get install --no-install-recommends -y libpulse-dev && apt-get clean
COPY --chown=19001:19001 Makefile pavolume.c .python-version /opt/source/
COPY --chown=19001:19001 tests/container/compiler /opt/check/compiler
WORKDIR /work
USER 19001:19001
