@echo off
curl https://lightningmaker.live/api/getimage?url=lightningmaker.live:5001 -o getimage.png
mspaint getimage.png
Release\imgdiff.exe flonne-pix-draw.png getimage.png
Release\pix2cmd.exe diff.png > diff.txt
