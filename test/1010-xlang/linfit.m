# Octave based linear regression reference data generation.
noise  = 1;
slope  = (round(100 * rand())-50) / 10;
offset = (round(100 * rand())-50) / 10;
n = 5 + abs(round(90 * rand()));
x = [0:n]' / 10;
y = round((slope * x + offset + (noise * randn(size(x)))) * 1e2) / 1e2;
c = [ones(length(x),1),x] \ y;
disp(jsonencode(struct('slope',c(2), 'offset',c(1), 'x',x , 'y',y)));
