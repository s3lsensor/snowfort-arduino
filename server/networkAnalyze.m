% networkAnalyze.m

clear all
close all

D = importfile('log2.csv');

sensorTime = D{:,3}/1000;
mean(sensorTime(2:end)-sensorTime(1:end-1))
std(sensorTime(2:end)-sensorTime(1:end-1))

plot(D{:,5}/16384)
plot(D{:,8}/131)