ARG PYTHON_VERSION=3.12
FROM node:24.22.2 AS markdown
WORKDIR /opt/markdown
COPY tests/container/package.json tests/container/package-lock.json ./
RUN npm ci --ignore-scripts --no-audit --no-fund
FROM koalaman/shellcheck:v0.11.0 AS shellcheck
FROM python:${PYTHON_VERSION}
COPY --from=markdown /usr/local /opt/node
COPY --from=markdown /opt/markdown /opt/markdown
COPY --from=shellcheck /bin/shellcheck /usr/local/bin/shellcheck
ENV PATH="/opt/markdown/node_modules/.bin:/opt/node/bin:/opt/venv/bin:${PATH}" \
    PYTHONDONTWRITEBYTECODE=1 \
    PIP_DISABLE_PIP_VERSION_CHECK=1
RUN apt-get update && apt-get install --no-install-recommends -y libpulse-dev && apt-get clean
COPY requirements-dev.txt /opt/check/requirements-dev.txt
RUN python -m venv /opt/venv && /opt/venv/bin/python -m pip install -r /opt/check/requirements-dev.txt
COPY tests/container/check /opt/check/run
WORKDIR /work
USER 19001:19001
