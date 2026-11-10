ARG PYTHON_VERSION=3.12
FROM node:24.22.2 AS node
FROM python:${PYTHON_VERSION}
COPY --from=node /usr/local /opt/node
ENV PATH="/opt/node/bin:/opt/venv/bin:${PATH}" \
    PYTHONDONTWRITEBYTECODE=1 \
    PIP_DISABLE_PIP_VERSION_CHECK=1
RUN apt-get update && apt-get install --no-install-recommends -y libpulse-dev && apt-get clean
COPY requirements-dev.txt /opt/check/requirements-dev.txt
RUN python -m venv /opt/venv && /opt/venv/bin/python -m pip install -r /opt/check/requirements-dev.txt
RUN npm install --prefix /opt/check-tools --ignore-scripts --no-audit --no-fund markdownlint-cli2@0.24.0
COPY tests/container/check /opt/check/run
WORKDIR /work
USER 19001:19001
