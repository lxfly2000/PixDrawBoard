@echo off
curl https://lightningmaker.live/api/getimage?url=lightningmaker.live:5001 -o getimage.png
Release\imgdiff.exe flonne-pix-draw.png getimage.png diff.png FFFFFFFF 1
Release\pix2cmd.exe diff.png > diff.txt
