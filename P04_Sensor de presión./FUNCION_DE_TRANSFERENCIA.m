clc; clear; close all;

%% 1. Leer el archivo de texto
opciones = detectImportOptions('datos_practica.txt');
datos = readtable('datos_practica.txt', opciones);

% Extracción de columnas del .txt
tiempo_ms = datos{:, 1};
t_bmp180  = datos{:, 2};
t_lm35    = datos{:, 3};

% Convertir el tiempo de milisegundos a segundos y comenzar en cero
tiempo_s = (tiempo_ms - tiempo_ms(1)) / 1000; 

%% 2. Parámetros del Experimento
t_escalon = 0; %ya estaba caliente la camara cuando meti el sensor 
idx_inicio = find(tiempo_s >= t_escalon, 1);
U = 1; % la fuente se puede apagar y encender por eso el valor es 1 

%% 3. Identificación del Sensor LM35
T_init_lm35 = mean(t_lm35(1:idx_inicio+1)); % Temperatura ambiente inicial
T_final_lm35 = max(t_lm35);                 % Temperatura máxima de estabilización
DeltaT_lm35 = T_final_lm35 - T_init_lm35;

K_lm35 = DeltaT_lm35 / U; % Ganancia del sistema (K), es de ganancia unitaria 
V_63_lm35 = T_init_lm35 + (0.632 * DeltaT_lm35); % Valor al 63.2%, como en control 

% Encontrar la constante de tiempo (tau)
idx_tau_lm35 = find(t_lm35 >= V_63_lm35, 1);
tau_lm35 = tiempo_s(idx_tau_lm35) - t_escalon;

% Crear la función de transferencia en tiempo continuo para LM35: G(s) = K / (tau*s + 1)
s = tf('s');
G_lm35 = K_lm35 / (tau_lm35 * s + 1);% chicharronera de control. 

%% 4. Identificación del Sensor BMP180
T_init_bmp = mean(t_bmp180(1:idx_inicio+1));
T_final_bmp = max(t_bmp180);
DeltaT_bmp = T_final_bmp - T_init_bmp;

K_bmp = DeltaT_bmp / U;
V_63_bmp = T_init_bmp + (0.632 * DeltaT_bmp);

% Encontrar la constante de tiempo (tau)
idx_tau_bmp = find(t_bmp180 >= V_63_bmp, 1);
tau_bmp = tiempo_s(idx_tau_bmp) - t_escalon;

% Crear la función de transferencia en tiempo continuo para BMP180
G_bmp = K_bmp / (tau_bmp * s + 1);
%% 5. Resultados 
fprintf('=== FUNCION DE TRANSFERENCIA LM35 ===\n');
fprintf('Ganancia (K): %.4f\n', K_lm35);
fprintf('Constante de tiempo (tau): %.2f segundos\n', tau_lm35);
fprintf('Ecuación Matemática:\n');
fprintf('          %.4f\n', K_lm35);
fprintf('G(s) = -------------\n');
fprintf('        %.4fs + 1\n\n', tau_lm35);

fprintf('=== FUNCION DE TRANSFERENCIA BMP180 ===\n');
fprintf('Ganancia (K): %.4f\n', K_bmp);
fprintf('Constante de tiempo (tau): %.2f segundos\n', tau_bmp);
fprintf('Ecuación Matemática:\n');
fprintf('          %.4f\n', K_bmp);
fprintf('G(s) = -------------\n');
fprintf('        %.4fs + 1\n', tau_bmp);
fprintf('=========================================\n');
%% 6. Graficas
figure('Name', 'Identificación de Sistemas Térmicos');

% Gráfica LM35
subplot(2,1,1);
plot(tiempo_s, t_lm35, 'b', 'LineWidth', 1.5); hold on;
[y_sim_lm35, t_sim] = step(G_lm35, tiempo_s(idx_inicio:end));
plot(t_sim + t_escalon, y_sim_lm35 + T_init_lm35, 'r--', 'LineWidth', 1.5);
grid on;
title('Respuesta al Escalón - Sensor LM35');
xlabel('Tiempo (s)'); ylabel('Temperatura (°C)');
legend('Datos Reales (TXT)', 'Modelo Identificado G(s)', 'Location', 'SouthEast');

% Gráfica BMP180
subplot(2,1,2);
plot(tiempo_s, t_bmp180, 'k', 'LineWidth', 1.5); hold on;
[y_sim_bmp, t_sim_bmp] = step(G_bmp, tiempo_s(idx_inicio:end));
plot(t_sim_bmp + t_escalon, y_sim_bmp + T_init_bmp, 'm--', 'LineWidth', 1.5);
grid on;
title('Respuesta al Escalón - Sensor Integrado BMP180');
xlabel('Tiempo (s)'); ylabel('Temperatura (°C)');
legend('Datos Reales (TXT)', 'Modelo Identificado G(s)', 'Location', 'SouthEast');
%% Fondo Blanco
set(gcf, 'Color', 'w'); 

% Ajustes para la primera gráfica (LM35)
subplot(2,1,1);
set(gca, 'Color', 'w');          % Fondo del área de dibujo blanco
set(gca, 'XColor', 'k', 'YColor', 'k'); % Ejes y números en color negro puro
set(gca, 'GridColor', [0.5 0.5 0.5]);   % Cuadrícula gris tenue para que no estorbe
set(gca, 'FontSize', 11);       % Tamaño de letra ideal para leer en Word

% Ajustes para la segunda gráfica (BMP180)
subplot(2,1,2);
set(gca, 'Color', 'w');          % Fondo del área de dibujo blanco
set(gca, 'XColor', 'k', 'YColor', 'k'); % Ejes y números en color negro puro
set(gca, 'GridColor', [0.5 0.5 0.5]);   % Cuadrícula gris tenue
set(gca, 'FontSize', 11);       % Tamaño de letra ideal