#!/bin/bash

if [ ! -d "/home/git/.ssh" ]; then
    mkdir -p /home/git/.ssh
    chmod 700 /home/git/.ssh
    chown git:git /home/git/.ssh
fi

if [ ! -f "/home/git/.ssh/authorized_keys" ]; then
    touch /home/git/.ssh/authorized_keys
    chmod 600 /home/git/.ssh/authorized_keys
    chown git:git /home/git/.ssh/authorized_keys
fi

chown -R git:git /home/git
chmod 700 /home/git/.ssh
chmod 600 /home/git/.ssh/authorized_keys

ssh-keygen -A

/usr/sbin/sshd

echo "Starting Codelab server..."
echo "  DB_PATH: $DB_PATH"

su git -c "export DB_PATH='$DB_PATH'; export DB_SCHEMA_PATH='$DB_SCHEMA_PATH'; cd /app && ./codelab_server"