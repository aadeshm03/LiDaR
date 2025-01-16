% Initialize serial port
clear; clc;
serialPorts = 'COM4'; % Change this to your serial port
baudRate = 115200; % Set the baud rate
s = serialport(serialPorts, baudRate);
configureTerminator(s, "CR/LF"); % Set the line terminator
flush(s); % Clear any previous data

% Create a new fi
% Create a new figure for the cube visualization
figure;
hold on;
axis equal;
grid on;

% Define vertices of the cube
vertices = [-1 -1 -1; 1 -1 -1; 1 1 -1; -1 1 -1; -1 -1 1; 1 -1 1; 1 1 1; -1 1 1];

% Define the faces of the cube
faces = [1 2 3 4; 5 6 7 8; 1 2 6 5; 2 3 7 6; 3 4 8 7; 4 1 5 8];

% Define the colors for each face of the cube
faceColors = [1 0 0;  % Red for face 1
              0 1 0;  % Green for face 2
              0 0 1;  % Blue for face 3
              1 1 0;  % Yellow for face 4
              1 0 1;  % Magenta for face 5
              0 1 1]; % Cyan for face 6

% Create the patch object for the cube with different colors for each face
h = patch('Vertices', vertices, 'Faces', faces, 'FaceColor', 'flat', 'FaceVertexCData', faceColors);

% Set axis for 3D visualization and hold the plot
axis([-2 2 -2 2 -2 2]);
xlabel('X');
ylabel('Y');
zlabel('Z');
grid on;
view(3);  % Set 3D view

% Define the 3D axis lines for orientation visualization
quiver3(0, 0, 0, 1, 0, 0, 'r', 'LineWidth', 2);  % X-axis in red
quiver3(0, 0, 0, 0, 1, 0, 'g', 'LineWidth', 2);  % Y-axis in green
quiver3(0, 0, 0, 0, 0, 1, 'b', 'LineWidth', 2);  % Z-axis in blue

% Continuously read data and update the 3D object
while true
    try
        % Read data from the serial port
        disp("hi")
        data = fscanf(s, '%s');
        
        disp(['Received data: ', data]);  % Debugging

        % Process JSON formatted data
        if startsWith(data, '{') && endsWith(data, '}')
            % Decode the JSON data
            dataStruct = jsondecode(data);

            % Check if the required fields are present
            if isfield(dataStruct, 'quat_w') && isfield(dataStruct, 'quat_x') && ...
               isfield(dataStruct, 'quat_y') && isfield(dataStruct, 'quat_z') && ...
               isfield(dataStruct, 'LidarDistance')

                % Extract quaternion values
                q0 = dataStruct.quat_w;
                q1 = dataStruct.quat_x;
                q2 = dataStruct.quat_y;
                q3 = dataStruct.quat_z;

                % Extract LiDAR distance
                lidar_distance = dataStruct.('LidarDistance');

                % Normalize quaternion (important for valid rotation matrix)
                q = [q0, q1, q2, q3];
                q = q / norm(q);

                % Convert quaternion to rotation matrix
                R = quat2rotm(q);

                % Update cube vertices
                h.Vertices = (R * vertices')';

                % Redraw plot
                drawnow;

                % Display LiDAR distance on the figure
                disp(['LiDAR Distance: ', num2str(lidar_distance), ' meters']);
            else
                disp('IMU quaternion or LiDAR distance data missing or incomplete.');
            end
        else
            disp('Received data is not valid JSON.');
        end
    catch ME
        disp(['Error: ', ME.message]);
    end
end

function R = quat2rotm(q)
    % Convert quaternion to rotation matrix
    w = q(1); x = q(2); y = q(3); z = q(4);
    R = [1-2*(y^2+z^2), 2*(x*y-w*z), 2*(x*z+w*y);
         2*(x*y+w*z), 1-2*(x^2+z^2), 2*(y*z-w*x);
         2*(x*z-w*y), 2*(y*z+w*x), 1-2*(x^2+y^2)];
end
