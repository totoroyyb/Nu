#!/bin/bash

sudo docker ps | awk '{print $1}' | grep -v CON | xargs sudo docker stop
sudo docker ps | awk '{print $1}' | grep -v CON | xargs sudo docker kill
