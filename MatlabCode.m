%% 2DX3 Final Project MATLAB UART Reader + Multi-Scan 3D Plot

clear;
clc;

%% Configuration
port = "COM3";
baudrate = 115200;
maxScans = 2;
pointsPerScan = 32;

%% Open serial port
device = serialport(port, baudrate);
device.Timeout = 20;
configureTerminator(device, "CR/LF");
flush(device);
fprintf("Opened %s\n", port);
disp("Press the button on the MCU for EACH scan...");

%% Storage
X = [];
Y = [];
Z = [];
C = []; % scan index
A = []; % angle index

%% LOOP OVER SCANS

for scan = 1:maxScans
    fprintf("\n--- Waiting for Scan %d ---\n", scan);
    pointsRead = 0;
    while pointsRead < pointsPerScan
        try
        raw = readline(device);
        catch
        disp("Timeout waiting for data...");
        break;
      end
    if isempty(raw)
        continue;
    end

line = strtrim(string(raw));
disp("Received: " + line);

% format: angle,distance,status,signal,ambient,spad

vals = sscanf(line, '%f,%f,%f,%f,%f,%f');
if numel(vals) == 6
    angleNum = vals(1);
    distance = vals(2) / 10; % convert mm → cm
    theta = (angleNum - 1) * (2*pi / pointsPerScan);%THETA ANGLE THINGY
x = (scan - 1) * 30; % exaggerated spacing for visualization
y = distance * cos(theta);
z = distance * sin(theta);

X(end+1) = x;
Y(end+1) = y;
Z(end+1) = z;
C(end+1) = scan;
A(end+1) = angleNum;

pointsRead = pointsRead + 1;

fprintf("Scan %d Point %d -> (%.2f, %.2f, %.2f)\n", ...
...
scan, pointsRead, x, y, z);

end
end

fprintf("Scan %d complete.\n", scan);

end

%% Close serial
clear device;

%% Plot settings
num_positions = maxScans;
x_positions = 0:30:(30*(num_positions-1));
cmap = jet(num_positions);

%% Plot
figure;

% ----- Scatter -----
subplot(1,2,1)
scatter3(X,Y,Z,60,C,'filled')
colormap(cmap)
colorbar
xlabel('X (cm)')
ylabel('Y (cm)')
zlabel('Z (cm)')
title('3D Scatter')
grid on

% ----- Connected rings -----
subplot(1,2,2)
hold on
for p = 1:num_positions
idx = find(C == p);
if isempty(idx)
continue;
end
[~, s] = sort(A(idx));
idx = idx(s);
y_ring = Y(idx);
z_ring = Z(idx);
y_loop = [y_ring, y_ring(1)];
z_loop = [z_ring, z_ring(1)];
x_loop = repmat(x_positions(p),1,length(y_loop));
plot3(x_loop,y_loop,z_loop,'-o', ...
'Color',cmap(p,:), ...
'LineWidth',2, ...
'MarkerFaceColor',cmap(p,:));

if p > 1
    prev_idx = find(C == p-1);
    [~, s2] = sort(A(prev_idx));
    prev_idx = prev_idx(s2);
    n = min(length(prev_idx), length(idx));

for k = 1:n
    plot3([x_positions(p-1), x_positions(p)], ...
    [Y(prev_idx(k)), Y(idx(k))], ...
    [Z(prev_idx(k)), Z(idx(k))], ...
    '-', 'Color',[0.7 0.7 0.7]);
end
end
end

xlabel('X (cm)')
ylabel('Y (cm)')
zlabel('Z (cm)')
title('Scan Planes Connected')
grid on
view(35,25)
hold off