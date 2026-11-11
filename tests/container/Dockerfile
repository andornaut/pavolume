ARG PYTHON_VERSION=3.12
FROM python:${PYTHON_VERSION}
RUN apt-get update && apt-get install --no-install-recommends -y libpulse-dev && apt-get clean
COPY tests/container/check /opt/check/run
WORKDIR /work
USER 19001:19001
