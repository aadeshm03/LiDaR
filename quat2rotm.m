function R = quat2rotm(q)
    % This function converts a quaternion q = [w, x, y, z] to a rotation matrix R.
    % The input quaternion q is a 1x4 vector: [w, x, y, z]
    % The output R is a 3x3 rotation matrix.

    % Extract quaternion components
    w = q(1); % Scalar part
    x = q(2); % Vector part (x)
    y = q(3); % Vector part (y)
    z = q(4); % Vector part (z)

    % Compute the rotation matrix using the quaternion components
    R = [1 - 2*(y^2 + z^2), 2*(x*y - z*w), 2*(x*z + y*w);
         2*(x*y + z*w), 1 - 2*(x^2 + z^2), 2*(y*z - x*w);
         2*(x*z - y*w), 2*(y*z + x*w), 1 - 2*(x^2 + y^2)];
end