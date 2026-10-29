# Smart Terrarium Controller

An IoT system for monitoring and controlling terrarium conditions. It combines
ESP32 firmware, a C++ REST API, and a React dashboard.

## Architecture

- **Firmware:** Reads AHT10 and BH1750 sensors, controls temperature,
  humidity, and lighting, and reports measurements from an ESP32.
- **Backend:** A Pistache REST API stores sensor data and setpoints in SQLite.
- **Frontend:** A React dashboard built with Vite displays readings and charts
  and lets users update setpoints.

## Local development

Run the backend and frontend from Ubuntu in WSL. The backend requires Pistache
and SQLite; the frontend requires Node.js and npm.

### Build Pistache (first-time setup)

Install the Linux build dependencies:

```sh
sudo apt update
sudo apt install build-essential git libevent-dev libssl-dev libsqlite3-dev meson ninja-build pkg-config
```

Build and install Pistache:

```sh
mkdir -p "$HOME/.cache"
git clone --depth 1 https://github.com/pistacheio/pistache.git "$HOME/.cache/pistache-src"
cd "$HOME/.cache/pistache-src"
meson setup build \
  --buildtype=release \
  -DPISTACHE_USE_SSL=true \
  -DPISTACHE_BUILD_TESTS=false \
  -DPISTACHE_BUILD_EXAMPLES=false \
  -DPISTACHE_BUILD_DOCS=false \
  -DPISTACHE_USE_CONTENT_ENCODING_BROTLI=false \
  -DPISTACHE_USE_CONTENT_ENCODING_DEFLATE=false \
  -DPISTACHE_USE_CONTENT_ENCODING_ZSTD=false \
  --prefix=/usr/local
meson compile -C build
sudo meson install -C build
sudo ldconfig
```

### Start the application

From the project directory:

```sh
make backend
make frontend-install
```

Start the backend and frontend in separate terminals:

```sh
make run-backend
```

```sh
make run-frontend
```

Open `http://localhost:5173`. The Vite development server proxies `/api`
requests to the backend at `http://localhost:9080`. The backend uses
`sensor_data.db` in its working directory by default. Set `TERRARIUM_DB_PATH`
to use a different database file, or `TERRARIUM_PORT` to change the backend
port.

## Firmware credentials

Copy `embedded/secrets.example.h` to `embedded/secrets.h` and set your Wi-Fi
credentials and backend URL in the local copy. The secrets file is excluded
from version control. If credentials have ever been committed or shared
publicly, change them.

The firmware keeps the original control behavior: PID temperature control,
timed humidity relay control, and light control with a dead zone. It reads the
sensors and updates the controllers 20 times, waiting 100 ms after each read,
then uploads the latest measurement and temperature output. Failed sensor
reads are skipped.

The board must be able to reach the backend URL configured in the local
`embedded/secrets.h` file.

Install Arduino CLI, the ESP32 core, and the PID library to build the firmware:

```sh
arduino-cli core install esp32:esp32
arduino-cli lib install PID
arduino-cli compile --fqbn esp32:esp32:esp32 embedded
```
