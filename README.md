# SPB Route Finder

Desktop C++/Qt application for round-trip route search through the Yandex Rasp API. By default, the app searches trips between Санкт-Петербург and Ростов-на-Дону, allows at most one transfer, caches API responses, and renders the result inside the GUI.

## Features

- simple Qt GUI with city-name input and colorful route cards
- round-trip route search with at most one transfer
- date validation from the current day up to one year ahead
- API response caching in memory and in the `cache/` directory
- automatic city-name to Yandex Rasp settlement-code resolution through a cached directory
- quality checks through `clang-tidy` and `cppcheck`

## Requirements

- CMake 3.16+
- C++23-compatible compiler
- Internet access for the initial project setup
- Yandex Rasp API key
- Qt Widgets/Concurrent
- `curl` available in `PATH`

## Quick Start

1. Create a `.env` file in the project root:

```env
API_KEY=your_yandex_rasp_api_key
```

Or export the environment variable:

```bash
export API_KEY=your_yandex_rasp_api_key
```

You can obtain the API key from the official Yandex developer resources:

- Yandex developer dashboard: `https://developer.tech.yandex.ru/`
- Yandex Rasp API access documentation: `https://yandex.ru/dev/rasp/doc/ru/concepts/access`

In the developer dashboard, choose the `Yandex Rasp API` service, generate a key, and complete its activation. According to the official documentation, the activation instructions are sent to the Yandex account email.

2. Build the application:

```bash
cmake -S . -B build
cmake --build build
```

3. Start the GUI:

```bash
./build/bin/navigator
```

After the build, CMake copies the required Qt platform plugins into `build/bin/platforms`, so a normal local launch does not require manual `QT_PLUGIN_PATH` setup.

On the first search, the application may download the Yandex Rasp settlements directory once into `cache/stations_list.json`. That file is used to resolve human-readable city names to settlement codes.

## Usage

1. Start `./build/bin/navigator`.
2. Pick a travel date.
3. Change the departure and destination city names if needed.
4. Keep `Use cache` enabled if you want to reuse API responses and the settlements directory.
5. Click `Search routes`.
6. Review the results in the `Search result` panel rendered as rich HTML route cards.

## Architecture

- `MainWindow`: Qt UI layer, input collection, and HTML result display.
- `CityCodeResolver`: city-name to settlement-code resolution through the cached Yandex directory.
- `RouteSearchService`: application-service layer orchestrating one route search.
- `DataProcessor`: Yandex Rasp API access, caching, and round-trip filtering.
- `DataPrinter`: plain-text report formatting.
- `RouteReportFormatter`: HTML formatting for the GUI.
- `ApiKeyManager`: `API_KEY` loading from the environment or `.env`.

## Error Handling

The application shows an error message when:

- `API_KEY` is missing
- a city name cannot be resolved through the Yandex Rasp directory
- the API returns an error
- the API response contains invalid JSON

If Qt reports `Could not find the Qt platform plugin "wayland"` or `"xcb"`, rebuild the project with `cmake --build build`. The post-build step should recreate `build/bin/platforms`. For non-standard launches, you can set the path explicitly:

```bash
QT_PLUGIN_PATH=./build/bin ./build/bin/navigator
```
