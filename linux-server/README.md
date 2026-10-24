# ForestNetra Linux Server

This directory contains the code and documentation for receiving and storing ForestNetra sensor data on a Linux system.

## Server environment

- Linux host: `min-dashboard`
- Project directory: `/home/min/ForestNetra`
- Data logger: `csv_logger.py`
- HTTP server port: `8081`

## Data logging

The Python server receives sensor data through an HTTP endpoint and appends measurements to a CSV file.

- Data endpoint: `POST /log`
- Health-check endpoint: `GET /health`
- CSV file: `forest_netra_data.csv`
- Time zone used for server-generated timestamps: `Asia/Kolkata`

## Service management

The logger is configured to run as a systemd service so it can restart automatically when required.

## Files to add

- `csv_logger.py` — verified Python logging server
- `requirements.txt` — only if external Python packages are needed
- `forest-netra-csv.service.example` — example systemd service configuration
- Setup and troubleshooting instructions

## Security

Do not commit passwords, private SSH keys, Wi-Fi credentials, API tokens or other secrets. Review configuration files before uploading them.
