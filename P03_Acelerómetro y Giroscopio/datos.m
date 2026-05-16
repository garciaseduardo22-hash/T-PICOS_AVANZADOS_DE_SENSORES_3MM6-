%% 1. CONEXIÓN Y CONFIGURACIÓN
if ~exist('m','var')
    try
        m = mobiledev;
    catch
        error('Asegúrate de que MATLAB Mobile esté transmitiendo y con sesión iniciada.');
    end
end
m.AccelerationSensorEnabled = 1;
m.AngularVelocitySensorEnabled = 1;

%% 2. CAPTURA DE DATOS
m.Logging = 1; 
fprintf('Iniciando grabación de 10 segundos...\n');
pause(10); 
m.Logging = 0;
fprintf('Grabación finalizada.\n\n');

[accel_raw, t_acc] = accellog(m);
[gyro_raw, t_gyro] = angvellog(m);

%% 3. PROCESAMIENTO (SOLUCIÓN AL ERROR)
% Extraer componentes de aceleración (m/s^2) 
ax = accel_raw(:,1); 
ay = accel_raw(:,2); 
az = accel_raw(:,3);

% Extraer componentes de velocidad angular y convertir a grados/s [cite: 20]
% MATLAB Mobile entrega rad/s por defecto
gx = gyro_raw(:,1) * (180/pi); 
gy = gyro_raw(:,2) * (180/pi);

% Ángulos SOLO ACELERÓMETRO
% Se calculan sobre todo el vector antes del bucle
roll_acc = atan2(ay, sqrt(ax.^2 + az.^2)) * (180/pi);
pitch_acc = atan2(-ax, sqrt(ay.^2 + az.^2)) * (180/pi);

% Preparación para integración y filtro
n = min([length(roll_acc), length(gx)]);
roll_gyro = zeros(n, 1);  % Solo giroscopio [cite: 20]
pitch_gyro = zeros(n, 1);
roll_comp = zeros(n, 1);  % Filtro complementario [cite: 22]
pitch_comp = zeros(n, 1);

alpha = 0.98; % Factor de confianza (Pasa-bajas para acel, Pasa-altas para giro) [cite: 22]

for i = 2:n
    dt = t_gyro(i) - t_gyro(i-1); % dt real entre muestras
    
    % 1. SOLO GIROSCOPIO (Integración pura) [cite: 20]
    roll_gyro(i) = roll_gyro(i-1) + gx(i)*dt;
    pitch_gyro(i) = pitch_gyro(i-1) + gy(i)*dt;
    
    % 2. FILTRO COMPLEMENTARIO (Fusión) [cite: 22]
    roll_comp(i) = alpha * (roll_comp(i-1) + gx(i)*dt) + (1-alpha) * roll_acc(i);
    pitch_comp(i) = alpha * (pitch_comp(i-1) + gy(i)*dt) + (1-alpha) * pitch_acc(i);
end

%% 4. VISUALIZACIÓN 
figure('Name', 'Comparativa de Estimación de Ángulos');
subplot(2,1,1);
plot(t_acc(1:n), roll_acc(1:n), 'r', 'DisplayName', 'Acelerómetro (Ruido)'); hold on;
plot(t_gyro(1:n), roll_gyro(1:n), 'g', 'DisplayName', 'Giroscopio (Deriva)');
plot(t_gyro(1:n), roll_comp(1:n), 'b', 'LineWidth', 1.5, 'DisplayName', 'Filtro Complementario');
title('Cálculo de ROLL'); ylabel('Grados'); legend; grid on;

subplot(2,1,2);
plot(t_acc(1:n), pitch_acc(1:n), 'r', 'DisplayName', 'Acelerómetro'); hold on;
plot(t_gyro(1:n), pitch_gyro(1:n), 'g', 'DisplayName', 'Giroscopio');
plot(t_gyro(1:n), pitch_comp(1:n), 'b', 'LineWidth', 1.5, 'DisplayName', 'Filtro');
title('Cálculo de PITCH'); ylabel('Grados'); xlabel('Tiempo (s)'); grid on;