#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/Build"
INSTALL_DIR="/opt/simpleelo"
SERVICE_NAME="simpleelo-server"
LISTEN_HOST="0.0.0.0"
LISTEN_PORT="18080"
DATA_FILE="${INSTALL_DIR}/data/serverData.sqlite"
ENABLE_SYSTEMD="1"

print_help() {
  cat <<EOF
Usage: $(basename "$0") [options]

Options:
  --install-dir <path>    Install root directory (default: /opt/simpleelo)
  --service-name <name>   systemd service name (default: simpleelo-server)
  --host <ip>             Listen host (default: 0.0.0.0)
  --port <port>           Listen port (default: 18080)
  --data-file <path>      Data file path in runtime config
  --no-systemd            Build and install only, do not create systemd service
  -h, --help              Show this help message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --install-dir)
      INSTALL_DIR="$2"
      shift 2
      ;;
    --service-name)
      SERVICE_NAME="$2"
      shift 2
      ;;
    --host)
      LISTEN_HOST="$2"
      shift 2
      ;;
    --port)
      LISTEN_PORT="$2"
      shift 2
      ;;
    --data-file)
      DATA_FILE="$2"
      shift 2
      ;;
    --no-systemd)
      ENABLE_SYSTEMD="0"
      shift
      ;;
    -h|--help)
      print_help
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      print_help
      exit 1
      ;;
  esac
done

if ! command -v cmake >/dev/null 2>&1; then
  echo "cmake is required but not found"
  exit 1
fi

if ! command -v g++ >/dev/null 2>&1; then
  echo "g++ is required but not found"
  exit 1
fi

echo "[1/5] Configure server-only build"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DSIMPLEELO_BUILD_SERVER=ON \
  -DSIMPLEELO_BUILD_CLIENT=OFF \
  -DSIMPLEELO_ENABLE_TESTS=OFF

echo "[2/5] Build simpleelo_server"
cmake --build "${BUILD_DIR}" --target simpleelo_server -j

SERVER_BIN="${BUILD_DIR}/server/simpleelo_server"
if [[ ! -f "${SERVER_BIN}" ]]; then
  echo "Build succeeded but binary not found: ${SERVER_BIN}"
  exit 1
fi

echo "[3/5] Install runtime files to ${INSTALL_DIR}"
sudo mkdir -p "${INSTALL_DIR}/bin" "${INSTALL_DIR}/config" "$(dirname "${DATA_FILE}")"
sudo cp "${SERVER_BIN}" "${INSTALL_DIR}/bin/simpleelo_server"
sudo chmod +x "${INSTALL_DIR}/bin/simpleelo_server"

TMP_CONFIG="$(mktemp)"
cat > "${TMP_CONFIG}" <<EOF
{
  "listenHost": "${LISTEN_HOST}",
  "listenPort": ${LISTEN_PORT},
  "dataFilePath": "${DATA_FILE}"
}
EOF
sudo cp "${TMP_CONFIG}" "${INSTALL_DIR}/config/server_config.json"
rm -f "${TMP_CONFIG}"

if [[ "${ENABLE_SYSTEMD}" == "1" ]]; then
  echo "[4/5] Create/update systemd service ${SERVICE_NAME}"
  TMP_SERVICE="$(mktemp)"
  cat > "${TMP_SERVICE}" <<EOF
[Unit]
Description=SimpleElo Server
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
WorkingDirectory=${INSTALL_DIR}
ExecStart=${INSTALL_DIR}/bin/simpleelo_server --config ${INSTALL_DIR}/config/server_config.json
Restart=always
RestartSec=3
User=root

[Install]
WantedBy=multi-user.target
EOF

  sudo cp "${TMP_SERVICE}" "/etc/systemd/system/${SERVICE_NAME}.service"
  rm -f "${TMP_SERVICE}"

  echo "[5/5] Enable and restart systemd service"
  sudo systemctl daemon-reload
  sudo systemctl enable --now "${SERVICE_NAME}"
  sudo systemctl status "${SERVICE_NAME}" --no-pager
else
  echo "[4/5] Skip systemd as requested"
  echo "[5/5] Start manually with:"
  echo "${INSTALL_DIR}/bin/simpleelo_server --config ${INSTALL_DIR}/config/server_config.json"
fi

echo "Deployment finished"
