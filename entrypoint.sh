#!/bin/bash

mkdir -p /var/run/sshd
chmod 0755 /var/run/sshd 2>/dev/null || true

if [ ! -d "/home/git/.ssh" ]; then
    mkdir -p /home/git/.ssh
    chmod 700 /home/git/.ssh 2>/dev/null || true
    chown git:git /home/git/.ssh 2>/dev/null || true
fi

if [ ! -f "/home/git/.ssh/authorized_keys" ]; then
    touch /home/git/.ssh/authorized_keys
    chmod 600 /home/git/.ssh/authorized_keys 2>/dev/null || true
    chown git:git /home/git/.ssh/authorized_keys 2>/dev/null || true
fi

chown -R git:git /home/git 2>/dev/null || true
chmod 700 /home/git/.ssh 2>/dev/null || true
chmod 600 /home/git/.ssh/authorized_keys 2>/dev/null || true

mkdir -p /app/data/ssh 2>/dev/null || true
if [ ! -f "/app/data/ssh/ssh_host_rsa_key" ]; then
    ssh-keygen -t rsa -b 4096 -f /app/data/ssh/ssh_host_rsa_key -N "" -q
    ssh-keygen -t ed25519 -f /app/data/ssh/ssh_host_ed25519_key -N "" -q
fi

# Start sshd in the foreground (-D) and send logs to stderr (-e) so they appear in container output
# We bind it explicitly to the custom host keys mounted inside the writable volume and store PID there!
/usr/sbin/sshd -D -e -p 2222 -h /app/data/ssh/ssh_host_rsa_key -h /app/data/ssh/ssh_host_ed25519_key -o "PidFile=/app/data/ssh/sshd.pid" -o "StrictModes=no" &

echo "Starting Codelab C++ API server..."
echo "  DB_PATH: $DB_PATH"

if [ "$(id -u)" = "0" ]; then
    CMD_PREFIX="su git -c"
else
    CMD_PREFIX="bash -c"
fi

$CMD_PREFIX "export DB_PATH='$DB_PATH'; export DB_SCHEMA_PATH='$DB_SCHEMA_PATH'; cd /app && ./codelab_server" &

echo "Starting Nuxt Frontend server..."
$CMD_PREFIX "export PORT=3000; export NITRO_PORT=\${PORT:-3000}; export NITRO_HOST=0.0.0.0; cd /app && node frontend/server/index.mjs" &

# Wait for processes
wait -n
