#!/bin/sh
set -e

echo "=== Criando diretório /etc/udev/rules.d caso não exista ==="
sudo mkdir -p /etc/udev/rules.d

echo "=== Configurando permissões de áudio para o grupo audio ==="
echo 'SUBSYSTEM=="sound", KERNEL=="controlC[0-9]*|pcmC[0-9]*|hwC[0-9]*", GROUP="audio", MODE="0660"' | sudo tee /etc/udev/rules.d/99-audio.rules

echo "=== Recarregando regras do udev ==="
sudo udevadm control --reload-rules
sudo udevadm trigger --subsystem-match=sound

echo "=== Verificando permissões em /dev/snd/ ==="
ls -l /dev/snd/controlC0 /dev/snd/pcmC0D0p

echo ""
echo "Concluído com sucesso! As permissões de áudio agora estão liberadas para o seu usuário."
