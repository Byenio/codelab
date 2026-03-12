#!/bin/bash

mkdir -p /home/git/.ssh
touch /home/git/.ssh/authorized_keys
chown -R git:git /home/git/.ssh
chmod 700 /home/git/.ssh
chmod 600 /home/git/.ssh/authorized_keys

ssh-keygen -A

/usr/sbin/sshd

echo "Starting Codelab server..."
echo "  DB_PATH: $DB_PATH"
echo "  DB_SCHEMA_PATH: $DB_SCHEMA_PATH"

su git -c "export DB_PATH='$DB_PATH'; export DB_SCHEMA_PATH='$DB_SCHEMA_PATH'; cd /app && ./codelab_server"